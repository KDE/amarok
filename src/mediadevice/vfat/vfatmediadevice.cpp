/***************************************************************************
 * copyright            : (C) 2005 Seb Ruiz <me@sebruiz.net>               *
 *                                                                         *
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
#include "statusbar/statusbar.h"

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

namespace amaroK { extern KConfig *config( const QString& ); }

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
    m_transferDir = "";
    m_dirLister = new KDirLister();
    m_dirLister->setNameFilter( "*.mp3 *.wav *.asf *.flac *.wma *.ogg" );
    m_dirLister->setAutoUpdate( false );
    connect( m_dirLister, SIGNAL( newItems(const KFileItemList &) ), this, SLOT( newItems(const KFileItemList &) ) );
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
    DEBUG_BLOCK

    if ( !m_medium->mountPoint() )
    {
        amaroK::StatusBar::instance()->longMessage( i18n( "Devices handled by this plugin must be mounted first.\nPlease mount the device and click Connect again." ),
            KDE::StatusBar::Sorry );
        return false;
    }

    m_connected = true;

    listDir( m_medium->mountPoint() );

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

    debug() << "Renaming " << src << " to: " << dest << endl;

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

    QString fullPath = getFullPath( parent );
    const QCString dirPath = QFile::encodeName( fullPath == QString::null ? m_medium->mountPoint() + "/" + name : fullPath + "/" + name );
    debug() << "Creating directory: " << dirPath << endl;

    const KURL url( dirPath );

    if( ! KIO::NetAccess::mkdir( url, m_parent ) ) //failed
    {
        debug() << "Failed to create directory " << dirPath << endl;
        return NULL;
    }

    m_tmpParent = parent;
    return m_last;

}

void
VfatMediaDevice::addToDirectory( MediaItem *directory, QPtrList<MediaItem> items )
{
    DEBUG_BLOCK
    if( !directory || items.isEmpty() ) return;

    m_tmpParent = directory;
    for( QPtrListIterator<MediaItem> it(items); *it; ++it )
    {
        QCString src  = QFile::encodeName( getFullPath( *it ) );
        QCString dest = QFile::encodeName( getFullPath( directory ) + "/" + (*it)->text(0) );
        debug() << "Moving: " << src << " to: " << dest << endl;

        const KURL srcurl(src);
        const KURL desturl(dest);

        if ( KIO::NetAccess::file_move( srcurl, desturl, -1, false, false, m_parent ) )
            debug() << "Failed moving " << src << " to " << dest << endl;

        m_view->takeItem( *it );
        directory->insertItem( *it );
    }
}

/// Uploading

MediaItem *
VfatMediaDevice::copyTrackToDevice( const MetaBundle& bundle, const PodcastInfo* /*info*/ )
{
    DEBUG_BLOCK
    if( !m_connected ) return 0;

    const QString  newFilenameSansMountpoint = bundle.prettyTitle().remove( "'" ) + "." + bundle.type();
    const QString  newFilename = m_medium->mountPoint() + "/" + newFilenameSansMountpoint;
    const QCString src  = QFile::encodeName( bundle.url().path() );
    const QCString dest = QFile::encodeName( newFilename ); // TODO: add to directory

    const KURL srcurl(src);
    const KURL desturl(dest);

    kapp->processEvents( 100 );

    if( KIO::NetAccess::file_copy( srcurl, desturl, -1, false, false, m_parent) ) //success
    {
        addTrackToList( MediaItem::TRACK, newFilenameSansMountpoint );
        return m_last;
    }

    return 0;
}

//Somewhat related...

MediaItem *
VfatMediaDevice::trackExists( const MetaBundle& bundle )
{
    void *dummy;
    const QString  newFilenameSansMountpoint = bundle.prettyTitle().remove( "'" ) + "." + bundle.type();
    const QString  newFilename = m_medium->mountPoint() + "/" + newFilenameSansMountpoint;
    if ( KIO::NetAccess::stat( KURL(newFilename), m_udsentry, m_parent ) )
        return reinterpret_cast<MediaItem *>(dummy);

    return 0;
}

/// File transfer methods


void
VfatMediaDevice::downloadSelectedItems()
{
//     KConfig *config = amaroK::config( "MediaDevice" );
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

    KIO::CopyJob *result;

    QListViewItemIterator it( m_view, QListViewItemIterator::Selected );
    for( ; it.current(); ++it )
    {
        QCString dest = QFile::encodeName( destDir.path() + (*it)->text(0) );
        QCString src = QFile::encodeName( getFullPath( *it ) );

        const KURL srcurl(src);
        const KURL desturl(dest);
        //TODO: Error handling here?
        //TODO: Make async?  But where the hell is KIO::file_copy?
        result = KIO::copy( srcurl, desturl, true );
    }

    hideProgress();
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
    //m_tmpParent = 0;
}

void
VfatMediaDevice::listDir( const QString &dir )
{
    DEBUG_BLOCK
    if ( m_dirLister->openURL( KURL(dir), true, true ) )
    {
        debug() << "Waiting for KDirLister, do anything here?" << endl;
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
    QPtrListIterator<KFileItem> it( items );
    KFileItem *kfi;
    while ( (kfi = it.current()) != 0 ) {
        ++it;
        addTrackToList( kfi->isFile() ? MediaItem::TRACK : MediaItem::DIRECTORY, kfi->name(), 0 );
    }

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
VfatMediaDevice::getCapacity( unsigned long *total, unsigned long *available )
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
        if (count % 30 == 0){
            debug() << "KDiskFreeSp taking a long time, perhaps something went wrong?" << endl;
        }
        if (count > 120){
            debug() << "KDiskFreeSp taking too long.  Returning false from getCapacity()" << endl;
            return false;
        }
    }

    *total = m_kBSize;
    *available = m_kBAvail;
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
VfatMediaDevice::getFullPath( const QListViewItem *item, const bool getFilename )
{
    DEBUG_BLOCK
    if( !item ) return QString::null;

    debug() << "Not returning QString::null" << endl;

    QString path;

    if ( getFilename ) path = item->text(0);

    QListViewItem *parent = item->parent();

    while( parent )
    {
        path.prepend( "/" );
        path.prepend( parent->text(0) );
        parent = parent->parent();
    }
    path.prepend( m_medium->mountPoint() + "/" );

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
        menu.insertSeparator();
        menu.insertItem( SmallIconSet( "up" ), i18n(" Transfer queue to here..." ), TRANSFER_HERE );

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
                if( item->type() == MediaItem::DIRECTORY )
                    m_transferDir = getFullPath( item, true ) + "/";
                else
                    m_transferDir = getFullPath( item, false );
                debug() << "New transfer dir is: " << m_transferDir << endl;
                //start transfer
                break;
        }
        return;
    }

    if( isConnected() )
    {
        KPopupMenu menu( m_view );
        menu.insertItem( SmallIconSet( "folder" ), i18n("Add Directory" ), DIRECTORY );
        menu.insertSeparator();
        menu.insertItem( SmallIconSet( "up" ), i18n(" Transfer queue to here..." ), TRANSFER_HERE );
        int id =  menu.exec( point );
        switch( id )
        {
            case DIRECTORY:
                m_view->newDirectory( 0 );
                break;

            case TRANSFER_HERE:
                m_transferDir = m_medium->mountPoint() + "/";
                debug() << "New transfer dir is: " << m_transferDir << endl;
                //start transfer
                break;

        }
    }
}

#include "vfatmediadevice.moc"
