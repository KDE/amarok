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
        debug() << "compare" << endl;
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
    dbChanged = false;
    m_itdb = NULL;

    openIPod(false);
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
GpodMediaDevice::renameArtist( const QString& oldArtist, const QString& newArtist )
{
#ifdef HAVE_LIBGPOD
    IpodArtist *artist = m_database.take(oldArtist);
    if(!artist)
        return false;

    dbChanged = true;

    for(QDictIterator<IpodAlbum> itArtist(*artist);
            itArtist.current();
            ++itArtist)
    {
        for(QDictIterator<Itdb_Track> it(*itArtist.current());
                it.current();
                ++it)
        {
            g_free( it.current()->artist );
            it.current()->artist = g_strdup( newArtist.utf8() );
            addTrackToList(it.current());
        }
    }

    delete artist;

    m_parent->m_deviceList->renderView( 0 );
    return true;
#else
    (void)oldArtist;
    (void)newArtist;
    return false;
#endif
}

bool
GpodMediaDevice::renameAlbum( const QString& artistName, const QString& oldAlbum, const QString& newAlbum )
{
#ifdef HAVE_LIBGPOD
    IpodArtist *artist = m_database[artistName];
    if(!artist)
        return false;

    IpodAlbum *album = artist->take(oldAlbum);
    if(!album)
        return false;

    dbChanged = true;

    for(QDictIterator<Itdb_Track> it(*album);
            it.current();
            ++it)
    {
        g_free( it.current()->album );
        it.current()->album = g_strdup( newAlbum.utf8() );
        addTrackToList(it.current());
    }

    delete album;

    m_parent->m_deviceList->renderView( 0 );
    return true;
#else
    (void)artistName;
    (void)oldAlbum;
    (void)newAlbum;
    return false;
#endif
}

bool
GpodMediaDevice::renameTrack( const QString& artistName, const QString& albumName,
                const QString& oldTrack, const QString& newTrack )
{
#ifdef HAVE_LIBGPOD
    IpodArtist *artist = m_database[artistName];
    if(!artist)
        return false;

    IpodAlbum *album = (*artist)[albumName];
    if(!album)
        return false;

    Itdb_Track *track = album->take(oldTrack);
    if(!track)
        return false;

    dbChanged = true;

    g_free( track->title );
    track->title = g_strdup( newTrack.utf8() );

    addTrackToList(track);

    m_parent->m_deviceList->renderView( 0 );
    return true;
#else
    (void)artistName;
    (void)albumName;
    (void)oldTrack;
    (void)newTrack;
    return false;
#endif
}

KURL::List
GpodMediaDevice::songsByArtist( const QString& artistName )
{
    KURL::List items;

#ifdef HAVE_LIBGPOD
    IpodArtist *artist = m_database[artistName];
    if(!artist)
        return items;

    for(QDictIterator<IpodAlbum> itArtist(*artist);
            itArtist.current();
            ++itArtist)
    {
        TrackList tracks;
        for(QDictIterator<Itdb_Track> it(*itArtist.current());
                it.current();
                ++it)
        {
            tracks.append(it.current());
        }

        tracks.sort();

        for(tracks.first(); tracks.current(); tracks.next())
        {
            KURL url;
            url.setPath(realPath(tracks.current()->ipod_path));
            items << url;
        }
    }
#else
    (void)artistName;
#endif
    return items;
}

KURL::List
GpodMediaDevice::songsByArtistAlbum( const QString& artistName, const QString& albumName )
{
    KURL::List items;

#ifdef HAVE_LIBGPOD
    IpodArtist *artist = m_database[artistName];
    if(!artist)
        return items;

    IpodAlbum *album = (*artist)[albumName];
    if(!album)
        return items;

    TrackList tracks;
    for(QDictIterator<Itdb_Track> it(*album);
            it.current();
            ++it)
    {
        tracks.append(it.current());
    }

    tracks.sort();

    for(tracks.first(); tracks.current(); tracks.next())
    {
        KURL url;
        url.setPath(realPath(tracks.current()->ipod_path));
        items << url;
    }
#else
    (void)artistName;
    (void)albumName;
#endif
    return items;
}

void
GpodMediaDevice::transferFiles()  //SLOT
{
#ifdef HAVE_LIBGPOD
    m_parent->m_transferButton->setEnabled( false );

    m_parent->m_progress->setProgress( 0 );
    m_parent->m_progress->setTotalSteps( m_parent->m_transferList->childCount() );
    m_parent->m_progress->show();

    // ok, let's copy the stuff to the ipod
    lock( true );

    // iterate through items
    for( MediaItem *cur =  dynamic_cast<MediaItem *>(m_parent->m_transferList->firstChild());
            cur != NULL;
            cur =  dynamic_cast<MediaItem *>(m_parent->m_transferList->firstChild()) )
    {
        debug() << "Transfering: " << cur->url().path() << endl;
        bool isPodcast = false;
        MetaBundle *bundle = cur->bundle();
        if(bundle)
        {
            // currently only podcasts bring their own bundle
            isPodcast = true;
        }
        else
        {
            bundle = new MetaBundle( cur->url() );
        }

        Itdb_Track *track = newDBTrack(*bundle);

        QString trackpath = realPath(track->ipod_path);

        // check if path exists and make it if needed
        QFileInfo finfo( trackpath );
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
            KMessageBox::error( m_parent->m_parent,
                    i18n("Could not create directory for file") + trackpath,
                    i18n( "Media Device Browser" ) );
            delete bundle;
            break;
        }

        m_wait = true;

        KIO::CopyJob *job = KIO::copy( cur->url(), KURL( trackpath ), false );
        connect( job, SIGNAL( copyingDone( KIO::Job *, const KURL &, const KURL &, bool, bool ) ),
                this,  SLOT( fileTransferred() ) );

        while ( m_wait )
        {
            usleep(10000);
            kapp->processEvents( 100 );
        }


        KURL url;
        url.setPath(trackpath);
        MetaBundle bundle2(url);
        if(!bundle2.isValidMedia())
        {
            // probably s.th. went wrong
            debug() << "Reading tags failed! File not added!" << endl;
            QFile::remove( trackpath );
        }
        else
        {
            addDBTrack(track, isPodcast);
            //writeITunesDB();

            m_parent->m_transferList->takeItem( cur );
            delete cur;
            cur = NULL;
        }
        delete bundle;
    }
    unlock();
    syncIPod();
    fileTransferFinished();

    m_parent->m_transferButton->setEnabled( m_parent->m_transferList->childCount()>0 );
#endif
}


void
GpodMediaDevice::deleteFiles( const KURL::List& urls )
{
#ifdef HAVE_LIBGPOD
    //NOTE we assume that currentItem is the main target
    int count  = urls.count();

    m_parent->m_stats->setText( i18n( "1 track to be deleted", "%n tracks to be deleted", count ) );
    m_parent->m_progress->setProgress( 0 );
    m_parent->m_progress->setTotalSteps( count );
    m_parent->m_progress->show();

    int button = KMessageBox::warningContinueCancel( m_parent,
            i18n( "<p>You have selected 1 file to be <b>irreversibly</b> deleted.",
                "<p>You have selected %n files to be <b>irreversibly</b> deleted.",
                count
                ),
            QString::null,
            KGuiItem(i18n("&Delete"),"editdelete") );

    if ( button == KMessageBox::Continue )
    {

        KURL::List::ConstIterator it = urls.begin();
        for ( ; it != urls.end(); ++it )
        {
            m_parent->m_progress->setProgress( m_parent->m_progress->progress() + 1 );
            debug() << "deleting " << (*it).prettyURL() << endl;
            //KIO::del( *it, false, false );
            KIO::file_delete( *it, false );
        }
        lock( true );
        deleteFromIPod( 0 );
        unlock();
        syncIPod();
    }
    QTimer::singleShot( 1500, m_parent->m_progress, SLOT(hide()) );
    m_parent->m_stats->setText( i18n( "1 track in queue", "%n tracks in queue", m_parent->m_transferList->childCount() ) );
#else
    (void)urls;
#endif
}


void
GpodMediaDevice::connectDevice() //SLOT
{
#ifdef HAVE_LIBGPOD
    if ( m_parent->m_connectButton->isOn() )
    {
        if ( !m_mntcmd.isEmpty() )
        {
            mount();
        }

        openIPod();
        m_parent->m_deviceList->renderView( 0 );

        if( isConnected() || m_parent->m_deviceList->childCount() != 0 )
        {
            m_parent->m_connectButton->setOn( true );
            if ( m_parent->m_transferList->childCount() != 0 )
            {
                m_parent->m_transferButton->setEnabled( true );
                m_parent->m_stats->setText( i18n( "Checking device for duplicate files." ) );
                KURL::List urls;
                for( MediaItem *cur = dynamic_cast<MediaItem *>(m_parent->m_transferList->firstChild());
                        cur != NULL;
                        cur = dynamic_cast<MediaItem *>(cur->nextSibling()) )
                {
                    urls.append( cur->url() );
                }
                clearItems();
                addURLs( urls );
            }
        }
        else
        {
            m_parent->m_connectButton->setOn( false );
            KMessageBox::error( m_parent->m_parent,
                    i18n( "Could not find device, please mount it and try again." ),
                    i18n( "Media Device Browser" ) );
        }
    }
    else
    {
        if ( m_parent->m_transferList->childCount() != 0 &&  isConnected() )
        {
            int button = KMessageBox::warningContinueCancel( m_parent->m_parent,
                    i18n( "There are tracks queued for transfer."
                        " Would you like to transfer them before disconnecting?"),
                    i18n( "Media Device Browser" ),
                    KGuiItem(i18n("&Transfer"),"rebuild") );

            if ( button == KMessageBox::Continue )
                transferFiles();
        }

        fileTransferFinished();
        closeIPod();
        QString text = i18n( "Your device is now in sync, please unmount it and disconnect now." );

        if ( !m_umntcmd.isEmpty() )
        {
            umount();
            text=i18n( "Your device is now in sync, you can disconnect now." );
        }

        m_parent->m_deviceList->renderView( 0 );
        m_parent->m_connectButton->setOn( false );
        KMessageBox::information( m_parent->m_parent, text, i18n( "Media Device Browser" ) );
    }
#endif
}

bool
GpodMediaDevice::fileExists( const MetaBundle& bundle )
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

#ifdef HAVE_LIBGPOD
void
GpodMediaDevice::openIPod(bool useDialogs)
{
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
                return;
            }

            // initialize iPod
            m_itdb = itdb_new();
            if(m_itdb==NULL)
            {
                if(useDialogs)
                    KMessageBox::error( m_parent->m_parent, i18n("Failed to initialize iPod mounted at ") + m_mntpnt,
                            i18n( "Media Device Browser" ) );

                return;
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

            path += "/Music";
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
            return;
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
}

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

void
GpodMediaDevice::closeIPod()  //SLOT
{
    debug() << "Syncing IPod!" << endl;

    writeITunesDB();

    m_database.clear();
    itdb_free(m_itdb);
    m_itdb = NULL;

    m_parent->m_deviceList->renderView( 0 );
}

void
GpodMediaDevice::syncIPod()
{
    closeIPod();

    openIPod();

    m_parent->m_deviceList->renderView( 0 );
}

GpodMediaDevice::IpodArtist *
GpodMediaDevice::getArtist(const QString &artist)
{
    return m_database[artist];
}

bool
GpodMediaDevice::deleteArtist(const QString &artistName)
{
    IpodArtist *artist = getArtist(artistName);
    if(!artist)
        return false;

    for(IpodArtist::Iterator it(*artist); it.current(); ++it)
    {
        deleteAlbum(artistName, it.currentKey());
    }

    return true;
}


GpodMediaDevice::IpodAlbum *
GpodMediaDevice::getAlbum(const QString &artist, const QString &album)
{
    IpodArtist *a = getArtist(artist);
    if(!a)
        return NULL;

    return (*a)[album];
}

bool
GpodMediaDevice::deleteAlbum(const QString &artistName, const QString &albumName)
{
    IpodAlbum *album = getAlbum(artistName, albumName);
    if(!album)
        return false;

    for(IpodAlbum::Iterator it(*album); it.current(); ++it)
    {
        deleteTrack(artistName, albumName, it.current());
    }

    return true;
}

Itdb_Track *
GpodMediaDevice::getTitle(const QString &artist, const QString &album, const QString &title)
{
    IpodAlbum *a = getAlbum(artist, album);
    if(!a)
        return NULL;

    return (*a)[title];
}

Itdb_Track *
GpodMediaDevice::newDBTrack(const MetaBundle &bundle)
{
    Itdb_Track *track = itdb_track_new();
    if(!track)
        return NULL;

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

    track->ipod_path = g_strdup( trackpath.latin1() );
    debug() << "on iPod: " << trackpath << endl;

    track->title = g_strdup( bundle.title().isEmpty() ? i18n("Unknown").utf8() : bundle.title().utf8() );
    track->album = g_strdup( bundle.album().isEmpty() ? i18n("Unknown").utf8() : bundle.album().utf8() );
    track->artist = g_strdup( bundle.artist().isEmpty() ? i18n("Unknown").utf8() : bundle.artist().utf8() );
    track->genre = g_strdup( bundle.genre().utf8() );
    track->filetype = g_strdup( (type + "-file").utf8() );
    track->comment = g_strdup( bundle.comment().utf8() );
    track->track_nr = bundle.track().toUInt();
    track->year = bundle.year().toUInt();
    //track->size = 0;
    track->bitrate = bundle.bitrate();
    track->samplerate = bundle.sampleRate();
    track->tracklen = bundle.length()*1000;

    return track;
}

bool
GpodMediaDevice::addDBTrack(Itdb_Track *track, bool isPodcast)
{
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
}

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

bool
GpodMediaDevice::deleteTrack(const QString& artist, const QString& album, Itdb_Track *track)
{
    (void)artist;
    (void)album;

    return removeDBTrack(track);
}

void
GpodMediaDevice::deleteFromIPod( MediaItem* item )
{
    MediaItem* fi;

    if ( !item )
        fi = (MediaItem*)m_parent->m_deviceList->firstChild();
    else
        fi = item;

    while ( fi )
    {
        if ( fi->isSelected() )
        {
            switch ( fi->depth() )
            {
            case 0:
                deleteArtist( fi->text( 0 ) );
                break;
            case 1:
                deleteAlbum( fi->parent()->text( 0 ), fi->text( 0 ) );

                break;
            case 2:
                Itdb_Track *track = getTitle(fi->parent()->parent()->text( 0 ),
                        fi->parent()->text( 0 ), fi->text( 0 ));
                if(track)
                    deleteTrack(fi->parent()->parent()->text( 0 ), fi->parent()->text( 0 ), track);
                break;
            }
        }
        else
        {
            if ( fi->childCount() )
                deleteFromIPod( (MediaItem*)fi->firstChild() );
        }

        fi = (MediaItem*)fi->nextSibling();
    }
}
#endif // HAVE_LIBGPOD

#include "gpodmediadevice.moc"
