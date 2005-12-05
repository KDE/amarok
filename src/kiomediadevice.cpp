// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2005 Martin Aumueller <aumuell@reserv.at>
// See COPYING file for licensing information

#define DEBUG_PREFIX "KioMediaDevice"

#include <config.h>
#include "kiomediadevice.h"

#include "debug.h"
#include "metabundle.h"
#include "statusbar/statusbar.h"
#include "k3bexporter.h"
#include "playlist.h"
#include "collectionbrowser.h"

#include <kapplication.h>
#include <kmountpoint.h>
#include <kpushbutton.h>
#include <kprogress.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <kpopupmenu.h>
#include <qfileinfo.h>
#include <qdir.h>
#include <qtimer.h>
#include <qlabel.h>
#include <qregexp.h>

#ifdef HAVE_STATVFS
#include <stdint.h>
#include <sys/statvfs.h>
#endif

#include <cstdlib>
#include <unistd.h>


// disable if it takes too long for you
#define CHECK_FOR_INTEGRITY 1



KioMediaDevice::KioMediaDevice( MediaDeviceView* parent, MediaDeviceList *listview )
: MediaDevice( parent, listview )
{
    m_podcastItem = NULL;
    m_staleItem = NULL;
    m_orphanedItem = NULL;
    m_invisibleItem = NULL;
    m_playlistItem = NULL;
}

KioMediaDevice::~KioMediaDevice()
{
}

bool
KioMediaDevice::isConnected()
{
    return true;
}

MediaItem *
KioMediaDevice::copyTrackToDevice(const MetaBundle &bundle, const PodcastInfo *podcastInfo) // used in GpodMediaDevice
{
    KURL url = determineURLOnDevice(bundle);

    // check if path exists and make it if needed
    QFileInfo finfo( url.path() );
    QDir dir = finfo.dir();
    while ( !dir.exists() )
    {
        QString path = dir.absPath();
        QDir parentdir;
        QDir create;
        do
        {
            create.setPath(path);
            path = path.section("/", 0, path.contains('/')-1);
            parentdir.setPath(path);
        }
        while( !path.isEmpty() && !(path==m_mntpnt) && !parentdir.exists() );
        debug() << "trying to create \"" << path << "\"" << endl;
        if(!create.mkdir( create.absPath() ))
        {
            break;
        }
    }

    if ( !dir.exists() )
    {
        amaroK::StatusBar::instance()->longMessage(
                i18n( "Media Device: Creating directory for file %1 failed" ).arg( url.path() ),
                KDE::StatusBar::Error );
        return NULL;
    }

    m_wait = true;

    KIO::CopyJob *job = KIO::copy( bundle.url(), url, false );
    connect( job, SIGNAL( result( KIO::Job * ) ),
            this,  SLOT( fileTransferred( KIO::Job * ) ) );

    while ( m_wait )
    {
        usleep(10000);
        kapp->processEvents( 100 );
    }

    if(m_copyFailed)
    {
        amaroK::StatusBar::instance()->longMessage(
                i18n( "Media Device: Copying %1 to %2 failed" ).arg(bundle.url().prettyURL()).arg(url.prettyURL()),
                KDE::StatusBar::Error );
        return NULL;
    }

    MetaBundle bundle2(url);
    if(!bundle2.isValidMedia())
    {
        // probably s.th. went wrong
        amaroK::StatusBar::instance()->longMessage(
                i18n( "Media Device: Reading tags from %1 failed" ).arg( url.prettyURL() ),
                KDE::StatusBar::Error );
        QFile::remove( url.path() );
        return NULL;
    }

    return insertTrackIntoDB( url.path(), bundle, podcastInfo );
}

MediaItem *
KioMediaDevice::insertTrackIntoDB(const QString &path, const MetaBundle &bundle, const PodcastInfo*)
{
    return addTrackToList( path, bundle );
}

void
KioMediaDevice::synchronizeDevice()
{
    sync();
}

MediaItem *
KioMediaDevice::trackExists( const MetaBundle& bundle )
{
    return NULL;
}

void
KioMediaDevice::addToDirectory(MediaItem *, QPtrList<MediaItem>)
{
    debug() << "addToDirectory: not implemented" << endl;
}

bool
KioMediaDevice::deleteItemFromDevice(MediaItem *mediaitem, bool onlyPlayed )
{
    MediaItem *item = dynamic_cast<MediaItem *>(mediaitem);
    if(!item)
        return false;

    bool ret = true;

    switch(item->type())
    {
        case MediaItem::STALE:
        case MediaItem::TRACK:
        case MediaItem::INVISIBLE:
        case MediaItem::PODCASTITEM:
            if(!onlyPlayed || item->played() > 0)
            {
                bool stale = item->type()==MediaItem::STALE;

                if( !stale )
                {
                    deleteFile( item->url() );
                }
                delete item;
            }
            break;
        case MediaItem::ORPHANED:
            deleteFile( item->url() );
            delete item;
            ret = true;
            break;
        case MediaItem::PLAYLISTSROOT:
        case MediaItem::PODCASTSROOT:
        case MediaItem::INVISIBLEROOT:
        case MediaItem::STALEROOT:
        case MediaItem::ORPHANEDROOT:
        case MediaItem::ARTIST:
        case MediaItem::ALBUM:
        case MediaItem::PODCASTCHANNEL:
        case MediaItem::PLAYLIST:
            // just recurse
            {
                MediaItem *next = NULL;
                for(MediaItem *it = dynamic_cast<MediaItem *>(item->firstChild());
                        it;
                        it = next)
                {
                    next = dynamic_cast<MediaItem *>(it->nextSibling());
                    ret = ret && deleteItemFromDevice(it, onlyPlayed);
                }
            }
            if(item->type() != MediaItem::PLAYLISTSROOT
                    && item->type() != MediaItem::PODCASTSROOT
                    && item->type() != MediaItem::INVISIBLEROOT
                    && item->type() != MediaItem::STALEROOT
                    && item->type() != MediaItem::ORPHANEDROOT)
            {
                if(!onlyPlayed || item->played() > 0 || item->childCount() == 0)
                {
                    if(item->childCount() > 0)
                        debug() << "recursive deletion should have removed all children from " << item << "(" << item->text(0) << ")" << endl;
                    delete item;
                }
            }
            break;
        case MediaItem::PLAYLISTITEM:
            delete item;
            break;
        case MediaItem::DIRECTORY:
        case MediaItem::UNKNOWN:
            ret = false;
            break;
    }

    updateRootItems();

    return ret;
}

bool
KioMediaDevice::openDevice(bool silent)
{
    m_playlistItem = new MediaItem( m_listview );
    m_playlistItem->setText( 0, i18n("Playlists") );
    m_playlistItem->m_order = -5;
    m_playlistItem->setType( MediaItem::PLAYLISTSROOT );

    m_podcastItem = new MediaItem( m_listview );
    m_podcastItem->setText( 0, i18n("Podcasts") );
    m_podcastItem->m_order = -4;
    m_podcastItem->setType( MediaItem::PODCASTSROOT );

    m_invisibleItem = new MediaItem( m_listview );
    m_invisibleItem->setText( 0, i18n("Invisible") );
    m_invisibleItem->m_order = -3;
    m_invisibleItem->setType( MediaItem::INVISIBLEROOT );

    m_staleItem = new MediaItem( m_listview );
    m_staleItem->setText( 0, i18n("Stale") );
    m_staleItem->m_order = -2;
    m_staleItem->setType( MediaItem::STALEROOT );

    m_orphanedItem = new MediaItem( m_listview );
    m_orphanedItem->setText( 0, i18n("Orphaned") );
    m_orphanedItem->m_order = -2;
    m_orphanedItem->setType( MediaItem::ORPHANEDROOT );

    updateRootItems();

    return true;
}

bool
KioMediaDevice::closeDevice()  //SLOT
{
    m_listview->clear();
    m_podcastItem = NULL;
    m_playlistItem = NULL;
    m_orphanedItem = NULL;
    m_staleItem = NULL;
    m_invisibleItem = NULL;

    return true;
}

MediaItem *
KioMediaDevice::addTrackToList( const QString &path, const MetaBundle &bundle)
{
    MediaItem *artist = getArtist(bundle.artist());
    if(!artist)
    {
        artist = new MediaItem(m_listview);
        artist->setText( 0, bundle.artist() );
        artist->setType( MediaItem::ARTIST );
    }

    MediaItem *album = artist->findItem(bundle.album());
    if(!album)
    {
        album = new MediaItem( artist );
        album->setText( 0, bundle.album() );
        album->setType( MediaItem::ALBUM );
    }

    MediaItem *item = new MediaItem( album );
    item->setText( 0, bundle.title() );
    item->setType( MediaItem::TRACK );
    item->m_bundle = new MetaBundle( bundle );
    item->m_order = bundle.track();
    item->m_url.setPath(path);

    updateRootItems();

    return item;
}


MediaItem *
KioMediaDevice::getArtist(const QString &artist)
{
    for(MediaItem *it = dynamic_cast<MediaItem *>(m_listview->firstChild());
            it;
            it = dynamic_cast<MediaItem *>(it->nextSibling()))
    {
        if(it->m_type==MediaItem::ARTIST && artist == it->text(0))
            return it;
    }

    return NULL;
}

MediaItem *
KioMediaDevice::getAlbum(const QString &artist, const QString &album)
{
    MediaItem *item = getArtist(artist);
    if(item)
        return dynamic_cast<MediaItem *>(item->findItem(album));

    return NULL;
}

MediaItem *
KioMediaDevice::getTrack(const QString &artist, const QString &album, const QString &title, int trackNumber)
{
    MediaItem *item = getAlbum(artist, album);
    if(item)
    {
        for( MediaItem *track = dynamic_cast<MediaItem *>(item->findItem(title));
                track;
                track = dynamic_cast<MediaItem *>(item->findItem(title, track)) )
        {
            if( trackNumber==-1 || track->bundle()->track() == trackNumber )
                return track;
        }
    }

    if(m_podcastItem)
    {
        item = dynamic_cast<MediaItem *>(m_podcastItem->findItem(album));
        if(item)
        {
            for( MediaItem *track = dynamic_cast<MediaItem *>(item->findItem(title));
                    track;
                    track = dynamic_cast<MediaItem *>(item->findItem(title, track)) )
            {
                if( trackNumber==-1 || track->bundle()->track() == trackNumber )
                    return track;
            }
        }
    }

    return NULL;
}

KURL
KioMediaDevice::determineURLOnDevice(const MetaBundle &bundle)
{
    QString local = bundle.filename();
    QString type = local.section('.', -1);

    QString track;
    track.sprintf("%02d", bundle.track() );

    QString trackpath;
    bool exists = false;
    int i=0;
    do
    {
        QString suff;
        if( i )
        {
            suff.sprintf("_%d", i );
        }
        trackpath = m_mntpnt
            + "/" + bundle.artist()
            + "/" + bundle.album()
            + "/" + track + " - " + bundle.title() + suff +  "." + type;
        trackpath.replace(" ", "_");
        QFileInfo finfo(QFile::encodeName(trackpath));
        exists = finfo.exists();
        i++;
    }
    while(exists);

    return trackpath;
}

bool
KioMediaDevice::getCapacity( unsigned long *total, unsigned long *available )
{
    if( !isConnected() )
        return false;

#ifdef HAVE_STATVFS
    QString path = m_mntpnt;
    struct statvfs buf;
    if(statvfs(QFile::encodeName(path), &buf) != 0)
    {
        *total = 0;
        *available = 0;
        return false;
    }

    *total = buf.f_blocks * (uint64_t)buf.f_frsize / 1024;
    *available = buf.f_bavail * (uint64_t)buf.f_frsize / 1024;

    return *total > 0;
#else
    return false;
#endif
}

void
KioMediaDevice::fileDeleted( KIO::Job *job )  //SLOT, used in GpodMediaDevice
{
    if(job->error())
    {
        debug() << "file deletion failed: " << job->errorText() << endl;
    }
    m_waitForDeletion = false;
    m_parent->updateStats();
}

void
KioMediaDevice::fileTransferred( KIO::Job *job )  //SLOT, used in GpodMediaDevice
{
    if(job->error())
    {
        m_copyFailed = true;
        debug() << "file transfer failed: " << job->errorText() << endl;
    }
    else
    {
        m_copyFailed = false;

        setProgress( progress() + 1 );

        // the track just transferred has not yet been removed from the queue
        m_transferList->takeItem( m_transferList->firstChild() );
    }
    m_parent->updateStats();

    m_wait = false;
}

void
KioMediaDevice::rmbPressed( MediaDeviceList *deviceList, QListViewItem* qitem, const QPoint& point, int )
{
    MediaItem *item = dynamic_cast<MediaItem *>(qitem);
    if ( item )
    {
        KURL::List urls = deviceList->nodeBuildDragList( 0 );
        KPopupMenu menu( deviceList );

        enum Actions { APPEND, LOAD, QUEUE,
            COPY_TO_COLLECTION,
            BURN_ARTIST, BURN_ALBUM, BURN_DATACD, BURN_AUDIOCD,
            RENAME, SUBSCRIBE,
            MAKE_PLAYLIST, ADD_TO_PLAYLIST, ADD,
            DELETE_PLAYED, DELETE,
            FIRST_PLAYLIST};

        menu.insertItem( SmallIconSet( "player_playlist_2" ), i18n( "&Load" ), LOAD );
        menu.insertItem( SmallIconSet( "1downarrow" ), i18n( "&Append to Playlist" ), APPEND );
        menu.insertItem( SmallIconSet( "2rightarrow" ), i18n( "&Queue Tracks" ), QUEUE );
        menu.insertSeparator();

        menu.insertItem( SmallIconSet( "collection" ), i18n( "&Copy to Collection" ), COPY_TO_COLLECTION );

        menu.insertItem( SmallIconSet( "cdrom_unmount" ), i18n( "Burn to CD as Data" ), BURN_DATACD );
        menu.setItemEnabled( BURN_DATACD, K3bExporter::isAvailable() );
        menu.insertItem( SmallIconSet( "cdaudio_unmount" ), i18n( "Burn to CD as Audio" ), BURN_AUDIOCD );
        menu.setItemEnabled( BURN_AUDIOCD, K3bExporter::isAvailable() );

        menu.insertSeparator();

        menu.insertItem( SmallIconSet( "editdelete" ), i18n( "Delete" ), DELETE );

        int id =  menu.exec( point );
        switch( id )
        {
            case LOAD:
                Playlist::instance()->insertMedia( urls, Playlist::Replace );
                break;
            case APPEND:
                Playlist::instance()->insertMedia( urls, Playlist::Append );
                break;
            case QUEUE:
                Playlist::instance()->insertMedia( urls, Playlist::Queue );
                break;
            case COPY_TO_COLLECTION:
                {
                    QPtrList<MediaItem> items;
                    deviceList->getSelectedLeaves( 0, &items );

                    KURL::List urls;
                    for( MediaItem *it = items.first();
                            it;
                            it = items.next() )
                    {
                        if( it->url().isValid() )
                            urls << it->url();
                    }

                    CollectionView::instance()->organizeFiles( urls, true );
                }
                break;
            case BURN_ARTIST:
                K3bExporter::instance()->exportArtist( item->text(0) );
                break;
            case BURN_ALBUM:
                K3bExporter::instance()->exportAlbum( item->text(0) );
                break;
            case BURN_DATACD:
                K3bExporter::instance()->exportTracks( urls, K3bExporter::DataCD );
                break;
            case BURN_AUDIOCD:
                K3bExporter::instance()->exportTracks( urls, K3bExporter::AudioCD );
                break;
            case RENAME:
                deviceList->rename(item, 0);
                break;
            case DELETE:
                deleteFromDevice();
                break;
        }
    }
}

void
KioMediaDevice::deleteFile( const KURL &url ) // used in GpodMediaDevice
{
    debug() << "deleting " << url.prettyURL() << endl;
    m_waitForDeletion = true;
    KIO::Job *job = KIO::file_delete( url, false );
    connect( job, SIGNAL( result( KIO::Job * ) ),
            this,  SLOT( fileDeleted( KIO::Job * ) ) );
    do
    {
        kapp->processEvents( 100 );
        usleep( 10000 );
    } while( m_waitForDeletion );

    if(!isTransferring())
        setProgress( progress() + 1 );
}

#include "kiomediadevice.moc"
