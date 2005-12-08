// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2005 Martin Aumueller <aumuell@reserv.at>
// See COPYING file for licensing information

#define DEBUG_PREFIX "GpodMediaDevice"

#include <config.h>
#include "gpodmediadevice.h"

#include "debug.h"
#include "metabundle.h"
#include "collectiondb.h"
#include "statusbar/statusbar.h"
#include "k3bexporter.h"
#include "playlist.h"
#include "collectionbrowser.h"
#include "playlistbrowser.h"

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


// disable if it takes too long for you
#define CHECK_FOR_INTEGRITY 1



#include "metadata/audible/taglib_audiblefile.h"

#include <glib-object.h>

// the gobject system needs this - otherwise ipod-device enabled libgpod crashes
static class GobjectInitializer {
    public:
    GobjectInitializer() { g_type_init(); }
} gobjectInitializer;

class TrackList : public QPtrList<Itdb_Track>
{
    int compareItems ( QPtrCollection::Item track1, QPtrCollection::Item track2 )
    {
        Itdb_Track *t1 = (Itdb_Track *)track1;
        Itdb_Track *t2 = (Itdb_Track *)track2;

        if(t1->track_nr != t2->track_nr)
            return t1->track_nr - t2->track_nr;

        return strcasecmp(t1->title, t2->title);
    }
};

class GpodMediaItem : public MediaItem
{
    public:
        GpodMediaItem(QListView *parent ) : MediaItem(parent) { init(); }
        GpodMediaItem(QListViewItem *parent ) : MediaItem(parent) { init(); }
        GpodMediaItem(QListView *parent, QListViewItem *after) : MediaItem(parent, after) { init(); }
        GpodMediaItem(QListViewItem *parent, QListViewItem *after) : MediaItem(parent, after) { init(); }
        void init() {m_track=0; m_playlist=0;}
        void bundleFromTrack( Itdb_Track *track )
        {
            if( m_bundle )
                delete m_bundle;

            m_bundle = new MetaBundle();
            m_bundle->setArtist( QString::fromUtf8( track->artist ) );
            m_bundle->setAlbum( QString::fromUtf8( track->album ) );
            m_bundle->setTitle( QString::fromUtf8( track->title ) );
            m_bundle->setComment( QString::fromUtf8( track->comment ) );
            m_bundle->setGenre( QString::fromUtf8( track->genre ) );
            m_bundle->setYear( track->year );
            m_bundle->setTrack( track->track_nr );
            m_bundle->setLength( track->tracklen/1000 );
            m_bundle->setBitrate( track->bitrate );
            m_bundle->setSampleRate( track->samplerate );
        }
        Itdb_Track *m_track;
        Itdb_Playlist *m_playlist;
        int played() const { if(m_track) return m_track->playcount; else return 0; }
        int recentlyPlayed() const { if(m_track) return m_track->recent_playcount; else return 0; }
        int rating() const { if(m_track) return m_track->rating; else return 0; }
        void setRating(int rating) { if(m_track) m_track->rating = m_track->app_rating = rating; /* m_dbChanged=true; */ }
        bool ratingChanged() const { if(m_track) return m_track->rating != m_track->app_rating; else return false; }
        GpodMediaItem *findTrack(Itdb_Track *track)
        {
            if(m_track == track)
                return this;

            for(GpodMediaItem *it=dynamic_cast<GpodMediaItem *>(firstChild());
                    it;
                    it = dynamic_cast<GpodMediaItem *>(it->nextSibling()))
            {
                GpodMediaItem *found = it->findTrack(track);
                if(found)
                    return found;
            }

            return 0;
        }
};


GpodMediaDevice::GpodMediaDevice( MediaDeviceView* parent, MediaDeviceList *listview )
    : KioMediaDevice( parent, listview )
    , m_masterPlaylist( 0 )
    , m_podcastPlaylist( 0 )
{
    m_dbChanged = false;
    m_itdb = 0;
    m_podcastItem = 0;
    m_staleItem = 0;
    m_orphanedItem = 0;
    m_invisibleItem = 0;
    m_playlistItem = 0;
    m_supportsArtwork = false;
    m_isShuffle = false;
}

GpodMediaDevice::~GpodMediaDevice()
{
    if(m_itdb)
        itdb_free(m_itdb);

    m_files.clear();
}

bool
GpodMediaDevice::isConnected()
{
    return (m_itdb != 0);
}

MediaItem *
GpodMediaDevice::insertTrackIntoDB(const QString &pathname, const MetaBundle &bundle, const PodcastInfo *podcastInfo)
{
    Itdb_Track *track = itdb_track_new();
    if(!track)
        return 0;

    QString type = pathname.section('.', -1).lower();

    track->ipod_path = g_strdup( ipodPath(pathname).latin1() );
    debug() << "on iPod: " << track->ipod_path << ", podcast=" << podcastInfo << endl;

    track->title = g_strdup( bundle.title().isEmpty() ? i18n("Unknown").utf8() : bundle.title().utf8() );
    track->album = g_strdup( bundle.album().isEmpty() ? i18n("Unknown").utf8() : bundle.album().utf8() );
    track->artist = g_strdup( bundle.artist().isEmpty() ? i18n("Unknown").utf8() : bundle.artist().utf8() );
    track->genre = g_strdup( bundle.genre().utf8() );
    if(type=="wav")
    {
        track->filetype = g_strdup( "wav" );
    }
    else if(type=="mp3" || type=="mpeg")
    {
        track->filetype = g_strdup( "mpeg" );
    }
    else if(type=="mp4" || type=="aac" || type=="m4a" || type=="m4p")
    {
        track->filetype = g_strdup( "mp4" );
    }
    else if(type=="m4b")
    {
        track->filetype = g_strdup( "mp4" );
        track->flag3 |= 0x01; // remember current position in track
    }
    else if(type=="aa")
    {
        track->filetype = g_strdup( "audible" );
        track->flag3 |= 0x01; // remember current position in track

        TagLib::Audible::File *f = new TagLib::Audible::File( QFile::encodeName( bundle.url().path() ) );
        TagLib::Audible::Tag *t = f->getAudibleTag();
        track->drm_userid = t->userID();
    }
    else
    {
        track->filetype = g_strdup( type.utf8() );
    }

    track->comment = g_strdup( bundle.comment().utf8() );
    track->track_nr = bundle.track();
    track->year = bundle.year();
    track->size = QFile( pathname ).size();
    track->bitrate = bundle.bitrate();
    track->samplerate = bundle.sampleRate();
    track->tracklen = bundle.length()*1000;

    if(podcastInfo)
    {
        track->flag1 |= 0x02; // podcast
        track->flag2 |= 0x01; // skip  when shuffling
        track->flag3 |= 0x01; // remember playback position
        track->flag4 |= 0x01; // also show description on iPod
        track->unk176 = 0x00020000; // for podcasts
        QString plaindesc = podcastInfo->description;
        plaindesc.replace( QRegExp("<[^>]*>"), "" );
        track->description = g_strdup( plaindesc.utf8() );
        track->subtitle = g_strdup( plaindesc.utf8() );
        track->podcasturl = g_strdup( podcastInfo->url.utf8() );
        track->podcastrss = g_strdup( podcastInfo->rss.utf8() );
        //track->category = g_strdup( "Unknown" );
        track->time_released = itdb_time_host_to_mac( podcastInfo->date.toTime_t() );
    }
    else
    {
        track->unk176 = 0x00010000; // for non-podcasts
    }

    m_dbChanged = true;

#ifdef HAVE_ITDB_TRACK_SET_THUMBNAILS
    if( m_supportsArtwork )
    {
        QString image = CollectionDB::instance()->albumImage(QString(track->artist), QString(track->album), 1);
        if( !image.endsWith( "@nocover.png" ) )
        {
            debug() << "adding image " << image << " to " << track->artist << ":" << track->album << endl;
            itdb_track_set_thumbnails( track, g_strdup( QFile::encodeName(image) ) );
        }
    }
#endif

    itdb_track_add(m_itdb, track, -1);
    if(podcastInfo)
    {
        Itdb_Playlist *podcasts = itdb_playlist_podcasts(m_itdb);
        if(!podcasts)
        {
            podcasts = itdb_playlist_new("Podcasts", false);
            itdb_playlist_set_podcasts(podcasts);
            itdb_playlist_add(m_itdb, podcasts, -1);
        }
        itdb_playlist_add_track(podcasts, track, -1);
    }
    else
    {
        // gtkpod 0.94 does not like if not all songs in the db are on the master playlist
        // but we try anyway
        Itdb_Playlist *mpl = itdb_playlist_mpl(m_itdb);
        itdb_playlist_add_track(mpl, track, -1);
    }

    return addTrackToList(track);
}


void
GpodMediaDevice::synchronizeDevice()
{
    writeITunesDB();
    //sync();
}

MediaItem *
GpodMediaDevice::trackExists( const MetaBundle& bundle )
{
    GpodMediaItem *item = getTrack( bundle.artist(),
            bundle.album().isEmpty() ? i18n( "Unknown" ) : bundle.album(),
            bundle.title(),
            bundle.track() );

    return item;
}

MediaItem *
GpodMediaDevice::newPlaylist(const QString &name, MediaItem *parent, QPtrList<MediaItem> items)
{
    m_dbChanged = true;
    GpodMediaItem *item = new GpodMediaItem(parent);
    item->setType(MediaItem::PLAYLIST);
    item->setText(0, name);

    addToPlaylist(item, 0, items);

    return item;
}


void
GpodMediaDevice::addToPlaylist(MediaItem *mlist, MediaItem *after, QPtrList<MediaItem> items)
{
    GpodMediaItem *list = dynamic_cast<GpodMediaItem *>(mlist);
    if(!list)
        return;

    m_dbChanged = true;

    if(list->m_playlist)
    {
        itdb_playlist_remove(list->m_playlist);
        list->m_playlist = 0;
    }

    int order;
    GpodMediaItem *it;
    if(after)
    {
        order = after->m_order + 1;
        it = dynamic_cast<GpodMediaItem*>(after->nextSibling());
    }
    else
    {
        order = 0;
        it = dynamic_cast<GpodMediaItem*>(list->firstChild());
    }

    for( ; it; it = dynamic_cast<GpodMediaItem *>(it->nextSibling()))
    {
        it->m_order += items.count();
    }

    for(GpodMediaItem *it = dynamic_cast<GpodMediaItem *>(items.first());
            it;
            it = dynamic_cast<GpodMediaItem *>(items.next()) )
    {
        if(!it->m_track)
            continue;

        GpodMediaItem *add;
        if(it->parent() == list)
        {
            add = it;
            if(after)
            {
                it->moveItem(after);
            }
            else
            {
                list->takeItem(it);
                list->insertItem(it);
            }
        }
        else
        {
            if(after)
            {
                add = new GpodMediaItem(list, after);
            }
            else
            {
                add = new GpodMediaItem(list);
            }
        }
        after = add;

        add->setType(MediaItem::PLAYLISTITEM);
        add->m_track = it->m_track;
        add->m_url.setPath( realPath( it->m_track->ipod_path ) );
        add->setText(0, QString::fromUtf8(it->m_track->artist) + " - " + QString::fromUtf8(it->m_track->title) );
        add->m_order = order;
        order++;
    }

    // make numbering consecutive
    int i=0;
    for(GpodMediaItem *it = dynamic_cast<GpodMediaItem *>(list->firstChild());
            it;
            it = dynamic_cast<GpodMediaItem *>(it->nextSibling()))
    {
        it->m_order = i;
        i++;
    }

    playlistFromItem(list);
}

void
GpodMediaDevice::addToDirectory(MediaItem *, QPtrList<MediaItem>)
{
   debug() << "addToDirectory: not implemented" << endl;
}

int
GpodMediaDevice::deleteItemFromDevice(MediaItem *mediaitem, bool onlyPlayed )
{
    GpodMediaItem *item = dynamic_cast<GpodMediaItem *>(mediaitem);
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
            Itdb_Track *track = item->m_track;
            delete item;

            // delete from playlists
            for( GpodMediaItem *it = static_cast<GpodMediaItem *>(m_playlistItem)->findTrack(track);
                    it;
                    it = static_cast<GpodMediaItem *>(m_playlistItem)->findTrack(track) )
            {
                delete it;
            }

            // delete all other occurences
            for( GpodMediaItem *it = getTrack( track );
                    it;
                    it = getTrack( track ) )
            {
                delete it;
            }

            if( !stale )
            {
                // delete file
                KURL url;
                url.setPath(realPath(track->ipod_path));
                deleteFile( url );
                count++;
            }

            // remove from database
            if( !removeDBTrack(track) )
                count = -1;
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
            GpodMediaItem *next = 0;
            for(GpodMediaItem *it = dynamic_cast<GpodMediaItem *>(item->firstChild());
                    it;
                    it = next)
            {
                next = dynamic_cast<GpodMediaItem *>(it->nextSibling());
                int ret = deleteItemFromDevice(it, onlyPlayed);
                if( ret >= 0 && count >= 0 )
                    count += ret;
                else
                    count = -1;
            }
        }
        if(item->type() == MediaItem::PLAYLIST)
        {
            m_dbChanged = true;
            itdb_playlist_remove(item->m_playlist);
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
        // FIXME possibly wrong instance of track is removed
        itdb_playlist_remove_track(item->m_playlist, item->m_track);
        delete item;
        m_dbChanged = true;
        break;
    case MediaItem::DIRECTORY:
    case MediaItem::UNKNOWN:
        // this should not happen
        count = -1;
        break;
    }

    updateRootItems();

    return count;
}

bool
GpodMediaDevice::openDevice(bool silent)
{
    m_isShuffle = false;
    m_supportsArtwork = false;
    m_dbChanged = false;
    m_files.clear();

    GError *err = 0;

    // prefer configured mount point
    // if existing but empty create initial directories and empty database
    if( !m_itdb && !m_mntpnt.isEmpty() )
    {
        m_itdb = itdb_parse( QFile::encodeName(m_mntpnt), &err );
        if( err )
            g_error_free(err);
        err = 0;

        if( m_itdb == 0 )
        {
            QDir dir(m_mntpnt);
            if( !dir.exists() )
            {
                if( !silent )
                {
                    amaroK::StatusBar::instance()->longMessage(
                            i18n("Media device: Mount point %1 does not exist").arg(m_mntpnt),
                            KDE::StatusBar::Error );
                }
                return false;
            }

            // initialize iPod
            m_itdb = itdb_new();
            if( m_itdb == 0 )
            {
                if( !silent )
                {
                    amaroK::StatusBar::instance()->longMessage(
                            i18n("Media Device: Failed to initialize iPod mounted at %1").arg(m_mntpnt),
                            KDE::StatusBar::Sorry );
                }

                return false;
            }

            Itdb_Playlist *mpl = itdb_playlist_new("iPod", false);
            itdb_playlist_set_mpl(mpl);
            Itdb_Playlist *podcasts = itdb_playlist_new("Podcasts", false);
            itdb_playlist_set_podcasts(podcasts);
            itdb_playlist_add(m_itdb, podcasts, -1);
            itdb_playlist_add(m_itdb, mpl, 0);

            itdb_set_mountpoint(m_itdb, QFile::encodeName(m_mntpnt));

            QString path = m_mntpnt + "/iPod_Control";
            dir.setPath(path);
            if(!dir.exists())
                dir.mkdir(dir.absPath());

            path += m_mntpnt + "/iPod_Control/Music";
            dir.setPath(path);
            if(!dir.exists())
                dir.mkdir(dir.absPath());

            path = m_mntpnt + "/iPod_Control/iTunes";
            dir.setPath(path);
            if(!dir.exists())
                dir.mkdir(dir.absPath());

            if(!silent)
            {
                amaroK::StatusBar::instance()->longMessage(
                        i18n("Media Device: Initialized iPod mounted at %1").arg(m_mntpnt),
                        KDE::StatusBar::Information );
            }
        }
    }
    else
    {
        // try some common directories
        if(!m_itdb)
        {
            m_itdb = itdb_parse("/mnt/ipod", &err);
            if(err)
                g_error_free(err);
            err = 0;
        }

        if(!m_itdb)
        {
            m_itdb = itdb_parse("/media/ipod", &err);
            if(err)
                g_error_free(err);
            err = 0;
        }

        if(!m_itdb)
        {
            m_itdb = itdb_parse("/media/iPod", &err);
            if(err)
                g_error_free(err);
            err = 0;
        }

        if ( !m_itdb ) {
            // try to find a mounted ipod
            KMountPoint::List currentmountpoints = KMountPoint::currentMountPoints();
            KMountPoint::List::Iterator mountiter = currentmountpoints.begin();
            for(; mountiter != currentmountpoints.end(); ++mountiter) {
                QString mountpoint = (*mountiter)->mountPoint();
                QString device = (*mountiter)->mountedFrom();

                // only care about scsi devices (/dev/sd at the beginning or scsi somewhere in its name)
                if (device.find("/dev/sd") != 0 && device.find("scsi") < 0)
                    continue;

                m_itdb = itdb_parse(QFile::encodeName(mountpoint), &err);
                if(err)
                    g_error_free(err);
                err = 0;

                if (m_itdb)
                {
                    break;
                }
            }
        }

        if(!m_itdb)
        {
            debug() << "failed to find mounted iPod" << endl;
            return false;
        }
    }

    if( m_itdb->device )
    {
        guint model;
        g_object_get(m_itdb->device,
                "device-model", &model,
                NULL); // 0 -> warning about missing sentinel

        switch(model)
        {
        case MODEL_TYPE_COLOR:
        case MODEL_TYPE_COLOR_U2:
        case MODEL_TYPE_NANO_WHITE:
        case MODEL_TYPE_NANO_BLACK:
        case MODEL_TYPE_VIDEO_WHITE:
        case MODEL_TYPE_VIDEO_BLACK:
            m_supportsArtwork = true;
            debug() << "detected iPod photo" << endl;
            break;
        case MODEL_TYPE_REGULAR:
        case MODEL_TYPE_REGULAR_U2:
        case MODEL_TYPE_MINI:
        case MODEL_TYPE_MINI_BLUE:
        case MODEL_TYPE_MINI_PINK:
        case MODEL_TYPE_MINI_GREEN:
        case MODEL_TYPE_MINI_GOLD:
            m_supportsArtwork = false;
            debug() << "detected regular iPod" << endl;
            break;
        case MODEL_TYPE_SHUFFLE:
            m_supportsArtwork = false;
            m_isShuffle = true;
            debug() << "detected iPod shuffle" << endl;
            break;
        default:
        case MODEL_TYPE_INVALID:
        case MODEL_TYPE_UNKNOWN:
            m_supportsArtwork = false;
            debug() << "unknown type" << endl;
            break;
        }
    }
    else
    {
        debug() << "device type detection failed, assuming regular iPod" << endl;
    }

#ifdef HAVE_ITDB_TRACK_SET_THUMBNAILS
    if( m_supportsArtwork )
    {
        QString path = m_mntpnt + "/iPod_Control/Artwork";
        QDir dir( path );
        if(!dir.exists())
            dir.mkdir(dir.absPath());

        path += "/ArtworkDB";
        QFile file( path );
        if( !file.exists() )
        {
            file.open( IO_WriteOnly );
            file.close();
        }
    }
#else
    m_supportsArtwork = false;
#endif

    if(itdb_musicdirs_number(m_itdb) <= 0)
    {
        m_itdb->musicdirs = 20;
    }

    m_playlistItem = new GpodMediaItem( m_listview );
    m_playlistItem->setText( 0, i18n("Playlists") );
    m_playlistItem->m_order = -5;
    m_playlistItem->setType( MediaItem::PLAYLISTSROOT );

    m_podcastItem = new GpodMediaItem( m_listview );
    m_podcastItem->setText( 0, i18n("Podcasts") );
    m_podcastItem->m_order = -4;
    m_podcastItem->setType( MediaItem::PODCASTSROOT );

    m_invisibleItem = new GpodMediaItem( m_listview );
    m_invisibleItem->setText( 0, i18n("Invisible") );
    m_invisibleItem->m_order = -3;
    m_invisibleItem->setType( MediaItem::INVISIBLEROOT );

    m_staleItem = new GpodMediaItem( m_listview );
    m_staleItem->setText( 0, i18n("Stale") );
    m_staleItem->m_order = -2;
    m_staleItem->setType( MediaItem::STALEROOT );

    m_orphanedItem = new GpodMediaItem( m_listview );
    m_orphanedItem->setText( 0, i18n("Orphaned") );
    m_orphanedItem->m_order = -2;
    m_orphanedItem->setType( MediaItem::ORPHANEDROOT );

    kapp->processEvents( 100 );

    GList *cur = m_itdb->playlists;
    while(cur)
    {
        Itdb_Playlist *playlist = (Itdb_Playlist *)cur->data;

        addPlaylistToList(playlist);

        cur = cur->next;
    }

    kapp->processEvents( 100 );

    cur = m_itdb->tracks;
    while(cur)
    {
        Itdb_Track *track = (Itdb_Track *)cur->data;

        addTrackToList(track);

        cur = cur->next;
    }

#ifdef CHECK_FOR_INTEGRITY
    QString musicpath = QString(m_itdb->mountpoint) + "/iPod_Control/Music";
    QDir dir( musicpath, QString::null, QDir::Name | QDir::IgnoreCase, QDir::Dirs );
    for(unsigned i=0; i<dir.count(); i++)
    {
        if(dir[i] == "." || dir[i] == "..")
            continue;

        kapp->processEvents( 100 );

        QString hashpath = musicpath + "/" + dir[i];
        QDir hashdir( hashpath, QString::null, QDir::Name | QDir::IgnoreCase, QDir::Files );
        for(unsigned j=0; j<hashdir.count(); j++)
        {

            QString filename = hashpath + "/" + hashdir[j];
            QString ipodPath = ":iPod_Control:Music:" + dir[i] + ":" + hashdir[j];
            Itdb_Track *track = m_files[ipodPath.lower()];
            if(!track)
            {
                debug() << "file: " << filename << " is orphaned" << endl;
                GpodMediaItem *item = new GpodMediaItem(m_orphanedItem);
                item->setType(MediaItem::ORPHANED);
                KURL url;
                url.setPath(filename);
                MetaBundle *bundle = new MetaBundle(url);
                QString title = bundle->artist() + " - " + bundle->title();
                item->setText(0, title);
                item->m_bundle = bundle;
                item->m_url.setPath(filename);
            }
        }
    }
#endif // CHECK_FOR_INTEGRITY
    kapp->processEvents( 100 );

    updateRootItems();

    return true;
}

bool
GpodMediaDevice::closeDevice()  //SLOT
{
    debug() << "Syncing iPod!" << endl;

    m_listview->clear();
    m_podcastItem = 0;
    m_playlistItem = 0;
    m_orphanedItem = 0;
    m_staleItem = 0;
    m_invisibleItem = 0;

    writeITunesDB();

    m_files.clear();
    itdb_free(m_itdb);
    m_itdb = 0;
    m_masterPlaylist = 0;
    m_podcastPlaylist = 0;

    return true;
}

void
GpodMediaDevice::renameItem( QListViewItem *i ) // SLOT
{
    GpodMediaItem *item = dynamic_cast<GpodMediaItem *>(i);
    if(!item)
        return;

    if(!item->type() == MediaItem::PLAYLIST)
        return;

    m_dbChanged = true;

    g_free(item->m_playlist->name);
    item->m_playlist->name = g_strdup( item->text( 0 ).utf8() );
}

void
GpodMediaDevice::playlistFromItem(GpodMediaItem *item)
{
    m_dbChanged = true;

    item->m_playlist = itdb_playlist_new(item->text(0).utf8(), false /* dumb playlist */ );
    itdb_playlist_add(m_itdb, item->m_playlist, -1);
    for(GpodMediaItem *it = dynamic_cast<GpodMediaItem *>(item->firstChild());
            it;
            it = dynamic_cast<GpodMediaItem *>(it->nextSibling()) )
    {
        itdb_playlist_add_track(item->m_playlist, it->m_track, -1);
        it->m_playlist = item->m_playlist;
    }
}


GpodMediaItem *
GpodMediaDevice::addTrackToList(Itdb_Track *track)
{
    bool visible = false;
    bool stale = false;
    GpodMediaItem *item = 0;

#ifdef CHECK_FOR_INTEGRITY
    QString path = realPath(track->ipod_path);
    QFileInfo finfo(path);
    if(!finfo.exists())
    {
        stale = true;
        debug() << "track: " << track->artist << " - " << track->album << " - " << track->title << " is stale: " << track->ipod_path << " does not exist" << endl;
        item = new GpodMediaItem(m_staleItem);
        item->setType(MediaItem::STALE);
        QString title = QString::fromUtf8(track->artist) + " - "
            + QString::fromUtf8(track->title);
        item->setText( 0, title );
        item->m_track = track;
        item->m_url.setPath(realPath(track->ipod_path));
    }
    else
    {
        m_files.insert( QString(track->ipod_path).lower(), track );
    }
#endif // CHECK_FOR_INTEGRITY

    if(!stale && m_masterPlaylist && itdb_playlist_contains_track(m_masterPlaylist, track))
    {
        visible = true;

        QString artistName(QString::fromUtf8(track->artist));
        GpodMediaItem *artist = getArtist(artistName);
        if(!artist)
        {
            artist = new GpodMediaItem(m_listview);
            artist->setText( 0, artistName );
            artist->setType( MediaItem::ARTIST );
        }

        QString albumName(QString::fromUtf8(track->album));
        MediaItem *album = artist->findItem(albumName);
        if(!album)
        {
            album = new GpodMediaItem( artist );
            album->setText( 0, albumName );
            album->setType( MediaItem::ALBUM );
        }

        item = new GpodMediaItem( album );
        QString titleName = QString::fromUtf8(track->title);
        item->setText( 0, titleName );
        item->setType( MediaItem::TRACK );
        item->m_track = track;
        item->bundleFromTrack( track );
        item->bundle()->setPath( realPath(track->ipod_path) );
        item->m_order = track->track_nr;
        item->m_url.setPath(realPath(track->ipod_path));
    }

    if(!stale && m_podcastPlaylist && itdb_playlist_contains_track(m_podcastPlaylist, track))
    {
        visible = true;

        QString channelName(QString::fromUtf8(track->album));
        MediaItem *channel = m_podcastItem->findItem(channelName);
        if(!channel)
        {
            channel = new GpodMediaItem(m_podcastItem);
            channel->setText( 0, channelName );
            channel->setType( MediaItem::PODCASTCHANNEL );
            channel->m_podcastInfo = new PodcastInfo;
        }

        item = new GpodMediaItem(channel);
        item->setText( 0, QString::fromUtf8(track->title) );
        item->setType( MediaItem::PODCASTITEM );
        item->m_track = track;
        item->bundleFromTrack( track );
        item->bundle()->setPath( realPath(track->ipod_path) );
        item->m_url.setPath(realPath(track->ipod_path));

        PodcastInfo *info = new PodcastInfo;
        item->m_podcastInfo = info;
        info->url = QString::fromUtf8( track->podcasturl );
        info->rss = QString::fromUtf8( track->podcastrss );
        info->description = QString::fromUtf8( track->description );
        info->date.setTime_t( itdb_time_mac_to_host( track->time_released) );

        if( !info->rss.isEmpty() && channel->podcastInfo()->rss.isEmpty() )
        {
           channel->podcastInfo()->rss = info->rss;
        }
    }

    if(!stale && !visible)
    {
        debug() << "invisible, title=" << track->title << endl;
        item = new GpodMediaItem(m_invisibleItem);
        QString title = QString::fromUtf8(track->artist) + " - "
            + QString::fromUtf8(track->title);
        item->setText( 0, title );
        item->setType( MediaItem::INVISIBLE );
        item->m_track = track;
        item->bundleFromTrack( track );
        item->bundle()->setPath( realPath(track->ipod_path) );
        item->m_url.setPath(realPath(track->ipod_path));
    }

    updateRootItems();

    return item;
}

void
GpodMediaDevice::addPlaylistToList(Itdb_Playlist *pl)
{
    if(itdb_playlist_is_mpl(pl))
    {
        m_masterPlaylist = pl;
        return;
    }

    if(itdb_playlist_is_podcasts(pl))
    {
        m_podcastPlaylist = pl;
        return;
    }

    if(pl->is_spl)
    {
        debug() << "playlist " << pl->name << " is a smart playlist, ignored" << endl;
        return;
    }

    QString name(QString::fromUtf8(pl->name));
    GpodMediaItem *playlist = dynamic_cast<GpodMediaItem *>(m_playlistItem->findItem(name));
    if(!playlist)
    {
        playlist = new GpodMediaItem( m_playlistItem );
        playlist->setText( 0, name );
        playlist->setType( MediaItem::PLAYLIST );
        playlist->m_playlist = pl;
    }

    int i=0;
    GList *cur = pl->members;
    while(cur)
    {
        Itdb_Track *track = (Itdb_Track *)cur->data;
        GpodMediaItem *item = new GpodMediaItem(playlist);
        QString title = QString::fromUtf8(track->artist) + " - "
            + QString::fromUtf8(track->title);
        item->setText( 0, title );
        item->setType( MediaItem::PLAYLISTITEM );
        item->m_playlist = pl;
        item->m_track = track;
        item->bundleFromTrack( track );
        item->bundle()->setPath( realPath(track->ipod_path) );
        item->m_order = i;
        item->m_url.setPath(realPath(track->ipod_path));

        cur = cur->next;
        i++;
    }
}

QString
GpodMediaDevice::realPath(const char *ipodPath)
{
    QString path;
    if(m_itdb)
    {
        path = QFile::decodeName(m_itdb->mountpoint);
        path.append(QString(ipodPath).replace(':', "/"));
    }

    return path;
}

QString
GpodMediaDevice::ipodPath(const QString &realPath)
{
    if(m_itdb && m_itdb->mountpoint)
    {
        QString mp = QFile::decodeName(m_itdb->mountpoint);
        if(realPath.startsWith(mp))
        {
            QString path = realPath;
            path = path.mid(mp.length());
            path = path.replace('/', ":");
            return path;
        }
    }

    return QString::null;
}

void
GpodMediaDevice::writeITunesDB()
{
    if(m_itdb)
        m_dbChanged = true; // write unconditionally for resetting recent_playcount

    if(m_dbChanged)
    {
        bool ok = true;
        GError *error = 0;
        if (!itdb_write (m_itdb, &error))
        {   /* an error occured */
            ok = false;
            if(error)
            {
                if (error->message)
                    debug() << "itdb_write error: " << error->message << endl;
                else
                    debug() << "itdb_write error: " << "error->message == 0!" << endl;
                g_error_free (error);
            }
            error = 0;
        }

        if( ok && m_isShuffle )
        {
            /* write shuffle data */
            if (!itdb_shuffle_write (m_itdb, &error))
            {   /* an error occured */
                ok = false;
                if(error)
                {
                    if (error->message)
                        debug() << "itdb_shuffle_write error: " << error->message << endl;
                    else
                        debug() << "itdb_shuffle_write error: " << "error->message == 0!" << endl;
                    g_error_free (error);
                }
                error = 0;
            }
        }

        if( !ok )
        {
            amaroK::StatusBar::instance()->longMessage(
                    i18n("Media device: failed to write iPod database"),
                    KDE::StatusBar::Error );
        }

        m_dbChanged = false;
    }
}


GpodMediaItem *
GpodMediaDevice::getArtist(const QString &artist)
{
    for(GpodMediaItem *it = dynamic_cast<GpodMediaItem *>(m_listview->firstChild());
            it;
            it = dynamic_cast<GpodMediaItem *>(it->nextSibling()))
    {
        if(it->m_type==MediaItem::ARTIST && artist == it->text(0))
            return it;
    }

    return 0;
}

GpodMediaItem *
GpodMediaDevice::getAlbum(const QString &artist, const QString &album)
{
    GpodMediaItem *item = getArtist(artist);
    if(item)
        return dynamic_cast<GpodMediaItem *>(item->findItem(album));

    return 0;
}

GpodMediaItem *
GpodMediaDevice::getTrack(const QString &artist, const QString &album, const QString &title, int trackNumber)
{
    GpodMediaItem *item = getAlbum(artist, album);
    if(item)
    {
        for( GpodMediaItem *track = dynamic_cast<GpodMediaItem *>(item->findItem(title));
                track;
                track = dynamic_cast<GpodMediaItem *>(item->findItem(title, track)) )
        {
            if( trackNumber==-1 || track->bundle()->track() == trackNumber )
                return track;
        }
    }

    if(m_podcastItem)
    {
        item = dynamic_cast<GpodMediaItem *>(m_podcastItem->findItem(album));
        if(item)
        {
            for( GpodMediaItem *track = dynamic_cast<GpodMediaItem *>(item->findItem(title));
                    track;
                    track = dynamic_cast<GpodMediaItem *>(item->findItem(title, track)) )
            {
                if( trackNumber==-1 || track->bundle()->track() == trackNumber )
                    return track;
            }
        }
    }

    return 0;
}

GpodMediaItem *
GpodMediaDevice::getTrack( const Itdb_Track *itrack )
{
    QString artist = QString::fromUtf8( itrack->artist );
    QString album = QString::fromUtf8( itrack->album );
    QString title = QString::fromUtf8( itrack->title );

    GpodMediaItem *item = getAlbum( artist, album );
    if(item)
    {
        for( GpodMediaItem *track = dynamic_cast<GpodMediaItem *>(item->findItem( title ) );
                track;
                track = dynamic_cast<GpodMediaItem *>(item->findItem(title, track)) )
        {
            if( track->m_track == itrack )
                return track;
        }
    }

    if(m_podcastItem)
    {
        item = dynamic_cast<GpodMediaItem *>(m_podcastItem->findItem(album));
        if(item)
        {
            for( GpodMediaItem *track = dynamic_cast<GpodMediaItem *>(item->findItem(title));
                    track;
                    track = dynamic_cast<GpodMediaItem *>(item->findItem(title, track)) )
            {
                if( track->m_track == itrack )
                    return track;
            }
        }
    }

    return 0;
}


KURL
GpodMediaDevice::determineURLOnDevice(const MetaBundle &bundle)
{
    QString local = bundle.filename();
    QString type = local.section('.', -1);

    QString trackpath;
    bool exists = false;
    do
    {
        int num = std::rand() % 1000000;
        int dir = num % itdb_musicdirs_number(m_itdb);
        trackpath.sprintf( ":iPod_Control:Music:F%02d:kpod%d.%s", dir, num, type.latin1() );
        QFileInfo finfo(realPath(trackpath.latin1()));
        exists = finfo.exists();
    }
    while(exists);

    return realPath(trackpath.latin1());
}

bool
GpodMediaDevice::removeDBTrack(Itdb_Track *track)
{
    if(!m_itdb)
        return false;

    if(!track)
        return false;

    if(track->itdb != m_itdb)
    {
        return false;
    }

    m_dbChanged = true;

    Itdb_Playlist *mpl = itdb_playlist_mpl(m_itdb);
    while(itdb_playlist_contains_track(mpl, track))
    {
        itdb_playlist_remove_track(mpl, track);
    }

    GList *cur = m_itdb->playlists;
    while(cur)
    {
        Itdb_Playlist *pl = (Itdb_Playlist *)cur->data;
        while(itdb_playlist_contains_track(pl, track))
        {
            itdb_playlist_remove_track(pl, track);
        }
        cur = cur->next;
    }

    // also frees track's memory
    itdb_track_remove(track);

    return true;
}

bool
GpodMediaDevice::getCapacity( unsigned long *total, unsigned long *available )
{
    if(!m_itdb)
        return false;

#ifdef HAVE_STATVFS
    QString path = QFile::decodeName(m_itdb->mountpoint);
    path.append("/iPod_Control");
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
    if(!m_itdb->device)
        return false;

    guint64 vol_size, vol_avail;

    g_object_get(m_itdb->device,
            "volume-size", &vol_size,
            "volume-available", &vol_avail,
            0);

    if(total)
        *total = vol_size/1024;

    if(available)
        *available = vol_avail/1024;

    return vol_size > 0;
#endif
}

void
GpodMediaDevice::rmbPressed( MediaDeviceList *deviceList, QListViewItem* qitem, const QPoint& point, int )
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
        switch( item->type() )
        {
        case MediaItem::ARTIST:
            menu.insertItem( SmallIconSet( "cdrom_unmount" ), i18n( "Burn All Tracks by This Artist" ), BURN_ARTIST );
            menu.setItemEnabled( BURN_ARTIST, K3bExporter::isAvailable() );
            break;

        case MediaItem::ALBUM:
            menu.insertItem( SmallIconSet( "cdrom_unmount" ), i18n( "Burn This Album" ), BURN_ALBUM );
            menu.setItemEnabled( BURN_ALBUM, K3bExporter::isAvailable() );
            break;

        default:
            menu.insertItem( SmallIconSet( "cdrom_unmount" ), i18n( "Burn to CD as Data" ), BURN_DATACD );
            menu.setItemEnabled( BURN_DATACD, K3bExporter::isAvailable() );
            menu.insertItem( SmallIconSet( "cdaudio_unmount" ), i18n( "Burn to CD as Audio" ), BURN_AUDIOCD );
            menu.setItemEnabled( BURN_AUDIOCD, K3bExporter::isAvailable() );
            break;
        }

        menu.insertSeparator();

        if( (item->type() == MediaItem::PODCASTITEM
                 || item->type() == MediaItem::PODCASTCHANNEL) )
        {
            menu.insertItem( SmallIconSet( "favorites" ), i18n( "Subscribe to This Podcast" ), SUBSCRIBE );
            menu.setItemEnabled( SUBSCRIBE, item->podcastInfo() && !item->podcastInfo()->rss.isEmpty() );
            menu.insertSeparator();
        }

        KPopupMenu *playlistsMenu = 0;
        switch( item->type() )
        {
        case MediaItem::ARTIST:
        case MediaItem::ALBUM:
        case MediaItem::TRACK:
        case MediaItem::PODCASTCHANNEL:
        case MediaItem::PODCASTSROOT:
        case MediaItem::PODCASTITEM:
            if(m_playlistItem)
            {
                menu.insertItem( SmallIconSet( "player_playlist_2" ), i18n( "Make Media Device Playlist" ), MAKE_PLAYLIST );

                playlistsMenu = new KPopupMenu(&menu);
                int i=0;
                for(MediaItem *it = dynamic_cast<MediaItem *>(m_playlistItem->firstChild());
                        it;
                        it = dynamic_cast<MediaItem *>(it->nextSibling()))
                {
                    playlistsMenu->insertItem( SmallIconSet( "player_playlist_2" ), it->text(0), FIRST_PLAYLIST+i );
                    i++;
                }
                menu.insertItem( SmallIconSet( "player_playlist_2" ), i18n("Add to Playlist"), playlistsMenu, ADD_TO_PLAYLIST );
                menu.setItemEnabled( ADD_TO_PLAYLIST, m_playlistItem->childCount()>0 );
                menu.insertSeparator();
            }
            break;

        case MediaItem::ORPHANED:
        case MediaItem::ORPHANEDROOT:
            menu.insertItem( SmallIconSet( "editrename" ), i18n( "Add to Database" ), ADD );
            break;

        case MediaItem::PLAYLIST:
            menu.insertItem( SmallIconSet( "editclear" ), i18n( "Rename" ), RENAME );
            break;

        default:
            break;
        }

        if( item->type() == MediaItem::PODCASTSROOT || item->type() == MediaItem::PODCASTCHANNEL )
        {
            menu.insertItem( SmallIconSet( "editdelete" ), i18n( "Delete Podcasts Already Played" ), DELETE_PLAYED );
        }
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
            case MAKE_PLAYLIST:
                {
                    QPtrList<MediaItem> items;
                    deviceList->getSelectedLeaves( 0, &items );
                    QString base(i18n("New Playlist"));
                    QString name = base;
                    int i=1;
                    while(m_playlistItem->findItem(name))
                    {
                        QString num;
                        num.setNum(i);
                        name = base + " " + num;
                        i++;
                    }
                    MediaItem *pl = newPlaylist(name, m_playlistItem, items);
                    deviceList->ensureItemVisible(pl);
                    deviceList->rename(pl, 0);
                }
                break;
            case ADD:
                if(item->type() == MediaItem::ORPHANEDROOT)
                {
                    MediaItem *next = 0;
                    for(MediaItem *it = dynamic_cast<MediaItem *>(item->firstChild());
                            it;
                            it = next)
                    {
                        next = dynamic_cast<MediaItem *>(it->nextSibling());
                        item->takeItem(it);
                        insertTrackIntoDB(it->url().path(), *it->bundle(), false);
                        delete it;
                    }
                }
                else
                {
                    for(deviceList->selectedItems().first();
                            deviceList->selectedItems().current();
                            deviceList->selectedItems().next())
                    {
                        MediaItem *it = dynamic_cast<MediaItem *>(deviceList->selectedItems().current());
                        if(it->type() == MediaItem::ORPHANED)
                        {
                            it->parent()->takeItem(it);
                            insertTrackIntoDB(it->url().path(), *it->bundle(), false);
                            delete it;
                        }
                    }
                }
                break;
            case DELETE_PLAYED:
                {
                    MediaItem *podcasts = 0;
                    if(item->type() == MediaItem::PODCASTCHANNEL)
                        podcasts = dynamic_cast<MediaItem *>(item->parent());
                    else
                        podcasts = item;
                    deleteFromDevice( podcasts, true );
                }
                break;
            case DELETE:
                deleteFromDevice();
                break;
            case SUBSCRIBE:
                PlaylistBrowser::instance()->addPodcast( item->podcastInfo()->rss );
                break;
            default:
                if( id >= FIRST_PLAYLIST )
                {
                    QString name = playlistsMenu->text(id);
                    if( name != QString::null )
                    {
                        MediaItem *list = m_playlistItem->findItem(name);
                        if(list)
                        {
                            MediaItem *after = 0;
                            for(MediaItem *it = dynamic_cast<MediaItem *>(list->firstChild());
                                    it;
                                    it = dynamic_cast<MediaItem *>(it->nextSibling()))
                                after = it;
                            QPtrList<MediaItem> items;
                            deviceList->getSelectedLeaves( 0, &items );
                            addToPlaylist( list, after, items );
                        }
                    }
                }
                break;
        }
    }
}

QStringList
GpodMediaDevice::supportedFiletypes()
{
    QStringList list;
    list << "m4a";
    list << "m4b";
    list << "mp3";
    list << "wav";
    list << "aa";
    return list;
}

#include "gpodmediadevice.moc"
