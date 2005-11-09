// (c) 2004 Christian Muehlhaeuser <chris@chris.de>, 2005 Martin Aumueller <aumuell@reserv.at>
// See COPYING file for licensing information

#define DEBUG_PREFIX "GpodMediaDevice"

#include "gpodmediadevice.h"

#include "debug.h"
#include "metabundle.h"
#include "collectiondb.h"

#include <kapplication.h>
#include <kmountpoint.h>
#include <kpushbutton.h>
#include <kprogress.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <qfileinfo.h>
#include <qdir.h>
#include <qtimer.h>
#include <qlabel.h>

#include <cstdlib>


// disable if it takes too long for you
#define CHECK_FOR_INTEGRITY 1



#ifdef HAVE_LIBGPOD
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
        void init() {m_track=NULL; m_playlist=NULL;}
        Itdb_Track *m_track;
        Itdb_Playlist *m_playlist;
        int played() const { if(m_track) return m_track->playcount; else return 0; }
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

            return NULL;
        }
};
#endif // HAVE_LIBGPOD


GpodMediaDevice::GpodMediaDevice( MediaDeviceView* parent, MediaDeviceList *listview )
    : MediaDevice( parent, listview )
#ifdef HAVE_LIBGPOD
    , m_masterPlaylist( NULL )
    , m_podcastPlaylist( NULL )
#endif // HAVE_LIBGPOD
{
#ifdef HAVE_LIBGPOD
    dbChanged = false;
    m_itdb = NULL;
    m_podcastItem = NULL;
    m_staleItem = NULL;
    m_orphanedItem = NULL;
    m_invisibleItem = NULL;
    m_playlistItem = NULL;
#endif // HAVE_LIBGPOD
}

GpodMediaDevice::~GpodMediaDevice()
{
#ifdef HAVE_LIBGPOD
    if(m_itdb)
        itdb_free(m_itdb);

    m_files.clear();
#endif
}

bool
GpodMediaDevice::isConnected()
{
#ifdef HAVE_LIBGPOD
    return (m_itdb != NULL);
#else
    return false;
#endif
}

MediaItem *
GpodMediaDevice::addTrackToDevice(const QString &pathname, const MetaBundle &bundle, bool isPodcast)
{
#ifdef HAVE_LIBGPOD
    Itdb_Track *track = itdb_track_new();
    if(!track)
        return NULL;

    QString type = pathname.section('.', -1);

    track->ipod_path = g_strdup( ipodPath(pathname.latin1()).latin1() );
    debug() << "on iPod: " << track->ipod_path << ", podcast=" << isPodcast << endl;

    track->title = g_strdup( bundle.title().isEmpty() ? i18n("Unknown").utf8() : bundle.title().utf8() );
    track->album = g_strdup( bundle.album().isEmpty() ? i18n("Unknown").utf8() : bundle.album().utf8() );
    track->artist = g_strdup( bundle.artist().isEmpty() ? i18n("Unknown").utf8() : bundle.artist().utf8() );
    track->genre = g_strdup( bundle.genre().utf8() );
    if(type=="wav" || type=="WAV")
    {
        track->filetype = g_strdup( "wav" );
    }
    else if(type=="mp3" || type=="MP3" || type=="mpeg" || type=="MPEG")
    {
        track->filetype = g_strdup( "mpeg" );
    }
    else if(type=="mp4" || type=="MP4" || type=="aac" || type=="AAC"
            || type=="m4a" || type=="M4A" || type=="m4b" || type=="M4B"
            || type=="m4p" || type=="M4P")
    {
        track->filetype = g_strdup( "mp4" );
    }
    else if(type=="aa" || type=="AA")
    {
        track->filetype = g_strdup( "audible" );
        track->unk164 |= 0x10000; // remember current position in track

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
    //track->size = 0;
    track->bitrate = bundle.bitrate();
    track->samplerate = bundle.sampleRate();
    track->tracklen = bundle.length()*1000;

    dbChanged = true;

#if 0
    // just for trying: probably leaks memory
    // artwork support not for my nano yet, cannot try
    QString image = CollectionDB::instance()->albumImage(QString(track->artist), QString(track->album), 1);
    debug() << "adding image " << image << " to " << track->artist << ":" << track->album << endl;
    itdb_track_set_thumbnail(track, image.latin1());
#endif

    itdb_track_add(m_itdb, track, -1);
    if(isPodcast)
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
#else // HAVE_LIBGPOD
    (void)pathname;
    (void)bundle;
    (void)isPodcast;
    return NULL;
#endif // HAVE_LIBGPOD
}


void
GpodMediaDevice::synchronizeDevice()
{
#ifdef HAVE_LIBGPOD
    writeITunesDB();
#endif // HAVE_LIBGPOD
}

bool
GpodMediaDevice::trackExists( const MetaBundle& bundle )
{
#ifdef HAVE_LIBGPOD

    GpodMediaItem *item = getTitle( bundle.artist(),
            bundle.album().isEmpty() ? i18n( "Unknown" ) : bundle.album(),
            bundle.title());

    return (item != NULL);
#else // HAVE_LIBGPOD
    (void)bundle;
    return false;
#endif // HAVE_LIBGPOD
}

MediaItem *
GpodMediaDevice::newPlaylist(const QString &name, MediaItem *parent, QPtrList<MediaItem> items)
{
#ifdef HAVE_LIBGPOD
    dbChanged = true;
    GpodMediaItem *item = new GpodMediaItem(parent);
    item->setType(MediaItem::PLAYLIST);
    item->setText(0, name);

    addToPlaylist(item, NULL, items);

    return item;
#else // HAVE_LIBGPOD
    (void)name;
    (void)parent;
    (void)items;
    return NULL;
#endif // HAVE_LIBGPOD
}


void
GpodMediaDevice::addToPlaylist(MediaItem *mlist, MediaItem *after, QPtrList<MediaItem> items)
{
#ifdef HAVE_LIBGPOD
    GpodMediaItem *list = dynamic_cast<GpodMediaItem *>(mlist);
    if(!list)
        return;

    dbChanged = true;

    if(list->m_playlist)
    {
        itdb_playlist_remove(list->m_playlist);
        list->m_playlist = NULL;
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
        add->setUrl( realPath( it->m_track->ipod_path ) );
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
#else // HAVE_LIBGPOD
    (void)mlist;
    (void)after;
    (void)items;
#endif // HAVE_LIBGPOD
}

bool
GpodMediaDevice::deleteItemFromDevice(MediaItem *mediaitem, bool onlyPlayed )
{
#ifdef HAVE_LIBGPOD
    GpodMediaItem *item = dynamic_cast<GpodMediaItem *>(mediaitem);
    if(!item)
        return false;

    bool ret = true;

    switch(item->type())
    {
    case MediaItem::TRACK:
    case MediaItem::INVISIBLE:
    case MediaItem::PODCASTITEM:
        if(!onlyPlayed || item->played() > 0)
        {
            // delete from playlists
            GpodMediaItem *i = static_cast<GpodMediaItem *>(m_playlistItem)->findTrack(item->m_track);
            while(i)
            {
                delete i;
                i = static_cast<GpodMediaItem *>(m_playlistItem)->findTrack(item->m_track);
            }

            // delete file
            KURL url;
            url.setPath(realPath(item->m_track->ipod_path));
            deleteFile( url );

            // remove from database
            ret = removeDBTrack(item->m_track);
            delete item;
        }
        break;
    case MediaItem::STALE:
        {
            GpodMediaItem *i = static_cast<GpodMediaItem *>(m_playlistItem)->findTrack(item->m_track);
            while(i)
            {
                delete i;
                i = static_cast<GpodMediaItem *>(m_playlistItem)->findTrack(item->m_track);
            }
            ret = removeDBTrack(item->m_track);
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
            GpodMediaItem *next = NULL;
            for(GpodMediaItem *it = dynamic_cast<GpodMediaItem *>(item->firstChild());
                    it;
                    it = next)
            {
                next = dynamic_cast<GpodMediaItem *>(it->nextSibling());
                ret = ret && deleteItemFromDevice(it, onlyPlayed);
            }
        }
        if(item->type() == MediaItem::PLAYLIST)
        {
            dbChanged = true;
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
        dbChanged = true;
        break;
    case MediaItem::UNKNOWN:
        ret = false;
        break;
    }

    updateRootItems();

    return ret;
#else // HAVE_LIBGPOD
    (void)mediaitem;
    return false;
#endif // HAVE_LIBGPOD
}

bool
GpodMediaDevice::openDevice(bool useDialogs)
{
#ifdef HAVE_LIBGPOD
    dbChanged = false;
    m_files.clear();

    GError *err = NULL;

    // prefer configured mount point
    // if existing but empty create initial directories and empty database
    if(!m_itdb && !m_mntpnt.isEmpty())
    {
        m_itdb = itdb_parse(m_mntpnt.latin1(), &err);
        if(err)
            g_error_free(err);
        err = NULL;

        if(m_itdb == NULL)
        {
            QDir dir(m_mntpnt);
            if(!dir.exists())
            {
                if(useDialogs)
                    KMessageBox::error( m_parent->m_parent, i18n("Media device mount point does not exist"),
                            i18n( "Media Device Browser" ) );
                return false;
            }

            // initialize iPod
            m_itdb = itdb_new();
            if(m_itdb==NULL)
            {
                if(useDialogs)
                    KMessageBox::error( m_parent->m_parent, i18n("Failed to initialize iPod mounted at ") + m_mntpnt,
                            i18n( "Media Device Browser" ) );

                return false;
            }

            Itdb_Playlist *mpl = itdb_playlist_new("amaroK", false);
            itdb_playlist_set_mpl(mpl);
            Itdb_Playlist *podcasts = itdb_playlist_new("Podcasts", false);
            itdb_playlist_set_podcasts(podcasts);
            itdb_playlist_add(m_itdb, podcasts, -1);
            itdb_playlist_add(m_itdb, mpl, 0);

            itdb_set_mountpoint(m_itdb, m_mntpnt.latin1());

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

            if(useDialogs)
                KMessageBox::information( m_parent->m_parent, i18n("Initialized iPod mounted at ") + m_mntpnt,
                        i18n( "Media Device Browser" ) );
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
            err = NULL;
        }

        if(!m_itdb)
        {
            m_itdb = itdb_parse("/media/ipod", &err);
            if(err)
                g_error_free(err);
            err = NULL;
        }

        if(!m_itdb)
        {
            m_itdb = itdb_parse("/media/iPod", &err);
            if(err)
                g_error_free(err);
            err = NULL;
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

                m_itdb = itdb_parse(mountpoint.latin1(), &err);
                if(err)
                    g_error_free(err);
                err = NULL;

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

    GList *cur = m_itdb->playlists;
    while(cur)
    {
        Itdb_Playlist *playlist = (Itdb_Playlist *)cur->data;

        addPlaylistToList(playlist);

        cur = cur->next;
    }

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
                item->setUrl(filename);
            }
        }
    }
#endif // CHECK_FOR_INTEGRITY

    updateRootItems();

    return true;
#else // HAVE_LIBGPOD
    (void)useDialogs;
    return false;
#endif // HAVE_LIBGPOD
}

bool
GpodMediaDevice::closeDevice()  //SLOT
{
#ifdef HAVE_LIBGPOD
    debug() << "Syncing iPod!" << endl;

    m_listview->clear();
    m_podcastItem = NULL;
    m_playlistItem = NULL;
    m_orphanedItem = NULL;
    m_staleItem = NULL;
    m_invisibleItem = NULL;

    writeITunesDB();

    m_files.clear();
    itdb_free(m_itdb);
    m_itdb = NULL;
    m_masterPlaylist = NULL;
    m_podcastPlaylist = NULL;

    return true;
#else // HAVE_LIBGPOD
    return false;
#endif // HAVE_LIBGPOD
}

void
GpodMediaDevice::renameItem( QListViewItem *i ) // SLOT
{
#ifdef HAVE_LIBGPOD
    GpodMediaItem *item = dynamic_cast<GpodMediaItem *>(i);
    if(!item)
        return;

    if(!item->type() == MediaItem::PLAYLIST)
        return;

    dbChanged = true;

    g_free(item->m_playlist->name);
    item->m_playlist->name = g_strdup( item->text( 0 ).utf8() );
#else // HAVE_LIBGPOD
    (void)i;
#endif // HAVE_LIBGPOD
}

#ifdef HAVE_LIBGPOD
void
GpodMediaDevice::playlistFromItem(GpodMediaItem *item)
{
    dbChanged = true;

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
    GpodMediaItem *item = NULL;

#ifdef CHECK_FOR_INTEGRITY
    QString path = realPath(track->ipod_path);
    QFileInfo finfo(path.latin1());
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
        item->setUrl(realPath(track->ipod_path));
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
        item->m_order = track->track_nr;
        item->setUrl(realPath(track->ipod_path));
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
        }

        item = new GpodMediaItem(channel);
        item->setText( 0, QString::fromUtf8(track->title) );
        item->setType( MediaItem::PODCASTITEM );
        item->m_track = track;
        item->setUrl(realPath(track->ipod_path));
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
        item->setUrl(realPath(track->ipod_path));
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
        item->m_order = i;
        item->setUrl(realPath(track->ipod_path));

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
        path = m_itdb->mountpoint;
        path.append(QString(ipodPath).replace(':', "/"));
    }

    return path;
}

QString
GpodMediaDevice::ipodPath(const char *realPath)
{
    QString path = realPath;
    if(m_itdb)
    {
        QString mp = m_itdb->mountpoint;
        if(path.startsWith(mp))
        {
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
    if(dbChanged)
    {
        bool ok = true;
        GError *error = NULL;
        if (!itdb_write (m_itdb, &error))
        {   /* an error occured */
            ok = false;
            if(error)
            {
                if (error->message)
                    debug() << "itdb_write error: " << error->message << endl;
                else
                    debug() << "itdb_write error: " << "error->message == NULL!" << endl;
                g_error_free (error);
            }
            error = NULL;
        }

        if (ok)
        {   /* write shuffle data */
            if (!itdb_shuffle_write (m_itdb, NULL, &error))
            {   /* an error occured */
                ok = false;
                if(error)
                {
                    if (error->message)
                        debug() << "itdb_shuffle_write error: " << error->message << endl;
                    else
                        debug() << "itdb_shuffle_write error: " << "error->message == NULL!" << endl;
                    g_error_free (error);
                }
                error = NULL;
            }
        }

        dbChanged = false;
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

    return NULL;
}

GpodMediaItem *
GpodMediaDevice::getAlbum(const QString &artist, const QString &album)
{
    GpodMediaItem *item = getArtist(artist);
    if(item)
        return dynamic_cast<GpodMediaItem *>(item->findItem(album));

    return NULL;
}

GpodMediaItem *
GpodMediaDevice::getTitle(const QString &artist, const QString &album, const QString &title)
{
    GpodMediaItem *item = getAlbum(artist, album);
    if(item)
    {
        GpodMediaItem *track = dynamic_cast<GpodMediaItem *>(item->findItem(title));
        if(track)
            return track;
    }

    if(m_podcastItem)
    {
        item = dynamic_cast<GpodMediaItem *>(m_podcastItem->findItem(album));
        if(item)
        {
            GpodMediaItem *track = dynamic_cast<GpodMediaItem *>(item->findItem(title));
            if(track)
                return track;
        }
    }

    return NULL;
}
#endif // HAVE_LIBGPOD

QString
GpodMediaDevice::createPathname(const MetaBundle &bundle)
{
#ifdef HAVE_LIBGPOD
    QString local = bundle.filename();
    QString type = local.section('.', -1);

    QString trackpath;
    bool exists = false;
    do
    {
        int num = std::rand() % 1000000;
        int dir = num % itdb_musicdirs_number(m_itdb);
        trackpath.sprintf( ":iPod_Control:Music:F%02d:%s", dir,
                (QString("kpod") + QString::number(num) + "." + type).latin1());
        QFileInfo finfo(realPath(trackpath.latin1()));
        exists = finfo.exists();
    }
    while(exists);

    return realPath(trackpath.latin1());
#else // HAVE_LIBGPOD
    (void)bundle;
    return QString::null;
#endif // HAVE_LIBGPOD
}

#ifdef HAVE_LIBGPOD
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

    dbChanged = true;

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
#endif // HAVE_LIBGPOD

#include "gpodmediadevice.moc"
