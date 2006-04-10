/****************************************************************************
 * copyright            :(C) 2005 Jeff Mitchell <kde-dev@emailgoeshere.com> *
 *                       (C) 2005 Seb Ruiz <me@sebruiz.net>                 *
 *                                                                          *
 * With some code helpers from KIO_VFAT                                     *
 *                        (c) 2004 Thomas Loeber <vfat@loeber1.de>          *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#define DEBUG_PREFIX "VfatMediaDevice"

#include "vfatmediadevice.h"

AMAROK_EXPORT_PLUGIN( VfatMediaDevice )

#include "debug.h"
#include "medium.h"
#include "metabundle.h"
#include "collectiondb.h"
#include "collectionbrowser.h"
#include "statusbar/statusbar.h"
#include "transferdialog.h"

#include <kapplication.h>
#include <kconfig.h>           //download saveLocation
#include <kdiskfreesp.h>
#include <kiconloader.h>       //smallIcon
#include <kio/job.h>
#include <kio/jobclasses.h>
#include <kio/netaccess.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <kurlrequester.h>     //downloadSelectedItems()
#include <kurlrequesterdlg.h>  //downloadSelectedItems()

#include <taglib/audioproperties.h>

#include <unistd.h>            //usleep()

#include <qcstring.h>
#include <qfile.h>
#include <qstringx.h>

namespace amaroK {
    extern KConfig *config( const QString& );
    extern QString cleanPath( const QString&, bool );
}

/**
 * VfatMediaItem Class
 */

class VfatMediaItem : public MediaItem
{
    public:
        VfatMediaItem( QListView *parent, QListViewItem *after = 0 )
            : MediaItem( parent, after )
        { }

        VfatMediaItem( QListViewItem *parent, QListViewItem *after = 0 )
            : MediaItem( parent, after )
        { }

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
            #define i static_cast<VfatMediaItem *>(i)
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
 * VfatMediaDevice Class
 */

VfatMediaDevice::VfatMediaDevice()
    : MediaDevice()
    , m_connected( false )
    , m_tmpParent( 0 )
    , m_kBSize( 0 )
    , m_kBAvail( 0 )
{
    m_name = "VFAT Device";
    m_td = NULL;
    m_dirLister = new KDirLister();
    m_dirLister->setNameFilter( "*.mp3 *.wav *.asf *.flac *.wma *.ogg" );
    m_dirLister->setAutoUpdate( false );
    m_spacesToUnderscores = false;
    m_isInCopyTrack = false;
    m_stopDirLister = false;
    m_firstSort = "None";
    m_secondSort = "None";
    m_thirdSort = "None";
    connect( m_dirLister, SIGNAL( newItems(const KFileItemList &) ), this, SLOT( newItems(const KFileItemList &) ) );
    connect( m_dirLister, SIGNAL( completed() ), this, SLOT( dirListerCompleted() ) );
}

void
VfatMediaDevice::init( MediaBrowser* parent )
{
    MediaDevice::init( parent );
}

VfatMediaDevice::~VfatMediaDevice()
{
    closeDevice();
}

bool
VfatMediaDevice::checkResult( int result, QString message )
{
    if( result == 0 )
        return true;

    error() << result << ": " << message << endl;
    return false;
}


bool
VfatMediaDevice::openDevice( bool /*silent*/ )
{
    if( !m_medium )
    {
        amaroK::StatusBar::instance()->longMessage( i18n( "VFAT Devices cannot be manually configured.  Ensure DBUS and HAL are running\n"
                                                          "and your kioslaves were built with DBUS and HAL support.  The device should be\n"
                                                          "autodetected; click the \"Manage Plugins...\" suboption of the Configure button\n"
                                                          "in the Media Device tab and choose the VFAT plugin for the detected device.  Then\n"
                                                          "ensure the device is mounted and click \"Connect\" again." ),
                                                    KDE::StatusBar::Sorry );
        return false;
    }
    if( !m_medium->mountPoint() )
    {
        amaroK::StatusBar::instance()->longMessage( i18n( "Devices handled by this plugin must be mounted first.\n"
                                                          "Please mount the device and click \"Connect\" again." ),
                                                    KDE::StatusBar::Sorry );
        return false;
    }
    m_actuallyVfat = m_medium->fsType() == "vfat" ? true : false;
    m_connected = true;
    m_transferDir = m_medium->mountPoint();
    listDir( m_medium->mountPoint() );
    connect( this, SIGNAL( startTransfer() ), MediaBrowser::instance(), SLOT( transferClicked() ) );
    return true;
}

bool
VfatMediaDevice::closeDevice()  //SLOT
{
    if( m_connected )
    {
        m_view->clear();

        m_connected = false;
    }

    return true;
}

void
VfatMediaDevice::runTransferDialog()
{
    m_td = new TransferDialog( this );
    m_td->exec();
}

/// Renaming

void
VfatMediaDevice::renameItem( QListViewItem *item ) // SLOT
{

    if( !item )
        return;

    #define item static_cast<VfatMediaItem*>(item)

    QCString src  = QFile::encodeName( getFullPath( item, false ) );
    src.append( item->encodedName() );

     //the rename line edit has already changed the QListViewItem text
    QCString dest = QFile::encodeName( getFullPath( item ) );

    debug() << "Renaming: " << src << " to: " << dest << endl;

    //TODO: do we want a progress dialog?  If so, set last false to true
    KIO::NetAccess::file_move( KURL(src), KURL(dest), -1, false, false, false );

    #undef item

}

/// Creating a directory

MediaItem *
VfatMediaDevice::newDirectory( const QString &name, MediaItem *parent )
{
    DEBUG_BLOCK
    if( !m_connected || name.isEmpty() ) return 0;

    debug() << "newDirectory called with name = " << name << ", and parent = " << parent << endl;

    bool equal = name.startsWith( m_medium->mountPoint(), true );

    debug() << "name.startsWith(m_medium->mountPoint() = " << (equal ? "true" : "false") << endl;

    QString fullPath = getFullPath( parent, true, ( equal ? false : true ), false );

    if( fullPath == QString::null )
        fullPath += m_medium->mountPoint();

    debug() << "fullPath = " << fullPath << endl;

    QCString dirPath;

    QString cleanedName = cleanPath(name);

    if( equal )
        dirPath = QFile::encodeName( cleanedName );
    else
        dirPath = QFile::encodeName( fullPath + "/" + cleanedName );

    debug() << "Creating directory: " << dirPath << endl;

    const KURL url( dirPath );

    if( ! KIO::NetAccess::mkdir( url, m_parent ) ) //failed
    {
        debug() << "Failed to create directory " << dirPath << endl;
        return NULL;
    }

    if( m_isInCopyTrack )
        addTrackToList( MediaItem::DIRECTORY, cleanedName );

    return m_last;

}

void
VfatMediaDevice::addToDirectory( MediaItem *directory, QPtrList<MediaItem> items )
{
    DEBUG_BLOCK
    if( !directory || items.isEmpty() ) return;

    MediaItem *previousTmpParent = static_cast<MediaItem *>(m_tmpParent);

    m_stopDirLister = true;
    m_tmpParent = directory;
    for( QPtrListIterator<MediaItem> it(items); *it; ++it )
    {
        QCString src  = QFile::encodeName( getFullPath( *it, true, true, false ) );
        QCString dest = QFile::encodeName( getFullPath( directory, true, true, false ) + "/" + (*it)->text(0) );
        debug() << "Moving: " << src << " to: " << dest << endl;

        const KURL srcurl(src);
        const KURL desturl(dest);

        if ( !KIO::NetAccess::file_move( srcurl, desturl, -1, false, false, m_parent ) )
            debug() << "Failed moving " << src << " to " << dest << endl;
        else
        {
            addTrackToList( (*it)->type(), (*it)->text(0) );
            delete *it;
        }
    }
    m_stopDirLister = false;
    m_tmpParent = previousTmpParent;
}

/// Uploading

void
VfatMediaDevice::copyTrackSortHelper( const MetaBundle& bundle, QString& sort, QString& temp, QString& base )
{
    QListViewItem *it;
    if( sort != "None" )
    {
        debug() << "sort = " << sort << endl;
        temp = bundle.prettyText( bundle.columnIndex(sort) );
        temp = ( temp == QString::null ? "Unknown" : cleanPath(temp) );
        base += temp + "/";

        if( !KIO::NetAccess::stat( KURL(base), m_udsentry, m_parent ) )
            m_tmpParent = static_cast<MediaItem *>(newDirectory( temp, static_cast<MediaItem*>(m_tmpParent) ));
        else
        {
            debug() << "m_tmpParent (firstSort) " << m_tmpParent << endl;
            if( m_tmpParent)
                it = m_tmpParent->firstChild();
            else
                it = m_view->firstChild();
            while( it && it->text(0) != temp )
            {
                it = it->nextSibling();
                debug() << "Looking for next in firstSort, temp = " << temp << ", text(0) = " << it->text(0) << endl;
            }
            m_tmpParent = static_cast<MediaItem *>( it );
        }
    }
}


MediaItem *
VfatMediaDevice::copyTrackToDevice( const MetaBundle& bundle )
{
    DEBUG_BLOCK
    debug() << "dirlister autoupdate = " << (m_dirLister->autoUpdate() ? "true" : "false") << endl;
    if( !m_connected ) return 0;

    m_isInCopyTrack = true;

    //TODO: dirlister's autoupdate should be off, but it's running, setting m_tmpParent to null and generally fucking everything up
    //what to do?  not have it return null?  figure out how to actually turn the dirlister's autoupdate off?

    debug() << "m_tmpParent = " << m_tmpParent << endl;
    MediaItem *previousTmpParent = static_cast<MediaItem *>(m_tmpParent);

    QString  newFilenameSansMountpoint = cleanPath( bundle.prettyTitle().remove( "'" ) + "." + bundle.type() );
    QString  base = m_transferDir + "/";
    QString  temp;

    copyTrackSortHelper( bundle, m_firstSort, temp, base);
    copyTrackSortHelper( bundle, m_secondSort, temp, base);
    copyTrackSortHelper( bundle, m_thirdSort, temp, base);

    QString  newFilename = base + newFilenameSansMountpoint;

    const QCString dest = QFile::encodeName( newFilename ); // TODO: add to directory
    const KURL desturl = KURL::fromPathOrURL( dest );

    kapp->processEvents( 100 );

    if( KIO::NetAccess::file_copy( bundle.url(), desturl, -1, false, false, m_parent) ) //success
    {
        addTrackToList( MediaItem::TRACK, newFilenameSansMountpoint );
        m_tmpParent = previousTmpParent;
        m_isInCopyTrack = false;
        return m_last;
    }

    m_tmpParent = previousTmpParent;
    m_isInCopyTrack = false;
    return 0;
}

//Somewhat related...

MediaItem *
VfatMediaDevice::trackExists( const MetaBundle& bundle )
{
    QString  newFilenameSansMountpoint = cleanPath( bundle.prettyTitle().remove( "'" ) + "." + bundle.type() );
    QString  base = m_transferDir + "/";
    QString  temp;

    debug() << "m_firstSort = " << m_firstSort << endl;
    if( m_firstSort != "None")
    {
        temp = bundle.prettyText( bundle.columnIndex(m_firstSort) );
        base += cleanPath( ( temp == QString::null ? "Unknown" : temp ) ) + "/";
    }

    debug() << "m_secondSort = " << m_secondSort << endl;
    if( m_secondSort != "None")
    {
        temp = bundle.prettyText( bundle.columnIndex(m_secondSort) );
        base += cleanPath( ( temp == QString::null ? "Unknown" : temp ) ) + "/";
    }

    debug() << "m_thirdSort = " << m_thirdSort << endl;
    if( m_thirdSort != "None")
    {
        temp = bundle.prettyText( bundle.columnIndex(m_thirdSort) );
        base += cleanPath( ( temp == QString::null ? "Unknown" : temp ) ) + "/";
    }

    QString  newFilename = base + newFilenameSansMountpoint;

    if ( KIO::NetAccess::stat( KURL(newFilename), m_udsentry, m_parent ) )
    {
        QListViewItemIterator it( m_view );
        while ( it.current() )
        {
            if ( (*it)->text( 0 ) == newFilenameSansMountpoint )
                return static_cast<MediaItem *>(it.current());
            ++it;
        }
    }
    return 0;
}

/// File transfer methods


void
VfatMediaDevice::downloadSelectedItems()
{
    while ( !m_downloadList.empty() )
        m_downloadList.pop_front();
    QListViewItemIterator it( m_view, QListViewItemIterator::Selected );
    MediaItem *curritem;
    for( ; it.current(); ++it )
    {
        curritem = static_cast<MediaItem *>(*it);
        if( curritem->type() == MediaItem::DIRECTORY )
            drillDown( curritem );
        else //file
            m_downloadList.append( KURL( getFullPath( curritem ) ) );
    }

    //here is where to call the dialog...maybe test first by printing out the entries
    KURL::List::iterator kit;
    //for( kit = m_downloadList.begin(); kit != m_downloadList.end(); ++kit)
    //    debug() << "Going to download: " << (*kit).path() << endl;

    CollectionView::instance()->organizeFiles( m_downloadList, "Download Files to Collection", true );

    hideProgress();
}

void
VfatMediaDevice::drillDown( MediaItem *curritem )
{
    //okay, can recursively call this for directories...
    m_downloadListerFinished  = 0;
    int count = 0;
    m_currentJobUrl = KURL( getFullPath( curritem ) );
    KIO::ListJob * listjob = KIO::listRecursive( m_currentJobUrl, false, false );
    connect( listjob, SIGNAL( result( KIO::Job* ) ), this, SLOT( downloadSlotResult( KIO::Job* ) ) );
    connect( listjob, SIGNAL( entries( KIO::Job*, const KIO::UDSEntryList& ) ), this, SLOT( downloadSlotEntries( KIO::Job*, const KIO::UDSEntryList& ) ) );
    connect( listjob, SIGNAL( redirection( KIO::Job*, const KURL& ) ), this, SLOT( downloadSlotRedirection( KIO::Job*, const KURL& ) ) );
    while( !m_downloadListerFinished ){
        usleep( 10000 );
        kapp->processEvents( 100 );
        count++;
        if (count > 120){
            debug() << "Taking too long to find files, returning from drillDown in " << m_currentJobUrl << endl;
            return;
        }
    }
}

void
VfatMediaDevice::downloadSlotResult( KIO::Job *job )
{
    if( job->error() )
        debug() << "downloadSlotResult: ListJob reported an error!  Error code = " << job->error() << endl;
    m_downloadListerFinished = true;
}

void
VfatMediaDevice::downloadSlotRedirection( KIO::Job */*job*/, const KURL &url )
{
    m_currentJobUrl = url;
}

void
VfatMediaDevice::downloadSlotEntries(KIO::Job */*job*/, const KIO::UDSEntryList &entries)
{
        KIO::UDSEntryListConstIterator it = entries.begin();
        KIO::UDSEntryListConstIterator end = entries.end();

        for (; it != end; ++it)
        {
                KFileItem file(*it, m_currentJobUrl, false /* no mimetype detection */, true);
                if (!file.isDir())
                        m_downloadList.append(KURL( file.url().path() ) );
        }
}

/// Deleting

int
VfatMediaDevice::deleteItemFromDevice( MediaItem *item, bool /*onlyPlayed*/ )
{
    if( !item || !m_connected ) return -1;

    QString path = getFullPath( item );

    QCString encodedPath = QFile::encodeName( path );
    debug() << "Deleting file: " << encodedPath << endl;
    bool flag = true;
    int count = 0;

    switch( item->type() )
    {
        case MediaItem::DIRECTORY:
            if ( !KIO::NetAccess::del( KURL(encodedPath), m_parent ))
            {
                debug() << "Error deleting directory: " << encodedPath << endl;
                flag = false;
            }
            count++;
            break;

        default:
            if ( !KIO::NetAccess::del( KURL(encodedPath), m_parent ))
            {
                debug() << "Error deleting file: " << encodedPath << endl;
                flag = false;
            }
            count++;
            break;
    }
    if( flag ) //success
        delete item;

    return flag ? count : -1;
}

/// Directory Reading

void
VfatMediaDevice::expandItem( QListViewItem *item ) // SLOT
{
    DEBUG_BLOCK
    if( !item || !item->isExpandable() ) return;

    while( item->firstChild() )
        delete item->firstChild();

    m_tmpParent = item;

    QString path = getFullPath( item );
    listDir( path );
}

void
VfatMediaDevice::listDir( const QString &dir )
{
    DEBUG_BLOCK
    if ( m_dirLister->openURL( KURL(dir), true, true ) )
    {
        //debug() << "Waiting for KDirLister, do anything here?" << endl;
    }
    else
    {
        debug() << "KDirLister::openURL FAILED" << endl;
    }

    /*
        for each entry in directory
        addTrackToList( type (MediaItem::DIRECTORY/TRACK), name, size (not used, can be 0))

        ...handled by newItems slot
    */
}

void
VfatMediaDevice::newItems( const KFileItemList &items )
{
    DEBUG_BLOCK
    //iterate over items, calling addTrackToList
    if( m_stopDirLister || m_isInCopyTrack )
        return;

    QPtrListIterator<KFileItem> it( items );
    KFileItem *kfi;
    while ( (kfi = it.current()) != 0 ) {
        ++it;
        addTrackToList( kfi->isFile() ? MediaItem::TRACK : MediaItem::DIRECTORY, kfi->name(), 0 );
    }
}

void
VfatMediaDevice::dirListerCompleted()
{
    DEBUG_BLOCK
    if( !m_stopDirLister && !m_isInCopyTrack)
        m_tmpParent = NULL;
}

int
VfatMediaDevice::addTrackToList( int type, QString name, int /*size*/ )
{
    DEBUG_BLOCK
    m_tmpParent ?
        m_last = new VfatMediaItem( m_tmpParent ):
        m_last = new VfatMediaItem( m_view );

    if( type == MediaItem::DIRECTORY ) //directory
        m_last->setType( MediaItem::DIRECTORY );

    //TODO: this logic could maybe be taken out later...or the dirlister shouldn't
    //filter, one or the other...depends if we want to allow viewing any files
    //or just update the list in the plugin as appropriate
    else if( type == MediaItem::TRACK ) //file
    {
        if( name.endsWith( "mp3", false ) || name.endsWith( "wma", false ) ||
            name.endsWith( "wav", false ) || name.endsWith( "ogg", false ) ||
            name.endsWith( "asf", false ) || name.endsWith( "flac", false ) )

            m_last->setType( MediaItem::TRACK );

        else
            m_last->setType( MediaItem::UNKNOWN );
    }
    m_last->setEncodedName( name );
    m_last->setText( 0, name );

    m_last->setBundle( new MetaBundle( KURL( getFullPath(m_last, true) ), true, TagLib::AudioProperties::Fast ) );

    return 0;
}

/// Capacity, in kB

bool
VfatMediaDevice::getCapacity( KIO::filesize_t *total, KIO::filesize_t *available )
{
    if( !m_connected ) return false;

    KDiskFreeSp* kdf = new KDiskFreeSp( m_parent, "vfat_kdf" );
    kdf->readDF( m_medium->mountPoint() );
    connect(kdf, SIGNAL(foundMountPoint( const QString &, unsigned long, unsigned long, unsigned long )),
                 SLOT(foundMountPoint( const QString &, unsigned long, unsigned long, unsigned long )));

    int count = 0;

    while( m_kBSize == 0 && m_kBAvail == 0){
        usleep( 10000 );
        kapp->processEvents( 100 );
        count++;
        if (count > 120){
            debug() << "KDiskFreeSp taking too long.  Returning false from getCapacity()" << endl;
            return false;
        }
    }

    *total = m_kBSize*1024;
    *available = m_kBAvail*1024;
    unsigned long localsize = m_kBSize;
    m_kBSize = 0;
    m_kBAvail = 0;

    return localsize > 0;
}

void
VfatMediaDevice::foundMountPoint( const QString & mountPoint, unsigned long kBSize, unsigned long /*kBUsed*/, unsigned long kBAvail )
{
    if ( mountPoint == m_medium->mountPoint() ){
        m_kBSize = kBSize;
        m_kBAvail = kBAvail;
    }
}

/// Helper functions

QString
VfatMediaDevice::getFullPath( const QListViewItem *item, const bool getFilename, const bool prependMount, const bool clean )
{
    //DEBUG_BLOCK
    if( !item ) return QString::null;

    QString path;

    if ( getFilename && clean )
        path = cleanPath(item->text(0));
    else if( getFilename )
        path = item->text(0);

    QListViewItem *parent = item->parent();

    while( parent )
    {
        path.prepend( "/" );
        path.prepend( ( clean ? cleanPath(parent->text(0)) : parent->text(0) ) );
        parent = parent->parent();
    }

    //debug() << "path before prependMount = " << path << endl;
    if( prependMount )
        path.prepend( m_medium->mountPoint() + "/" );

    //debug() << "path after prependMount = " << path << endl;

    return path;

}


void
VfatMediaDevice::rmbPressed( QListViewItem* qitem, const QPoint& point, int )
{
    enum Actions { DOWNLOAD, DIRECTORY, RENAME, DELETE, TRANSFER_HERE };

    MediaItem *item = static_cast<MediaItem *>(qitem);
    if ( item )
    {
        KPopupMenu menu( m_view );
        menu.insertItem( SmallIconSet( "down" ), i18n( "Download" ), DOWNLOAD );
        menu.insertSeparator();
        menu.insertItem( SmallIconSet( "folder" ), i18n("Add Directory" ), DIRECTORY );
        menu.insertItem( SmallIconSet( "editclear" ), i18n( "Rename" ), RENAME );
        menu.insertItem( SmallIconSet( "editdelete" ), i18n( "Delete" ), DELETE );
        if ( MediaBrowser::queue()->childCount())
        {
            menu.insertSeparator();
            menu.insertItem( SmallIconSet( "up" ), i18n(" Transfer queue to here..." ), TRANSFER_HERE );
        }

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

            case TRANSFER_HERE:
                m_tmpParent = item;
                if( item->type() == MediaItem::DIRECTORY )
                {
                    m_transferDir = getFullPath( item, true );
                }
                else
                {
                    m_transferDir = getFullPath( item, false );
                    if (m_transferDir != QString::null)
                        m_transferDir = m_transferDir.remove( m_transferDir.length() - 1, 1 );
                }
                emit startTransfer();
                break;
        }
        return;
    }

    if( isConnected() )
    {
        KPopupMenu menu( m_view );
        menu.insertItem( SmallIconSet( "folder" ), i18n("Add Directory" ), DIRECTORY );
        if ( MediaBrowser::queue()->childCount())
        {
            menu.insertSeparator();
            menu.insertItem( SmallIconSet( "up" ), i18n(" Transfer queue to here..." ), TRANSFER_HERE );
        }
        int id =  menu.exec( point );
        switch( id )
        {
            case DIRECTORY:
                m_view->newDirectory( 0 );
                break;

            case TRANSFER_HERE:
                m_transferDir = m_medium->mountPoint();
                m_tmpParent = NULL;
                emit startTransfer();
                break;

        }
    }
}


QString VfatMediaDevice::cleanPath( const QString &component )
{
    QString result = component;

    if( m_actuallyVfat )
    {
        result = amaroK::cleanPath( result, true /* replaces weird stuff by '_' */);
    }

    result.simplifyWhiteSpace();
    if( m_spacesToUnderscores )
        result.replace( QRegExp( "\\s" ), "_" );
    if( m_actuallyVfat )
        result.replace( "?", "_" ).replace( "\\", "_" ).replace( "*", "_" ).replace( ":", "_" ).replace( "\"", "" );

    result.replace( "/", "-" );

    return result;
}

#include "vfatmediadevice.moc"
