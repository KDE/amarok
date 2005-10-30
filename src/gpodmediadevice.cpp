// (c) 2004 Christian Muehlhaeuser <chris@chris.de>, 2005 Martin Aumueller <aumuell@reserv.at>
// See COPYING file for licensing information


#define DEBUG_PREFIX "GpodMediaDevice"

#include "debug.h"
#include "metabundle.h"

#include "gpodmediadevice.h"
#include "collectiondb.h"


#include <kapplication.h>
#include <kmountpoint.h>
#include <kpushbutton.h>
#include <kprogress.h>
#include <kmessagebox.h>
#include <qfileinfo.h>
#include <qdir.h>
#include <qtimer.h>
#include <qlabel.h>

#include <cstdlib>

#ifdef HAVE_LIBGPOD
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
#endif


GpodMediaDevice::GpodMediaDevice( MediaDeviceView* parent )
: MediaDevice( parent )
{
#ifdef HAVE_LIBGPOD
    debug() << "sizeof(MediaItem)=" << sizeof(MediaItem) << endl;
    debug() << "sizeof(Itdb_Track)=" << sizeof(Itdb_Track) << endl;
    debug() << "sizeof(IpodAlbum)=" << sizeof(IpodAlbum) << endl;
    dbChanged = false;
    m_itdb = NULL;
#endif
}

GpodMediaDevice::~GpodMediaDevice()
{
#ifdef HAVE_LIBGPOD
    if(m_itdb)
        itdb_free(m_itdb);

    m_database.clear();
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

bool
GpodMediaDevice::addTrackToDevice(const QString &pathname, const MetaBundle &bundle, bool isPodcast)
{
#ifdef HAVE_LIBGPOD
    Itdb_Track *track = itdb_track_new();
    if(!track)
        return false;

    QString type = pathname.section('.', -1);

    track->ipod_path = g_strdup( ipodPath(pathname.latin1()).latin1() );
    debug() << "on iPod: " << track->ipod_path << endl;

    track->title = g_strdup( bundle.title().isEmpty() ? i18n("Unknown").utf8() : bundle.title().utf8() );
    track->album = g_strdup( bundle.album().isEmpty() ? i18n("Unknown").utf8() : bundle.album().utf8() );
    track->artist = g_strdup( bundle.artist().isEmpty() ? i18n("Unknown").utf8() : bundle.artist().utf8() );
    track->genre = g_strdup( bundle.genre().utf8() );
    track->filetype = g_strdup( (type + "-file").utf8() );
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
        debug() << "adding podcast" << endl;
        Itdb_Playlist *podcasts = itdb_playlist_podcasts(m_itdb);
        if(!podcasts)
        {
            debug() << "creating podcast playlist" << endl;
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

    return true;
#else
    (void)pathname;
    (void)bundle;
//     (void)podcast; //FIXME
    return false;
#endif
}


void
GpodMediaDevice::synchronizeDevice()
{
    closeDevice();

    openDevice();

    updateView();
}

// should probably return MediaItems instead
QStringList
GpodMediaDevice::items( QListViewItem* item )
{
    QStringList items;

#ifdef HAVE_LIBGPOD
    if(!item)
    {
        for(QDictIterator<IpodArtist> it(m_database);
                it.current();
                ++it)
            items << it.currentKey();
    }
    else if(!item->parent())
    {
        IpodArtist *artist = m_database[item->text(0)];
        if(artist)
            for(QDictIterator<IpodAlbum> it(*artist);
                    it.current();
                    ++it)
                items << it.currentKey();
    }
    else
    {
        IpodArtist *artist = m_database[item->parent()->text(0)];
        IpodAlbum *album = NULL;
        if(artist)
            album = (*artist)[item->text(0)];
        if(album)
        {
            for(QDictIterator<Itdb_Track> it(*album);
                    it.current();
                    ++it)
            {
                items << it.currentKey();
                items << realPath(it.current()->ipod_path);
                QString track;
                track.setNum(it.current()->track_nr);
                items << track;
            }
        }
    }
#else
    (void)item;
#endif

    return items;
}


bool
GpodMediaDevice::trackExists( const MetaBundle& bundle )
{
#ifdef HAVE_LIBGPOD

    Itdb_Track *track = getTitle( bundle.artist(),
            bundle.album().isEmpty() ? i18n( "Unknown" ) : bundle.album(),
            bundle.title());

    return (track != NULL);
#else
    (void)bundle;
    return false;
#endif
}


bool
GpodMediaDevice::deleteTrackFromDevice(const QString& artist, const QString& album, const QString& title)
{
#ifdef HAVE_LIBGPOD
    debug() << "deleting: " << artist << " - " << album << " - " << title << endl;
    Itdb_Track *track = getTitle(artist, album, title);
    if(!track)
    {
        debug() << "not found" << endl;
        return false;
    }

    return removeDBTrack(track);
#else
    (void)artist;
    (void)album;
    (void)title;
    return false;
#endif
}
bool
GpodMediaDevice::openDevice(bool useDialogs)
{
#ifdef HAVE_LIBGPOD
    dbChanged = false;
    m_database.clear();

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

    GList *cur = m_itdb->tracks;
    while(cur)
    {
        Itdb_Track *track = (Itdb_Track *)cur->data;

        addTrackToList(track);

        cur = cur->next;
    }

    cur = m_itdb->playlists;
    while(cur)
    {
        Itdb_Playlist *playlist = (Itdb_Playlist *)cur->data;

        addPlaylistToList(playlist);

        cur = cur->next;
    }

    return true;
#else
    (void)useDialogs;
    return false;
#endif
}

bool
GpodMediaDevice::closeDevice()  //SLOT
{
#ifdef HAVE_LIBGPOD
    debug() << "Syncing IPod!" << endl;

    writeITunesDB();

    m_database.clear();
    itdb_free(m_itdb);
    m_itdb = NULL;

    updateView();

    return true;
#else
    return false;
#endif
}

#ifdef HAVE_LIBGPOD
void
GpodMediaDevice::addTrackToList(Itdb_Track *track)
{
    QString artistName(QString::fromUtf8(track->artist));
    IpodArtist *artist = m_database[artistName];
    if(!artist)
    {
        artist = new IpodArtist();
        m_database.insert(artistName, artist);
    }

    QString albumName(QString::fromUtf8(track->album));
    IpodAlbum *album = (*artist)[albumName];
    if(!album)
    {
        album = new IpodAlbum();
        artist->insert(albumName, album);
    }

    album->insert(QString::fromUtf8(track->title), track);
}

void
GpodMediaDevice::addPlaylistToList(Itdb_Playlist *pl)
{
    if(pl->is_spl)
    {
        debug() << "playlist " << pl->name << " is a smart playlist, ignored" << endl;
        return;
    }

    QString name(QString::fromUtf8(pl->name));
    IpodPlaylist *playlist = m_playlists[name];
    if(playlist == NULL)
    {
        playlist = new IpodPlaylist();
        m_playlists.insert(name, playlist);

        playlist->m_dbPlaylist = pl;
    }

    GList *cur = pl->members;
    while(cur)
    {
        Itdb_Track *track = (Itdb_Track *)cur->data;

        playlist->append(track);

        cur = cur->next;
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


GpodMediaDevice::IpodArtist *
GpodMediaDevice::getArtist(const QString &artist)
{
    return m_database[artist];
}

GpodMediaDevice::IpodAlbum *
GpodMediaDevice::getAlbum(const QString &artist, const QString &album)
{
    IpodArtist *a = getArtist(artist);
    if(!a)
        return NULL;

    return (*a)[album];
}

Itdb_Track *
GpodMediaDevice::getTitle(const QString &artist, const QString &album, const QString &title)
{
    IpodAlbum *a = getAlbum(artist, album);
    if(!a)
        return NULL;

    return (*a)[title];
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
#endif // HAVE_LIBGPOD
}

#ifdef HAVE_LIBGPOD
bool
GpodMediaDevice::removeDBTrack(Itdb_Track *track)
{
    if(!m_itdb)
        return false;

    if(track->itdb != m_itdb)
    {
        return false;
    }

    dbChanged = true;

    Itdb_Playlist *mpl = itdb_playlist_mpl(m_itdb);
    if(itdb_playlist_contains_track(mpl, track))
    {
        itdb_playlist_remove_track(mpl, track);
    }

    GList *cur = m_itdb->playlists;
    while(cur)
    {
        Itdb_Playlist *pl = (Itdb_Playlist *)cur->data;
        if(itdb_playlist_contains_track(pl, track))
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
