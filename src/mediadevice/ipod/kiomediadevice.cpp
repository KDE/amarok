// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2005 Martin Aumueller <aumuell@reserv.at>
// See COPYING file for licensing information

#define DEBUG_PREFIX "KioMediaDevice"

#include <config.h>
#include "kiomediadevice.h"

#include "amarok.h"
#include "collectionbrowser.h"
#include "debug.h"
#include "k3bexporter.h"
#include "metabundle.h"
#include "playlist.h"
#include "statusbar/statusbar.h"

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



KioMediaDevice::KioMediaDevice()
: MediaDevice()
{
    m_podcastItem = NULL;
    m_staleItem = NULL;
    m_orphanedItem = NULL;
    m_invisibleItem = NULL;
    m_playlistItem = NULL;
}

void
KioMediaDevice::init( MediaBrowser* parent )
{
    MediaDevice::init( parent );
}

KioMediaDevice::~KioMediaDevice()
{
}

bool
KioMediaDevice::isConnected()
{
    return true;
}

void
KioMediaDevice::synchronizeDevice()
{
    sync();
}

MediaItem *
KioMediaDevice::trackExists( const MetaBundle& /*bundle*/ )
{
    return NULL;
}

int
KioMediaDevice::deleteItemFromDevice(MediaItem *mediaitem, bool onlyPlayed )
{
    MediaItem *item = dynamic_cast<MediaItem *>(mediaitem);
    if(!item)
        return -1;

    int count = 0;

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
                    if( count >= 0 )
                        count++;
                }
                delete item;
            }
            break;
        case MediaItem::ORPHANED:
            deleteFile( item->url() );
            delete item;
            if( count >= 0 )
                count++;
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
                    int ret = deleteItemFromDevice(it, onlyPlayed);
                    if( ret >= 0 && count >= 0 )
                    {
                        count += ret;
                    }
                    else
                        count = -1;
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
            break;
    }

    updateRootItems();

    return count;
}

bool
KioMediaDevice::openDevice( bool silent )
{
    Q_UNUSED( silent );

    m_playlistItem = new MediaItem( m_view );
    m_playlistItem->setText( 0, i18n("Playlists") );
    m_playlistItem->m_order = -5;
    m_playlistItem->setType( MediaItem::PLAYLISTSROOT );

    m_podcastItem = new MediaItem( m_view );
    m_podcastItem->setText( 0, i18n("Podcasts") );
    m_podcastItem->m_order = -4;
    m_podcastItem->setType( MediaItem::PODCASTSROOT );

    m_invisibleItem = new MediaItem( m_view );
    m_invisibleItem->setText( 0, i18n("Invisible") );
    m_invisibleItem->m_order = -3;
    m_invisibleItem->setType( MediaItem::INVISIBLEROOT );

    m_staleItem = new MediaItem( m_view );
    m_staleItem->setText( 0, i18n("Stale") );
    m_staleItem->m_order = -2;
    m_staleItem->setType( MediaItem::STALEROOT );

    m_orphanedItem = new MediaItem( m_view );
    m_orphanedItem->setText( 0, i18n("Orphaned") );
    m_orphanedItem->m_order = -2;
    m_orphanedItem->setType( MediaItem::ORPHANEDROOT );

    updateRootItems();

    return true;
}

bool
KioMediaDevice::closeDevice()  //SLOT
{
    m_view->clear();
    m_podcastItem = NULL;
    m_playlistItem = NULL;
    m_orphanedItem = NULL;
    m_staleItem = NULL;
    m_invisibleItem = NULL;

    return true;
}

MediaItem *
KioMediaDevice::getArtist(const QString &artist)
{
    for(MediaItem *it = dynamic_cast<MediaItem *>(m_view->firstChild());
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
        trackpath = mountPoint()
            + "/" + bundle.artist()
            + "/" + bundle.album()
            + "/" + track + " - " + bundle.title() + suff +  "." + type;
        trackpath.replace(" ", "_");
        QFileInfo finfo(QFile::encodeName(trackpath));
        exists = finfo.exists();
        i++;
    }
    while(exists);

    return KURL::fromPathOrURL( trackpath );
}

bool
KioMediaDevice::getCapacity( KIO::filesize_t *total, KIO::filesize_t *available )
{
    if( !isConnected() )
        return false;

#ifdef HAVE_STATVFS
    QString path = mountPoint();
    struct statvfs buf;
    if(statvfs(QFile::encodeName(path), &buf) != 0)
    {
        *total = 0;
        *available = 0;
        return false;
    }

    *total = buf.f_blocks * (KIO::filesize_t)buf.f_frsize;
    *available = buf.f_bavail * (KIO::filesize_t)buf.f_frsize;

    return *total > 0;
#else
    (void) total;
    (void) available;
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
KioMediaDevice::rmbPressed( MediaView *deviceList, QListViewItem* qitem, const QPoint& point, int )
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
        menu.insertItem( SmallIconSet( amaroK::icon( "fastforward" ) ), i18n( "&Queue Tracks" ), QUEUE );
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

                    CollectionView::instance()->organizeFiles( urls, "Copy Files To Collection", true );
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
        if( isCancelled() )
            break;
        usleep( 10000 );
    } while( m_waitForDeletion );

    if(!isTransferring())
        setProgress( progress() + 1 );
}

#include "kiomediadevice.moc"
