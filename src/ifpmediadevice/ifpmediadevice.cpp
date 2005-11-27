/***************************************************************************
 * copyright            : (C) 2005 Seb Ruiz <me@sebruiz.net>               *
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
  *  @see http://ifp-driver.sourceforge.net/libifp/docs/ifp_8h.html
  *  @note ifp uses a backslash '\' as a directory delimiter for _remote_ files
  */

#define DEBUG_PREFIX "IfpMediaDevice"

#include "ifpmediadevice.h"

#include "debug.h"
#include "metabundle.h"
#include "collectiondb.h"
#include "statusbar/statusbar.h"

#include <kapplication.h>
#include <kiconloader.h>       //smallIcon
#include <kmessagebox.h>

#include <qfile.h>
#include <qcstring.h>

/**
 * IfpMediaItem Class
 */

class IfpMediaItem : public MediaItem
{
    public:
        IfpMediaItem( QListView *parent, QListViewItem *after = 0 )
            : MediaItem( parent, after )
        { }

        IfpMediaItem( QListViewItem *parent, QListViewItem *after = 0 )
            : MediaItem( parent, after )
        { }

        void
        IfpMediaItem::setEncodedName( QString &name )
        {
            m_encodedName = QFile::encodeName( name );
        }

        void
        IfpMediaItem::setEncodedName( QCString &name ) { m_encodedName = name; }

        QCString
        IfpMediaItem::encodedName() { return m_encodedName; }

        // List directories first, always
        int
        IfpMediaItem::compare( QListViewItem *i, int col, bool ascending ) const
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

IfpMediaDevice::IfpMediaDevice( MediaDeviceView* parent, MediaDeviceList *listview )
    : MediaDevice( parent, listview )
    , m_dev( 0 )
    , m_dh( 0 )
    , m_connected( false )
    , m_tmpParent( 0 )
{
    DEBUG_BLOCK
}

IfpMediaDevice::~IfpMediaDevice()
{
    DEBUG_BLOCK

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
        amaroK::StatusBar::instance()->shortLongMessage( genericError,
                                        i18n("iFP: A suitable iRiver iFP device couldn't be found")
                                        , KDE::StatusBar::Error );
        return false;
    }

    m_dev = usb_device( m_dh );
    if( m_dev == NULL )
    {
        error() << "Could not get usb_device()" << endl;
        amaroK::StatusBar::instance()->shortLongMessage( genericError,
                                        i18n("iFP: Could not get a usb device handle"), KDE::StatusBar::Error );
        if( ifp_release_device( m_dh ) )
            error() << "warning: release_device failed." << endl;
        return false;
    }

    /* "must be called" written in the libusb documentation */
    if( usb_claim_interface( m_dh, m_dev->config->interface->altsetting->bInterfaceNumber ) )
    {
        error() << "Device is busy.  (I was unable to claim its interface.)" << endl;
        amaroK::StatusBar::instance()->shortLongMessage( genericError,
                                        i18n("iFP: Device is busy"), KDE::StatusBar::Error );
        if( ifp_release_device( m_dh ) )
            error() << "warning: release_device failed." << endl;
        return false;
    }

    int i = ifp_init( &m_ifpdev, m_dh );
    if( i )
    {
        error() << "IFP device: Device cannot be opened." << endl;
        amaroK::StatusBar::instance()->shortLongMessage( genericError,
                                        i18n("iFP: Could not open device"), KDE::StatusBar::Error );
        usb_release_interface( m_dh, m_dev->config->interface->altsetting->bInterfaceNumber );
        return false;
    }

    m_connected = true;

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

        m_listview->clear();

        m_connected = false;
    }

    return true;
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

    const QCString dirPath = QFile::encodeName( getFullPath( parent ) + "\\" + name );
    debug() << "Creating directory: " << dirPath << endl;
    int err = ifp_mkdir( &m_ifpdev, dirPath );

    if( err ) //failed
        return 0;

    m_tmpParent = parent;
    addTrackToList( IFP_DIR, name );
    return m_last;
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

        m_listview->takeItem( *it );
        directory->insertItem( *it );
    }
}

/// Uploading

MediaItem *
IfpMediaDevice::copyTrackToDevice( const MetaBundle& bundle, bool /*isPodcast*/ )
{
    if( !m_connected ) return 0;

    const QString  newFilename = bundle.prettyTitle() + "." + bundle.type();
    const QCString src  = QFile::encodeName( bundle.url().path() );
    const QCString dest = QFile::encodeName( "\\" + newFilename ); // TODO: add to directory

    kapp->processEvents( 100 );
    int result = uploadTrack( src, dest );

    if( !result ) //success
    {
        addTrackToList( IFP_FILE, newFilename );
        return m_last;
    }

    return 0;
}

int
IfpMediaDevice::uploadTrack( const QCString& src, const QCString& dest )
{
    debug() << "Transferring " << src << " to: " << dest << endl;

    return ifp_upload_file( &m_ifpdev, src, dest, uploadCallback, this );
}

int
IfpMediaDevice::uploadCallback( void *pData, struct ifp_transfer_status *progress )
{
    kapp->processEvents( 100 );
    // will be called by 'ifp_upload_file_with_callback'
    return static_cast<IfpMediaDevice *>(pData)->setProgressInfo( progress );
}

int
IfpMediaDevice::setProgressInfo( struct ifp_transfer_status *progress )
{
    setProgress( progress->file_bytes, progress->file_total );
    return 0;
}


/// Deleting

bool
IfpMediaDevice::deleteItemFromDevice( MediaItem *item, bool /*onlyPlayed*/ )
{
    if( !item || !m_connected ) return false;

    QString path = getFullPath( item );

    QCString encodedPath = QFile::encodeName( path );
    debug() << "Deleting file: " << encodedPath << endl;
    int err;

    switch( item->type() )
    {
        case MediaItem::DIRECTORY:
            err = ifp_delete_dir_recursive( &m_ifpdev, encodedPath );
            checkResult( err, i18n("Directory cannot be deleted: '%1'").arg(encodedPath) );
            break;

        default:
            err = ifp_delete( &m_ifpdev, encodedPath );
            checkResult( err, i18n("File does not exist: '%1'").arg(encodedPath) );
            break;
    }
    if( err == 0 ) //success
        delete item;

    return (err == 0);
}

/// Directory Reading

void
IfpMediaDevice::expandItem( QListViewItem *item ) // SLOT
{
    if( !item || !item->isExpandable() ) return;

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
    DEBUG_BLOCK

    debug() << "listing contents in: '" << dir << "'" << endl;

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
            m_last = new IfpMediaItem( m_listview );

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
IfpMediaDevice::getCapacity( unsigned long *total, unsigned long *available )
{
    if( !m_connected ) return false;

    int totalBytes = ifp_capacity( &m_ifpdev );
    int freeBytes = ifp_freespace( &m_ifpdev );

    *total = (unsigned long)totalBytes / 1024;
    *available = (unsigned long)freeBytes / 1024;

    return totalBytes > 0;
}

/// Helper functions

QString
IfpMediaDevice::getFullPath( const QListViewItem *item, const bool getFilename )
{
    if( !item ) return QString::null;

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


#include "ifpmediadevice.moc"
