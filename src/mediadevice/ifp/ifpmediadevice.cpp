/***************************************************************************
 * copyright            : (C) 2005-2006 Seb Ruiz <me@sebruiz.net>          *
 *                                                                         *
 * With some code helpers from KIO_IFP                                     *
 *                        (c) 2004 Thomas Loeber <ifp@loeber1.de>          *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

 /**
  *  iRiver ifp media device code
  *  @author Seb Ruiz <me@sebruiz.net>
  *  @see http://ifp-driver.sourceforge.net/libifp/docs/ifp_8h.html
  *  @note ifp uses a backslash '\' as a directory delimiter for _remote_ files
  */

#define DEBUG_PREFIX "IfpMediaDevice"

#include "ifpmediadevice.h"

AMAROK_EXPORT_PLUGIN( IfpMediaDevice )

#include "debug.h"
#include "metabundle.h"
#include "collectiondb.h"
#include "statusbar/statusbar.h"
#include "transferdialog.h"

#include <kapplication.h>
#include <kconfig.h>           //download saveLocation
#include <kiconloader.h>       //smallIcon
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <kurlrequester.h>     //downloadSelectedItems()
#include <kurlrequesterdlg.h>  //downloadSelectedItems()

#include <qfile.h>
#include <qcstring.h>
#include <qregexp.h>

namespace Amarok {
    extern KConfig *config( const QString& );
    extern QString cleanPath( const QString&, bool );
}

/**
 * IfpMediaItem Class
 */

class IfpMediaItem : public MediaItem
{
    public:
        IfpMediaItem( QListView *parent, QListViewItem *after = 0 )
            : MediaItem( parent, after )
        {}

        IfpMediaItem( QListViewItem *parent, QListViewItem *after = 0 )
            : MediaItem( parent, after )
        {}

        void
        setEncodedName( QString &name )
        {
            m_encodedName = QFile::encodeName( name );
        }

        void
        setEncodedName( QCString &name ) { m_encodedName = name; }

        QCString
        encodedName() { return m_encodedName; }

        // List directories first, always
        int
        compare( QListViewItem *i, int col, bool ascending ) const
        {
            #define i static_cast<IfpMediaItem *>(i)
            switch( type() )
            {
                case MediaItem::DIRECTORY:
                    if( i->type() == MediaItem::DIRECTORY )
                        break;
                    return -1;

                default:
                    if( i->type() == MediaItem::DIRECTORY )
                        return 1;
            }
            #undef i

            return MediaItem::compare(i, col, ascending);
        }

    private:
        bool     m_dir;
        QCString m_encodedName;
};


/**
 * IfpMediaDevice Class
 */

IfpMediaDevice::IfpMediaDevice()
    : MediaDevice()
    , m_dev( 0 )
    , m_dh( 0 )
    , m_connected( false )
    , m_last( 0 )
    , m_tmpParent( 0 )
    , m_td( 0 )
{
    m_name = "iRiver";
    m_hasMountPoint = false;

    m_spacesToUnderscores = configBool("spacesToUnderscores");
    m_firstSort           = configString( "firstGrouping", i18n("None") );
    m_secondSort          = configString( "secondGrouping", i18n("None") );
    m_thirdSort           = configString( "thirdGrouping", i18n("None") );
}

void
IfpMediaDevice::init( MediaBrowser* parent )
{
    MediaDevice::init( parent );
}

IfpMediaDevice::~IfpMediaDevice()
{
    setConfigString( "firstGrouping"    , m_firstSort );
    setConfigString( "secondGrouping"   , m_secondSort );
    setConfigString( "thirdGrouping"    , m_thirdSort );
    setConfigBool( "spacesToUnderscores", m_spacesToUnderscores );

    closeDevice();
}

bool
IfpMediaDevice::checkResult( int result, QString message )
{
    if( result == 0 )
        return true;

    error() << result << ": " << message << endl;
    return false;
}


bool
IfpMediaDevice::openDevice( bool /*silent*/ )
{
    DEBUG_BLOCK

    usb_init();

    m_dh = (usb_dev_handle*)ifp_find_device();

    QString genericError = i18n( "Could not connect to iFP device" );

    if( m_dh == NULL )
    {
        error() << "A suitable iRiver iFP device couldn't be found" << endl;
        Amarok::StatusBar::instance()->shortLongMessage( genericError,
                                        i18n("iFP: A suitable iRiver iFP device could not be found")
                                        , KDE::StatusBar::Error );
        return false;
    }

    m_dev = usb_device( m_dh );
    if( m_dev == NULL )
    {
        error() << "Could not get usb_device()" << endl;
        Amarok::StatusBar::instance()->shortLongMessage( genericError,
                                        i18n("iFP: Could not get a USB device handle"), KDE::StatusBar::Error );
        if( ifp_release_device( m_dh ) )
            error() << "warning: release_device failed." << endl;
        return false;
    }

    /* "must be called" written in the libusb documentation */
    if( usb_claim_interface( m_dh, m_dev->config->interface->altsetting->bInterfaceNumber ) )
    {
        error() << "Device is busy.  (I was unable to claim its interface.)" << endl;
        Amarok::StatusBar::instance()->shortLongMessage( genericError,
                                        i18n("iFP: Device is busy"), KDE::StatusBar::Error );
        if( ifp_release_device( m_dh ) )
            error() << "warning: release_device failed." << endl;
        return false;
    }

    int i = ifp_init( &m_ifpdev, m_dh );
    if( i )
    {
        error() << "iFP device: Device cannot be opened." << endl;
        Amarok::StatusBar::instance()->shortLongMessage( genericError,
                                        i18n("iFP: Could not open device"), KDE::StatusBar::Error );
        usb_release_interface( m_dh, m_dev->config->interface->altsetting->bInterfaceNumber );
        return false;
    }

    m_connected = true;

    char info[20];
    ifp_model( &m_ifpdev, info, 20 );
    m_transferDir = QString(info);
    debug() << "Successfully connected to: " << info << endl;

    listDir( "" );

    return true;
}

bool
IfpMediaDevice::closeDevice()  //SLOT
{
    DEBUG_BLOCK

    if( m_connected )
    {
        if( m_dh )
        {
            usb_release_interface( m_dh, m_dev->config->interface->altsetting->bInterfaceNumber );

            if( ifp_release_device( m_dh ) )
                error() << "warning: release_device failed." << endl;

            ifp_finalize( &m_ifpdev );
            m_dh = 0;
        }

        m_view->clear();

        m_connected = false;
    }

    return true;
}

void
IfpMediaDevice::runTransferDialog()
{
    m_td = new TransferDialog( this );
    m_td->exec();
}

/// Renaming

void
IfpMediaDevice::renameItem( QListViewItem *item ) // SLOT
{
    if( !item )
        return;

    #define item static_cast<IfpMediaItem*>(item)

    QCString src  = QFile::encodeName( getFullPath( item, false ) );
    src.append( item->encodedName() );

     //the rename line edit has already changed the QListViewItem text
    QCString dest = QFile::encodeName( getFullPath( item ) );

    debug() << "Renaming " << src << " to: " << dest << endl;

    if( ifp_rename( &m_ifpdev, src, dest ) ) //success == 0
        //rename failed
        item->setText( 0, item->encodedName() );

    #undef item
}

/// Creating a directory

MediaItem *
IfpMediaDevice::newDirectory( const QString &name, MediaItem *parent )
{
    if( !m_connected || name.isEmpty() ) return 0;

    QString cleanedName = cleanPath( name );

    const QCString dirPath = QFile::encodeName( getFullPath( parent ) + "\\" + cleanedName );
    debug() << "Creating directory: " << dirPath << endl;
    int err = ifp_mkdir( &m_ifpdev, dirPath );

    if( err ) //failed
        return 0;

    m_tmpParent = parent;
    addTrackToList( IFP_DIR, cleanedName );
    return m_last;
}

MediaItem *
IfpMediaDevice::newDirectoryRecursive( const QString &name, MediaItem *parent )
{
    QStringList folders = QStringList::split( '\\', name );
    QString progress = "";

    if( parent )
        progress += getFullPath( parent ) + "\\";
    else
        progress += "\\";

    foreach( folders )
    {
        debug() << "Checking folder: " << progress << endl;
        progress += *it;
        const QCString dirPath = QFile::encodeName( progress );

        if( ifp_exists( &m_ifpdev, dirPath ) == IFP_DIR )
        {
            m_tmpParent = parent;
            parent = findChildItem( *it, parent );
            if( !parent )
            {
                addTrackToList( IFP_DIR, *it );
                parent = m_last;
            }
        }
        else
        {
            parent = newDirectory( *it, parent );
            if( !parent ) //failed
                return 0;
        }
        progress += "\\";
    }
    return parent;
}

MediaItem *
IfpMediaDevice::findChildItem( const QString &name, MediaItem *parent )
{
    QListViewItem *child;

    parent ?
        child = parent->firstChild():
        child = m_view->firstChild();

    while( child )
    {
        if( child->text(0) == name )
            return static_cast<MediaItem*>(child);
        child = child->nextSibling();
    }
    return 0;
}

void
IfpMediaDevice::addToDirectory( MediaItem *directory, QPtrList<MediaItem> items )
{
    if( !directory || items.isEmpty() ) return;

    m_tmpParent = directory;
    for( QPtrListIterator<MediaItem> it(items); *it; ++it )
    {
        QCString src  = QFile::encodeName( getFullPath( *it ) );
        QCString dest = QFile::encodeName( getFullPath( directory ) + "\\" + (*it)->text(0) );
        debug() << "Moving: " << src << " to: " << dest << endl;

        int err = ifp_rename( &m_ifpdev, src, dest );
        if( err ) //failed
            continue;

        m_view->takeItem( *it );
        directory->insertItem( *it );
    }
}

/// Uploading

MediaItem *
IfpMediaDevice::copyTrackToDevice( const MetaBundle& bundle )
{
    if( !m_connected ) return 0;
    m_transferring = true;

    const QCString src  = QFile::encodeName( bundle.url().path() );

    QString directory = "\\"; //root
    bool cleverFilename = false;
    bool addFileToView = true;
    if( m_firstSort != i18n("None") )
    {
        addFileToView = false;
        directory += bundle.prettyText( bundle.columnIndex(m_firstSort) ) + "\\";

        if( m_secondSort != i18n("None") )
        {
            directory += bundle.prettyText( bundle.columnIndex(m_secondSort) ) + "\\";

            if( m_thirdSort != i18n("None") )
                directory += bundle.prettyText( bundle.columnIndex(m_thirdSort) ) + "\\";
        }
        if( m_firstSort == i18n("Album") || m_secondSort == i18n("Album") || m_thirdSort == i18n("Album") )
            cleverFilename = true;
    }

    m_tmpParent = newDirectoryRecursive( directory, 0 ); // recursively create folders as required.

    QString newFilename;
    // we don't put this in cleanPath because of remote directory delimiters
    const QString title = bundle.title().replace( '\\', '-' );
    if( cleverFilename && !title.isEmpty() )
    {
        if( bundle.track() > 0 )
            newFilename = cleanPath( QString::number(bundle.track()) + " - " + title ) + '.' + bundle.type();
        else
            newFilename = cleanPath( title ) + '.' + bundle.type();
    }
    else
        newFilename = cleanPath( bundle.prettyTitle() ) + '.' + bundle.type();

    const QCString dest = QFile::encodeName( cleanPath(directory + newFilename) );

    kapp->processEvents( 100 );
    int result = uploadTrack( src, dest );

    if( !result ) //success
    {
        addTrackToList( IFP_FILE, cleanPath( newFilename ) );
        return m_last;
    }
    return 0;
}

/// File transfer methods

int
IfpMediaDevice::uploadTrack( const QCString& src, const QCString& dest )
{
    debug() << "Transferring " << src << " to: " << dest << endl;

    return ifp_upload_file( &m_ifpdev, src, dest, filetransferCallback, this );
}

int
IfpMediaDevice::downloadTrack( const QCString& src, const QCString& dest )
{
    debug() << "Downloading " << src << " to: " << dest << endl;

    return ifp_download_file( &m_ifpdev, src, dest, filetransferCallback, this );
}

void
IfpMediaDevice::downloadSelectedItems()
{
//     KConfig *config = Amarok::config( "MediaDevice" );
//     QString save = config->readEntry( "DownloadLocation", QString::null );  //restore the save directory
    QString save = QString::null;

    KURLRequesterDlg dialog( save, 0, 0 );
    dialog.setCaption( kapp->makeStdCaption( i18n( "Choose a Download Directory" ) ) );
    dialog.urlRequester()->setMode( KFile::Directory | KFile::ExistingOnly );
    dialog.exec();

    KURL destDir = dialog.selectedURL();
    if( destDir.isEmpty() )
        return;

    destDir.adjustPath( 1 ); //add trailing slash

//     if( save != destDir.path() )
//         config->writeEntry( "DownloadLocation", destDir.path() );

    QListViewItemIterator it( m_view, QListViewItemIterator::Selected );
    for( ; it.current(); ++it )
    {
        QCString dest = QFile::encodeName( destDir.path() + (*it)->text(0) );
        QCString src = QFile::encodeName( getFullPath( *it ) );

        downloadTrack( src, dest );
    }

    hideProgress();
}

int
IfpMediaDevice::filetransferCallback( void *pData, struct ifp_transfer_status *progress )
{
    // will be called by 'ifp_upload_file' by callback

    kapp->processEvents( 100 );

    IfpMediaDevice *that = static_cast<IfpMediaDevice *>(pData);

    if( that->isCanceled() )
    {
        debug() << "Canceling transfer operation" << endl;
        that->setCanceled( false );
        that->setProgress( progress->file_bytes, progress->file_bytes );
        return 1; //see ifp docs, return 1 for user cancel request
    }

    return that->setProgressInfo( progress );
}
int
IfpMediaDevice::setProgressInfo( struct ifp_transfer_status *progress )
{
    setProgress( progress->file_bytes, progress->file_total );
    return 0;
}


/// Deleting

int
IfpMediaDevice::deleteItemFromDevice( MediaItem *item, int /*flags*/ )
{
    if( !item || !m_connected ) return -1;

    QString path = getFullPath( item );

    QCString encodedPath = QFile::encodeName( path );
    int err;
    int count = 0;

    switch( item->type() )
    {
        case MediaItem::DIRECTORY:
            err = ifp_delete_dir_recursive( &m_ifpdev, encodedPath );
            debug() << "Deleting folder: " << encodedPath << endl;
            checkResult( err, i18n("Directory cannot be deleted: '%1'").arg(encodedPath) );
            break;

        default:
            err = ifp_delete( &m_ifpdev, encodedPath );
            debug() << "Deleting file: " << encodedPath << endl;
            count += 1;
            checkResult( err, i18n("File does not exist: '%1'").arg(encodedPath) );
            break;
    }
    if( err == 0 ) //success
        delete item;

    return (err == 0) ? count : -1;
}

/// Directory Reading

void
IfpMediaDevice::expandItem( QListViewItem *item ) // SLOT
{
    if( !item || !item->isExpandable() || m_transferring ) return;

    while( item->firstChild() )
        delete item->firstChild();

    m_tmpParent = item;

    QString path = getFullPath( item );
    listDir( path );

    m_tmpParent = 0;
}

void
IfpMediaDevice::listDir( const QString &dir )
{
    int err = ifp_list_dirs( &m_ifpdev, QFile::encodeName( dir ), listDirCallback, this );
    checkResult( err, i18n("Cannot enter directory: '%1'").arg(dir) );
}

// will be called by 'ifp_list_dirs'
int
IfpMediaDevice::listDirCallback( void *pData, int type, const char *name, int size )
{
    QString qName = QFile::decodeName( name );
    return static_cast<IfpMediaDevice *>(pData)->addTrackToList( type, qName, size );
}

int
IfpMediaDevice::addTrackToList( int type, QString name, int /*size*/ )
{
    m_tmpParent ?
            m_last = new IfpMediaItem( m_tmpParent ):
            m_last = new IfpMediaItem( m_view );

    if( type == IFP_DIR ) //directory
        m_last->setType( MediaItem::DIRECTORY );

    else if( type == IFP_FILE ) //file
    {
        if( name.endsWith( "mp3", false ) || name.endsWith( "wma", false ) ||
            name.endsWith( "wav", false ) || name.endsWith( "ogg", false ) ||
            name.endsWith( "asf", false ) )

            m_last->setType( MediaItem::TRACK );

        else
            m_last->setType( MediaItem::UNKNOWN );
    }
    m_last->setEncodedName( name );
    m_last->setText( 0, name );
    return 0;
}

/// Capacity, in kB

bool
IfpMediaDevice::getCapacity( KIO::filesize_t *total, KIO::filesize_t *available )
{
    if( !m_connected ) return false;

    int totalBytes = ifp_capacity( &m_ifpdev );
    int freeBytes = ifp_freespace( &m_ifpdev );

    *total = totalBytes;
    *available = freeBytes;

    return totalBytes > 0;
}

/// Helper functions

QString
IfpMediaDevice::getFullPath( const QListViewItem *item, const bool getFilename )
{
    if( !item ) return QString();

    QString path;

    if( getFilename ) path = item->text(0);

    QListViewItem *parent = item->parent();
    while( parent )
    {
        path.prepend( "\\" );
        path.prepend( parent->text(0) );
        parent = parent->parent();
    }
    path.prepend( "\\" );

    return path;
}


void
IfpMediaDevice::rmbPressed( QListViewItem* qitem, const QPoint& point, int )
{
    enum Actions { DOWNLOAD, DIRECTORY, RENAME, DELETE };

    MediaItem *item = static_cast<MediaItem *>(qitem);
    if ( item )
    {
        KPopupMenu menu( m_view );
        menu.insertItem( SmallIconSet( Amarok::icon( "collection" ) ), i18n( "Download" ), DOWNLOAD );
        menu.insertSeparator();
        menu.insertItem( SmallIconSet( Amarok::icon( "folder" ) ), i18n("Add Directory" ), DIRECTORY );
        menu.insertItem( SmallIconSet( Amarok::icon( "edit" ) ), i18n( "Rename" ), RENAME );
        menu.insertItem( SmallIconSet( Amarok::icon( "remove" ) ), i18n( "Delete" ), DELETE );

        int id =  menu.exec( point );
        switch( id )
        {
            case DOWNLOAD:
                downloadSelectedItems();
                break;

            case DIRECTORY:
                if( item->type() == MediaItem::DIRECTORY )
                    m_view->newDirectory( static_cast<MediaItem*>(item) );
                else
                    m_view->newDirectory( static_cast<MediaItem*>(item->parent()) );
                break;

            case RENAME:
                m_view->rename( item, 0 );
                break;

            case DELETE:
                deleteFromDevice();
                break;
        }
        return;
    }

    if( isConnected() )
    {
        KPopupMenu menu( m_view );
        menu.insertItem( SmallIconSet( Amarok::icon( "folder" ) ), i18n("Add Directory" ), DIRECTORY );
        int id =  menu.exec( point );
        switch( id )
        {
            case DIRECTORY:
                m_view->newDirectory( 0 );
                break;
        }
    }
}

QString IfpMediaDevice::cleanPath( const QString &component )
{
    QString result = Amarok::asciiPath( component );

    result.simplifyWhiteSpace();

    result.remove( "?" ).replace( "*", " " ).replace( ":", " " );

//     if( m_spacesToUnderscores )
//         result.replace( QRegExp( "\\s" ), "_" );

    result.replace( "/", "-" );

    return result;
}

#include "ifpmediadevice.moc"
