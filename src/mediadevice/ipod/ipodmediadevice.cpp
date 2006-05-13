// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2005 Martin Aumueller <aumuell@reserv.at>
// See COPYING file for licensing information

#define DEBUG_PREFIX "IpodMediaDevice"

#include <config.h>
#include "ipodmediadevice.h"

AMAROK_EXPORT_PLUGIN( IpodMediaDevice )

#include <debug.h>
#include <metabundle.h>
#include <collectiondb.h>
#include <statusbar/statusbar.h>
#include <k3bexporter.h>
#include <playlist.h>
#include <collectionbrowser.h>
#include <playlistbrowser.h>
#include <tagdialog.h>
#include <threadweaver.h>
#include <metadata/tplugins.h>
#include <hintlineedit.h>

#include <kapplication.h>
#include <kmountpoint.h>
#include <kpushbutton.h>
#include <kprogress.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <kpopupmenu.h>
#include <krfcdate.h>

#include <qcheckbox.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qregexp.h>
#include <qtimer.h>
#include <qtooltip.h>

#ifdef HAVE_STATVFS
#include <stdint.h>
#include <sys/statvfs.h>
#endif

#include <cstdlib>


// disable if it takes too long for you
#define CHECK_FOR_INTEGRITY 1

#ifndef HAVE_ITDB_GET_MOUNTPOINT
#define itdb_get_mountpoint(x) (x)->mountpoint
#endif



#include "metadata/audible/taglib_audiblefile.h"

struct PodcastInfo
{
    // per show
    QString url;
    QString description;
    QDateTime date;
    QString author;

    // per channel
    QString rss;
};

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

class IpodMediaItem : public MediaItem
{
    public:
        IpodMediaItem(QListView *parent, MediaDevice *dev ) : MediaItem(parent) { init( dev ); }
        IpodMediaItem(QListViewItem *parent, MediaDevice *dev ) : MediaItem(parent) { init( dev ); }
        IpodMediaItem(QListView *parent, QListViewItem *after, MediaDevice *dev ) : MediaItem(parent, after) { init( dev ); }
        IpodMediaItem(QListViewItem *parent, QListViewItem *after, MediaDevice *dev ) : MediaItem(parent, after) { init( dev ); }
        ~IpodMediaItem() { delete m_podcastInfo; }
        void init(MediaDevice *dev) {m_track=0; m_playlist=0; m_device=dev;m_podcastInfo=0;}
        void bundleFromTrack( Itdb_Track *track, const QString& path )
        {
            MetaBundle *bundle = new MetaBundle();
            bundle->setArtist( QString::fromUtf8( track->artist ) );
            bundle->setComposer( QString::fromUtf8( track->composer ) );
            bundle->setAlbum( QString::fromUtf8( track->album ) );
            bundle->setTitle( QString::fromUtf8( track->title ) );
            bundle->setComment( QString::fromUtf8( track->comment ) );
            bundle->setGenre( QString::fromUtf8( track->genre ) );
            bundle->setYear( track->year );
            bundle->setTrack( track->track_nr );
            bundle->setDiscNumber( track->cd_nr );
            bundle->setLength( track->tracklen/1000 );
            bundle->setBitrate( track->bitrate );
            bundle->setSampleRate( track->samplerate );
            bundle->setPath( path );
            bundle->setFilesize( track->size );
            setBundle( bundle );
        }
        Itdb_Track *m_track;
        Itdb_Playlist *m_playlist;
        PodcastInfo *m_podcastInfo;
        int played() const { if(m_track) return m_track->playcount; else return 0; }
        int recentlyPlayed() const { if(m_track) return m_track->recent_playcount; else return 0; }
        int rating() const { if(m_track) return m_track->rating; else return 0; }
        void setRating(int rating) {
            if(m_track) m_track->rating = m_track->app_rating = rating;
            if( dynamic_cast<IpodMediaDevice *>(device()) )
                static_cast<IpodMediaDevice *>(device())->m_dbChanged=true;
        }
        bool ratingChanged() const { if(m_track) return m_track->rating != m_track->app_rating; else return false; }
        QDateTime playTime() const { QDateTime t; if(m_track) t.setTime_t( itdb_time_mac_to_host( m_track->time_played ) ); return t; }
        IpodMediaItem *findTrack(Itdb_Track *track)
        {
            if(m_track == track)
                return this;

            for(IpodMediaItem *it=dynamic_cast<IpodMediaItem *>(firstChild());
                    it;
                    it = dynamic_cast<IpodMediaItem *>(it->nextSibling()))
            {
                IpodMediaItem *found = it->findTrack(track);
                if(found)
                    return found;
            }

            return 0;
        }
};


IpodMediaDevice::IpodMediaDevice()
    : KioMediaDevice()
    , m_masterPlaylist( 0 )
    , m_podcastPlaylist( 0 )
    , m_lockFile( 0 )
{

    registerTaglibPlugins();

    m_dbChanged = false;
    m_itdb = 0;
    m_podcastItem = 0;
    m_staleItem = 0;
    m_orphanedItem = 0;
    m_invisibleItem = 0;
    m_playlistItem = 0;
    m_supportsArtwork = false;
    m_supportsVideo = false;
    m_isShuffle = true;

    m_requireMount = true;
    m_name = "iPod";

    // config stuff
    m_syncStatsCheck = 0;
    m_autoDeletePodcastsCheck = 0;
}

void
IpodMediaDevice::init( MediaBrowser* parent )
{
    KioMediaDevice::init( parent );
}

IpodMediaDevice::~IpodMediaDevice()
{
    if(m_itdb)
        itdb_free(m_itdb);

    m_files.clear();
}

bool
IpodMediaDevice::isConnected()
{
    return (m_itdb != 0);
}

MediaItem *
IpodMediaDevice::insertTrackIntoDB(const QString &pathname, const MetaBundle &bundle, const PodcastInfo *podcastInfo)
{
    return updateTrackInDB( 0, pathname, bundle, podcastInfo );
}

MediaItem *
IpodMediaDevice::updateTrackInDB(IpodMediaItem *item,
        const QString &pathname, const MetaBundle &bundle, const PodcastInfo *podcastInfo)
{
    if( !m_itdb )
        return 0;

    Itdb_Track *track = 0;
    if( item )
        track = item->m_track;
    if( !track )
        track = itdb_track_new();
    if(!track)
    {
        delete item;
        return 0;
    }

    QString type = pathname.section('.', -1).lower();

    track->ipod_path = g_strdup( ipodPath(pathname).latin1() );
    debug() << "on iPod: " << track->ipod_path << ", podcast=" << podcastInfo << endl;

    track->title = g_strdup( bundle.title().utf8() );
    track->album = g_strdup( bundle.album()->utf8() );
    track->artist = g_strdup( bundle.artist()->utf8() );
    track->genre = g_strdup( bundle.genre()->utf8() );
    track->unk208 = 0x01; // for audio
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
    else if(type=="m4v" || type=="mp4v" || type=="mov" || type=="mpg")
    {
        track->filetype = g_strdup( "m4v video" );
        track->unk208 = 0x02; // for videos
    }
    else if(type=="aa")
    {
        track->filetype = g_strdup( "audible" );
        track->flag3 |= 0x01; // remember current position in track

        TagLib::Audible::File f( QFile::encodeName( bundle.url().path() ) );
        TagLib::Audible::Tag *t = f.getAudibleTag();
        if( t )
            track->drm_userid = t->userID();
        // libgpod also tries to set those, but this won't work
        track->unk126 = 0x01;
        track->unk144 = 0x01000029;
    }
    else
    {
        track->filetype = g_strdup( type.utf8() );
    }

    track->composer = g_strdup( bundle.composer()->utf8() );
    track->comment = g_strdup( bundle.comment()->utf8() );
    track->track_nr = bundle.track();
    track->cd_nr = bundle.discNumber();
    track->year = bundle.year();
    track->size = bundle.filesize();
    if( track->size == 0 )
    {
        debug() << "filesize is zero for " << track->ipod_path << ", expect strange problems with your ipod" << endl;
    }
    track->bitrate = bundle.bitrate();
    track->samplerate = bundle.sampleRate();
    track->tracklen = bundle.length()*1000;

    if(podcastInfo)
    {
        //track->flag1 |= 0x02; // artwork flag, handled by libgpod
        track->flag2 |= 0x01; // skip  when shuffling
        track->flag3 |= 0x01; // remember playback position
        track->flag4 |= 0x02; // also show description on iPod
        // FIXME: track->unk176 = 0x00020000; // for podcasts
        QString plaindesc = podcastInfo->description;
        plaindesc.replace( QRegExp("<[^>]*>"), "" );
        track->description = g_strdup( plaindesc.utf8() );
        track->subtitle = g_strdup( plaindesc.utf8() );
        track->podcasturl = g_strdup( podcastInfo->url.utf8() );
        track->podcastrss = g_strdup( podcastInfo->rss.utf8() );
        //track->category = g_strdup( i18n( "Unknown" ) );
        track->time_released = itdb_time_host_to_mac( podcastInfo->date.toTime_t() );
        //track->compilation = 0x01; // this should have made the ipod play a sequence of podcasts
    }
    else
    {
        // FIXME: track->unk176 = 0x00010000; // for non-podcasts

        uint albumID = CollectionDB::instance()->albumID( bundle.album(), false );
        if( CollectionDB::instance()->albumIsCompilation( QString::number( albumID ) ) )
        {
            track->compilation = 0x01;
        }
        else
        {
            track->compilation = 0x00;
        }
    }

    m_dbChanged = true;

#ifdef HAVE_ITDB_TRACK_SET_THUMBNAILS
    if( m_supportsArtwork )
    {
        QString image;
        if( bundle.podcastBundle() )
        {
            PodcastChannelBundle pcb;
            if( CollectionDB::instance()->getPodcastChannelBundle( bundle.podcastBundle()->parent(), &pcb ) )
                image = CollectionDB::instance()->podcastImage( pcb.imageURL().url(), 0 );
        }
        if( image.isEmpty() )
            image  = CollectionDB::instance()->albumImage(bundle.artist(), bundle.album(), 0);
        if( !image.endsWith( "@nocover.png" ) )
        {
            debug() << "adding image " << image << " to " << bundle.artist() << ":" << bundle.album() << endl;
            itdb_track_set_thumbnails( track, g_strdup( QFile::encodeName(image) ) );
        }
    }
#endif

    if( item )
    {
        MediaItem *parent = dynamic_cast<MediaItem *>(item->parent());
        if( parent )
        {
            parent->takeItem( item );
            if( parent->childCount() == 0 && !isSpecialItem( parent ) )
            {
                MediaItem *pp = dynamic_cast<MediaItem *>(parent->parent());
                delete parent;
                if( pp && pp->childCount() == 0 && !isSpecialItem( pp ) )
                    delete pp;
            }
        }
    }
    else
    {
        itdb_track_add(m_itdb, track, -1);
        if(podcastInfo)
        {
            Itdb_Playlist *podcasts = itdb_playlist_podcasts(m_itdb);
            if(!podcasts)
            {
                podcasts = itdb_playlist_new("Podcasts", false);
                itdb_playlist_add(m_itdb, podcasts, -1);
                itdb_playlist_set_podcasts(podcasts);
                addPlaylistToView( podcasts );
            }
            itdb_playlist_add_track(podcasts, track, -1);
        }
        else
        {
            // gtkpod 0.94 does not like if not all songs in the db are on the master playlist
            // but we try anyway
            Itdb_Playlist *mpl = itdb_playlist_mpl(m_itdb);
            if( !mpl )
            {
                mpl = itdb_playlist_new( "MPL", false );
                itdb_playlist_add( m_itdb, mpl, -1 );
                itdb_playlist_set_mpl( mpl );
                addPlaylistToView( mpl );
            }
            itdb_playlist_add_track(mpl, track, -1);
        }
    }

    return addTrackToView( track, item );
}

MediaItem *
IpodMediaDevice::copyTrackToDevice(const MetaBundle &bundle)
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
        while( !path.isEmpty() && !(path==mountPoint()) && !parentdir.exists() );
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

    if( !kioCopyTrack( bundle.url(), url ) )
    {
        return NULL;
    }

    PodcastInfo *podcastInfo = 0;
    if( bundle.podcastBundle() )
    {
        PodcastEpisodeBundle *peb = bundle.podcastBundle();
        podcastInfo = new PodcastInfo;
        podcastInfo->url = peb->url().url();
        podcastInfo->description = peb->description();
        podcastInfo->author = peb->author();
        podcastInfo->rss = peb->parent().url();
        if( !peb->date().isEmpty()  )
            podcastInfo->date.setTime_t( KRFCDate::parseDate( peb->date() ) );
    }

    MediaItem *ret = insertTrackIntoDB( url.path(), bundle, podcastInfo );
    delete podcastInfo;
    return ret;
}

MediaItem *
IpodMediaDevice::tagsChanged( MediaItem *item, const MetaBundle &bundle )
{
    return updateTrackInDB( dynamic_cast<IpodMediaItem *>(item), item->url().path(), bundle, NULL );
}

void
IpodMediaDevice::synchronizeDevice()
{
    debug() << "Syncing iPod!" << endl;
    writeITunesDB();
}

MediaItem *
IpodMediaDevice::trackExists( const MetaBundle& bundle )
{
    IpodMediaItem *item = getTrack( bundle.artist(),
            bundle.album(),
            bundle.title(),
            bundle.track() );

    return item;
}

MediaItem *
IpodMediaDevice::newPlaylist(const QString &name, MediaItem *parent, QPtrList<MediaItem> items)
{
    m_dbChanged = true;
    IpodMediaItem *item = new IpodMediaItem(parent, this);
    item->setType(MediaItem::PLAYLIST);
    item->setText(0, name);

    addToPlaylist(item, 0, items);

    return item;
}


void
IpodMediaDevice::addToPlaylist(MediaItem *mlist, MediaItem *after, QPtrList<MediaItem> items)
{
    IpodMediaItem *list = dynamic_cast<IpodMediaItem *>(mlist);
    if(!list)
        return;

    m_dbChanged = true;

    if(list->m_playlist)
    {
        itdb_playlist_remove(list->m_playlist);
        list->m_playlist = 0;
    }

    int order;
    IpodMediaItem *it;
    if(after)
    {
        order = after->m_order + 1;
        it = dynamic_cast<IpodMediaItem*>(after->nextSibling());
    }
    else
    {
        order = 0;
        it = dynamic_cast<IpodMediaItem*>(list->firstChild());
    }

    for( ; it; it = dynamic_cast<IpodMediaItem *>(it->nextSibling()))
    {
        it->m_order += items.count();
    }

    for(IpodMediaItem *it = dynamic_cast<IpodMediaItem *>(items.first());
            it;
            it = dynamic_cast<IpodMediaItem *>(items.next()) )
    {
        if(!it->m_track)
            continue;

        IpodMediaItem *add;
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
                add = new IpodMediaItem(list, after, this);
            }
            else
            {
                add = new IpodMediaItem(list, this);
            }
        }
        after = add;

        add->setType(MediaItem::PLAYLISTITEM);
        add->m_track = it->m_track;
        MetaBundle *bundle;
        if( it->bundle() )
            bundle = new MetaBundle( *it->bundle() );
        else
            bundle = new MetaBundle();
        add->setBundle( bundle );
        add->setText(0, QString::fromUtf8(it->m_track->artist) + " - " + QString::fromUtf8(it->m_track->title) );
        add->m_order = order;
        order++;
    }

    // make numbering consecutive
    int i=0;
    for(IpodMediaItem *it = dynamic_cast<IpodMediaItem *>(list->firstChild());
            it;
            it = dynamic_cast<IpodMediaItem *>(it->nextSibling()))
    {
        it->m_order = i;
        i++;
    }

    playlistFromItem(list);
}

int
IpodMediaDevice::deleteItemFromDevice(MediaItem *mediaitem, bool onlyPlayed )
{
    IpodMediaItem *item = dynamic_cast<IpodMediaItem *>(mediaitem);
    if(!item)
        return -1;

    if( isCancelled() )
        return 0;

    if( !item->isVisible() )
        return 0;

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
            for( IpodMediaItem *it = static_cast<IpodMediaItem *>(m_playlistItem)->findTrack(track);
                    it;
                    it = static_cast<IpodMediaItem *>(m_playlistItem)->findTrack(track) )
            {
                delete it;
            }

            // delete all other occurences
            for( IpodMediaItem *it = getTrack( track );
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
            IpodMediaItem *next = 0;
            for(IpodMediaItem *it = dynamic_cast<IpodMediaItem *>(item->firstChild());
                    it;
                    it = next)
            {
                if( isCancelled() )
                    break;

                next = dynamic_cast<IpodMediaItem *>(it->nextSibling());
                int ret = deleteItemFromDevice(it, onlyPlayed);
                if( ret >= 0 && count >= 0 )
                    count += ret;
                else
                    count = -1;
            }
        }
        if(item->type() == MediaItem::PLAYLIST && !isCancelled())
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
                else
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
IpodMediaDevice::createLockFile( const QString &mountpoint, bool silent )
{
    m_lockFile = new QFile( QFile::encodeName(mountpoint + "/iPod_Control/iTunes/iTunesLock") );
    QString msg;
    bool ok = true;
    if( m_lockFile->exists() )
    {
        ok = false;
        msg = i18n( "Media Device: iPod mounted at %1 already locked! " ).arg( mountpoint );
        msg += i18n( "If you are sure that this is an error, then remove the file %1 and try again." )
           .arg( mountpoint + "/iPod_Control/iTunes/iTunesLock" );

        if( !silent )
        {
            if( KMessageBox::warningContinueCancel( m_parent, msg, i18n( "Remove iTunes Lock File?" ),
                        KGuiItem(i18n("&Remove"), "editdelete"), QString::null, KMessageBox::Dangerous )
                    == KMessageBox::Continue )
            {
                msg = i18n( "Media Device: removing lockfile %1 failed: %2! " )
                    .arg( mountpoint + "/iPod_Control/iTunes/iTunesLock", m_lockFile->errorString() );
                ok = m_lockFile->remove();
            }
            else
            {
                msg = "";
            }
        }
    }

    if( ok && !m_lockFile->open( IO_WriteOnly ) )
    {
        ok = false;
        msg = i18n( "Media Device: failed to create lockfile on iPod mounted at %1: %2" )
            .arg(mountpoint, m_lockFile->errorString());
    }

    if( ok )
        return true;

    delete m_lockFile;
    m_lockFile = 0;

    if( !msg.isEmpty() )
        amaroK::StatusBar::instance()->longMessage( msg, KDE::StatusBar::Sorry );
    return false;
}

bool
IpodMediaDevice::initializeIpod( const QString &mountpoint )
{
    QDir dir( mountpoint );
    if( !dir.exists() )
    {
        amaroK::StatusBar::instance()->longMessage(
                i18n("Media device: Mount point %1 does not exist").arg(mountpoint),
                KDE::StatusBar::Error );
        return false;
    }

    debug() << "initializing iPod mounted at " << mountpoint << endl;

    // initialize iPod
    m_itdb = itdb_new();
    if( m_itdb == 0 )
        return false;

    itdb_set_mountpoint(m_itdb, QFile::encodeName(mountpoint));

    Itdb_Playlist *mpl = itdb_playlist_new("iPod", false);
    itdb_playlist_set_mpl(mpl);
    Itdb_Playlist *podcasts = itdb_playlist_new("Podcasts", false);
    itdb_playlist_set_podcasts(podcasts);
    itdb_playlist_add(m_itdb, podcasts, -1);
    itdb_playlist_add(m_itdb, mpl, 0);

    QString path = mountpoint + "/iPod_Control";
    dir.setPath(path);
    if(!dir.exists())
        dir.mkdir(dir.absPath());
    if(!dir.exists())
        return false;


    path = mountpoint + "/iPod_Control/Music";
    dir.setPath(path);
    if(!dir.exists())
        dir.mkdir(dir.absPath());
    if(!dir.exists())
        return false;

    path = mountpoint + "/iPod_Control/iTunes";
    dir.setPath(path);
    if(!dir.exists())
        dir.mkdir(dir.absPath());
    if(!dir.exists())
        return false;

    if( !writeITunesDB( false ) )
        return false;

    amaroK::StatusBar::instance()->longMessage(
            i18n("Media Device: Initialized iPod mounted at %1").arg(mountpoint),
            KDE::StatusBar::Information );

    return true;
}

bool
IpodMediaDevice::openDevice( bool silent )
{
    m_isShuffle = true;
    m_supportsArtwork = false;
    m_supportsVideo = false;
    m_dbChanged = false;
    m_files.clear();

    if( m_itdb )
    {
        amaroK::StatusBar::instance()->longMessage(
                i18n("Media Device: iPod at %1 already opened").arg(mountPoint()),
                KDE::StatusBar::Sorry );
        return false;
    }

    // try to find a mounted ipod
    bool ipodFound = false;
    KMountPoint::List currentmountpoints = KMountPoint::currentMountPoints();
    KMountPoint::List::Iterator mountiter = currentmountpoints.begin();
    for(; mountiter != currentmountpoints.end(); ++mountiter)
    {
        QString devicenode = (*mountiter)->mountedFrom();
        QString mountpoint = (*mountiter)->mountPoint();

        if( !deviceNode().isEmpty() )
        {
            if( devicenode != deviceNode() )
                continue;
        }
        else if( !mountPoint().isEmpty() )
        {
            if( mountPoint() != mountpoint )
                continue;
        }
        else
        {
            GError *err = 0;
            m_itdb = itdb_parse(QFile::encodeName(mountpoint), &err);
            if(err)
            {
                g_error_free(err);
                if( m_itdb )
                {
                    itdb_free( m_itdb );
                    m_itdb = 0;
                }
                continue;
            }
            itdb_free( m_itdb );
            m_itdb = 0;
        }

        if( mountPoint().isEmpty() )
            m_medium.setMountPoint( mountpoint );
        ipodFound = true;
        break;
    }

    if( !ipodFound )
    {
        if( !silent )
        {
            amaroK::StatusBar::instance()->longMessage(
                    i18n("Media Device: No mounted iPod found" ),
                    KDE::StatusBar::Sorry );
        }
        return false;
    }

    GError *err = 0;
    m_itdb = itdb_parse(QFile::encodeName(mountPoint()), &err);
    if(err)
    {
        g_error_free(err);
        if( m_itdb )
        {
            itdb_free( m_itdb );
            m_itdb = 0;
        }
    }

    if( !m_itdb )
    {
        QString msg = i18n( "Media Device: could not find iTunesDB on device mounted at %1. "
                "Should I try to initialize your iPod?" ).arg( mountPoint() );

        if( !silent
                && KMessageBox::warningContinueCancel( m_parent, msg, i18n( "Initialize iPod?" ),
                    KGuiItem(i18n("&Initialize"), "new") ) == KMessageBox::Continue )
        {
            if( !initializeIpod( mountPoint() ) )
            {
                if( m_itdb )
                {
                    itdb_free( m_itdb );
                    m_itdb = 0;
                }

                amaroK::StatusBar::instance()->longMessage(
                        i18n("Media Device: Failed to initialize iPod mounted at %1").arg(mountPoint()),
                        KDE::StatusBar::Sorry );

                return false;
            }
        }
        else
           return false;
    }

    if( !createLockFile( mountPoint(), silent ) )
    {
        if( m_itdb )
        {
            itdb_free( m_itdb );
            m_itdb = 0;
        }
        return false;
    }

#if 0
    // does not work any longer in libgpod-cvs (0.3.3)
    if( m_itdb->device )
    {
        guint model;
        gchar *modelString;
        gchar *name;
        g_object_get(m_itdb->device,
                "device-model", &model,
                "device-model-string", &modelString,
                "device-name", &name,
                NULL); // 0 -> warning about missing sentinel

        switch(model)
        {
        case MODEL_TYPE_COLOR:
        case MODEL_TYPE_COLOR_U2:
        case MODEL_TYPE_NANO_WHITE:
        case MODEL_TYPE_NANO_BLACK:
            m_supportsArtwork = true;
            m_isShuffle = false;
            debug() << "detected iPod photo/nano" << endl;
            break;
        case MODEL_TYPE_VIDEO_WHITE:
        case MODEL_TYPE_VIDEO_BLACK:
            m_supportsArtwork = true;
            m_isShuffle = false;
            m_supportsVideo = true;
            debug() << "detected iPod video" << endl;
            break;
        case MODEL_TYPE_REGULAR:
        case MODEL_TYPE_REGULAR_U2:
        case MODEL_TYPE_MINI:
        case MODEL_TYPE_MINI_BLUE:
        case MODEL_TYPE_MINI_PINK:
        case MODEL_TYPE_MINI_GREEN:
        case MODEL_TYPE_MINI_GOLD:
            m_supportsArtwork = false;
            m_isShuffle = false;
            debug() << "detected regular iPod (b/w display)" << endl;
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
            m_isShuffle = true;
            debug() << "unknown type" << endl;
            break;
        }

        m_name = name != NULL ?
            QString( "iPod %1 \"%2\"" )
            .arg( QString::fromUtf8( modelString ) )
            .arg( QString::fromUtf8( name ) )
            :
            QString( "iPod %1" )
            .arg( QString::fromUtf8( modelString ) );
    }
    else
    {
        debug() << "device type detection failed, assuming iPod shuffle" << endl;
        amaroK::StatusBar::instance()->shortMessage( i18n("Media device: device type detection failed, assuming iPod shuffle") );
    }
#endif

    m_isShuffle = true;
    m_supportsArtwork = true;

    for( int i=0; i < itdb_musicdirs_number(m_itdb); i++)
    {
        QString ipod;
        ipod.sprintf( ":iPod_Control:Music:f%02d", i );
        QString real = realPath( ipod.latin1() );
        QDir dir( real );
        if( !dir.exists() )
        {
            ipod.sprintf( ":iPod_Control:Music:F%02d", i );
            real = realPath( ipod.latin1() );
            dir.setPath( real );
            if( !dir.exists() )
            {
                dir.mkdir( real );
                dir.setPath( real );
                if( !dir.exists() )
                {
                    debug() << "failed to create hash dir " << real << endl;
                    amaroK::StatusBar::instance()->longMessage(
                            i18n("Media device: Failed to create directory %1").arg(real),
                            KDE::StatusBar::Error );
                    return false;
                }
            }
        }
    }

    m_playlistItem = new IpodMediaItem( m_view, this );
    m_playlistItem->setText( 0, i18n("Playlists") );
    m_playlistItem->m_order = -6;
    m_playlistItem->setType( MediaItem::PLAYLISTSROOT );

    m_podcastItem = new IpodMediaItem( m_view, this );
    m_podcastItem->setText( 0, i18n("Podcasts") );
    m_podcastItem->m_order = -5;
    m_podcastItem->setType( MediaItem::PODCASTSROOT );

    m_invisibleItem = new IpodMediaItem( m_view, this );
    m_invisibleItem->setText( 0, i18n("Invisible") );
    m_invisibleItem->m_order = -4;
    m_invisibleItem->setType( MediaItem::INVISIBLEROOT );

    m_staleItem = new IpodMediaItem( m_view, this );
    m_staleItem->setText( 0, i18n("Stale") );
    m_staleItem->m_order = -3;
    m_staleItem->setType( MediaItem::STALEROOT );

    m_orphanedItem = new IpodMediaItem( m_view, this );
    m_orphanedItem->setText( 0, i18n("Orphaned") );
    m_orphanedItem->m_order = -2;
    m_orphanedItem->setType( MediaItem::ORPHANEDROOT );

    if( !silent )
        kapp->processEvents( 100 );

    GList *cur = m_itdb->playlists;
    while(cur)
    {
        Itdb_Playlist *playlist = (Itdb_Playlist *)cur->data;

        addPlaylistToView(playlist);

        cur = cur->next;
    }

    if( !silent )
        kapp->processEvents( 100 );

    cur = m_itdb->tracks;
    while(cur)
    {
        Itdb_Track *track = (Itdb_Track *)cur->data;

        addTrackToView(track);

        cur = cur->next;
    }

#ifdef CHECK_FOR_INTEGRITY
    QString musicpath = QString(itdb_get_mountpoint(m_itdb)) + "/iPod_Control/Music";
    QDir dir( musicpath, QString::null, QDir::Name | QDir::IgnoreCase, QDir::Dirs );
    for(unsigned i=0; i<dir.count(); i++)
    {
        if(dir[i] == "." || dir[i] == "..")
            continue;

        if( !silent )
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
                IpodMediaItem *item = new IpodMediaItem(m_orphanedItem, this);
                item->setType(MediaItem::ORPHANED);
                KURL url = KURL::fromPathOrURL(filename);
                MetaBundle *bundle = new MetaBundle(url);
                item->setBundle( bundle );
                QString title = bundle->artist() + " - " + bundle->title();
                item->setText(0, title);
            }
        }
    }
#endif // CHECK_FOR_INTEGRITY
    if( !silent )
        kapp->processEvents( 100 );

    updateRootItems();

    return true;
}

bool
IpodMediaDevice::closeDevice()  //SLOT
{
    writeITunesDB();

    m_view->clear();
    m_podcastItem = 0;
    m_playlistItem = 0;
    m_orphanedItem = 0;
    m_staleItem = 0;
    m_invisibleItem = 0;

    if( m_lockFile )
    {
        m_lockFile->remove();
        m_lockFile->close();
        delete m_lockFile;
        m_lockFile = 0;
    }

    m_files.clear();
    itdb_free(m_itdb);
    m_itdb = 0;
    m_masterPlaylist = 0;
    m_podcastPlaylist = 0;

    m_name = "iPod";

    return true;
}

void
IpodMediaDevice::renameItem( QListViewItem *i ) // SLOT
{
    IpodMediaItem *item = dynamic_cast<IpodMediaItem *>(i);
    if(!item)
        return;

    if(!item->type() == MediaItem::PLAYLIST)
        return;

    m_dbChanged = true;

    g_free(item->m_playlist->name);
    item->m_playlist->name = g_strdup( item->text( 0 ).utf8() );
}

void
IpodMediaDevice::playlistFromItem(IpodMediaItem *item)
{
    if( !m_itdb )
        return;

    m_dbChanged = true;

    item->m_playlist = itdb_playlist_new(item->text(0).utf8(), false /* dumb playlist */ );
    itdb_playlist_add(m_itdb, item->m_playlist, -1);
    for(IpodMediaItem *it = dynamic_cast<IpodMediaItem *>(item->firstChild());
            it;
            it = dynamic_cast<IpodMediaItem *>(it->nextSibling()) )
    {
        itdb_playlist_add_track(item->m_playlist, it->m_track, -1);
        it->m_playlist = item->m_playlist;
    }
}


IpodMediaItem *
IpodMediaDevice::addTrackToView(Itdb_Track *track, IpodMediaItem *item)
{
    bool visible = false;
    bool stale = false;

#ifdef CHECK_FOR_INTEGRITY
    QString path = realPath(track->ipod_path);
    QFileInfo finfo(path);
    if(!finfo.exists())
    {
        stale = true;
        debug() << "track: " << track->artist << " - " << track->album << " - " << track->title << " is stale: " << track->ipod_path << " does not exist" << endl;
        if( item )
            m_staleItem->insertItem( item );
        else
            item = new IpodMediaItem(m_staleItem, this);
        item->setType(MediaItem::STALE);
        QString title = QString::fromUtf8(track->artist) + " - "
            + QString::fromUtf8(track->title);
        item->setText( 0, title );
        item->m_track = track;
    }
    else
    {
        m_files.insert( QString(track->ipod_path).lower(), track );
    }
#endif // CHECK_FOR_INTEGRITY

    if(!stale && m_masterPlaylist && itdb_playlist_contains_track(m_masterPlaylist, track))
    {
        visible = true;

        QString artistName;
        if( track->compilation )
            artistName = i18n( "Various Artists" );
        else
            artistName = QString::fromUtf8(track->artist);

        IpodMediaItem *artist = getArtist(artistName);
        if(!artist)
        {
            artist = new IpodMediaItem(m_view, this);
            artist->setText( 0, artistName );
            artist->setType( MediaItem::ARTIST );
            if( artistName == i18n( "Various Artists" ) )
                artist->m_order = -1;
        }

        QString albumName(QString::fromUtf8(track->album));
        MediaItem *album = artist->findItem(albumName);
        if(!album)
        {
            album = new IpodMediaItem( artist, this );
            album->setText( 0, albumName );
            album->setType( MediaItem::ALBUM );
        }

        if( item )
            album->insertItem( item );
        else
        {
            item = new IpodMediaItem( album, this );
        }
        QString titleName = QString::fromUtf8(track->title);
        if( track->compilation )
            item->setText( 0, QString::fromUtf8(track->artist) + i18n( " - " ) + titleName );
        else
            item->setText( 0, titleName );
        item->setType( MediaItem::TRACK );
        item->m_track = track;
        item->bundleFromTrack( track, realPath(track->ipod_path) );
        item->m_order = track->track_nr;
    }

    if(!stale && m_podcastPlaylist && itdb_playlist_contains_track(m_podcastPlaylist, track))
    {
        visible = true;

        QString channelName(QString::fromUtf8(track->album));
        IpodMediaItem *channel = dynamic_cast<IpodMediaItem *>(m_podcastItem->findItem(channelName));
        if(!channel)
        {
            channel = new IpodMediaItem(m_podcastItem, this);
            channel->setText( 0, channelName );
            channel->setType( MediaItem::PODCASTCHANNEL );
            channel->m_podcastInfo = new PodcastInfo;
        }

        if( item )
            channel->insertItem( item );
        else
            item = new IpodMediaItem(channel, this);
        item->setText( 0, QString::fromUtf8(track->title) );
        item->setType( MediaItem::PODCASTITEM );
        item->m_track = track;
        item->bundleFromTrack( track, realPath(track->ipod_path) );

        PodcastInfo *info = new PodcastInfo;
        item->m_podcastInfo = info;
        info->url = QString::fromUtf8( track->podcasturl );
        info->rss = QString::fromUtf8( track->podcastrss );
        info->description = QString::fromUtf8( track->description );
        info->date.setTime_t( itdb_time_mac_to_host( track->time_released) );

        if( !info->rss.isEmpty() && channel->m_podcastInfo->rss.isEmpty() )
        {
           channel->m_podcastInfo->rss = info->rss;
        }
    }

    if(!stale && !visible)
    {
        debug() << "invisible, title=" << track->title << endl;
        if( item )
            m_invisibleItem->insertItem( item );
        else
            item = new IpodMediaItem(m_invisibleItem, this);
        QString title = QString::fromUtf8(track->artist) + " - "
            + QString::fromUtf8(track->title);
        item->setText( 0, title );
        item->setType( MediaItem::INVISIBLE );
        item->m_track = track;
        item->bundleFromTrack( track, realPath(track->ipod_path) );
    }

    updateRootItems();

    return item;
}

void
IpodMediaDevice::addPlaylistToView(Itdb_Playlist *pl)
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
    IpodMediaItem *playlist = dynamic_cast<IpodMediaItem *>(m_playlistItem->findItem(name));
    if(!playlist)
    {
        playlist = new IpodMediaItem( m_playlistItem, this );
        playlist->setText( 0, name );
        playlist->setType( MediaItem::PLAYLIST );
        playlist->m_playlist = pl;
    }

    int i=0;
    GList *cur = pl->members;
    while(cur)
    {
        Itdb_Track *track = (Itdb_Track *)cur->data;
        IpodMediaItem *item = new IpodMediaItem(playlist, this);
        QString title = QString::fromUtf8(track->artist) + " - "
            + QString::fromUtf8(track->title);
        item->setText( 0, title );
        item->setType( MediaItem::PLAYLISTITEM );
        item->m_playlist = pl;
        item->m_track = track;
        item->bundleFromTrack( track, realPath(track->ipod_path) );
        item->m_order = i;

        cur = cur->next;
        i++;
    }
}

QString
IpodMediaDevice::realPath(const char *ipodPath)
{
    QString path;
    if(m_itdb)
    {
        path = QFile::decodeName(itdb_get_mountpoint(m_itdb));
        path.append(QString(ipodPath).replace(':', "/"));
    }

    return path;
}

QString
IpodMediaDevice::ipodPath(const QString &realPath)
{
    if(m_itdb)
    {
        QString mp = QFile::decodeName(itdb_get_mountpoint(m_itdb));
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

class IpodWriteDBJob : public ThreadWeaver::DependentJob
{
    public:
        IpodWriteDBJob( QObject *parent, Itdb_iTunesDB *itdb, bool isShuffle, bool *resultPtr )
        : ThreadWeaver::DependentJob( parent, "IpodWriteDBJob" )
        , m_itdb( itdb )
        , m_isShuffle( isShuffle )
        , m_resultPtr( resultPtr )
        , m_return( true )
        {}

    private:
        virtual bool doJob()
        {
            if( !m_itdb )
            {
                m_return = false;
            }

            GError *error = 0;
            if (m_return && !itdb_write (m_itdb, &error))
            {   /* an error occured */
                m_return = false;
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

            if( m_return && m_isShuffle )
            {
                /* write shuffle data */
                if (!itdb_shuffle_write (m_itdb, &error))
                {   /* an error occured */
                    m_return = false;
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

            return true;
        }

        virtual void completeJob()
        {
            *m_resultPtr = m_return;
        }

        Itdb_iTunesDB *m_itdb;
        bool m_isShuffle;
        bool *m_resultPtr;
        bool m_return;
};

bool
IpodMediaDevice::writeITunesDB( bool threaded )
{
    if(m_itdb)
        m_dbChanged = true; // write unconditionally for resetting recent_playcount

    if(m_dbChanged)
    {
        bool ok = false;
        if( !threaded || MediaBrowser::instance()->isQuitting() )
        {
            if( !m_itdb )
            {
                return false;
            }

            ok = true;
            GError *error = 0;
            if ( !itdb_write (m_itdb, &error) )
            {   /* an error occured */
                if(error)
                {
                    if (error->message)
                        debug() << "itdb_write error: " << error->message << endl;
                    else
                        debug() << "itdb_write error: " << "error->message == 0!" << endl;
                    g_error_free (error);
                }
                error = 0;
                ok = false;
            }

            if( m_isShuffle )
            {
                /* write shuffle data */
                if (!itdb_shuffle_write (m_itdb, &error))
                {   /* an error occured */
                    if(error)
                    {
                        if (error->message)
                            debug() << "itdb_shuffle_write error: " << error->message << endl;
                        else
                            debug() << "itdb_shuffle_write error: " << "error->message == 0!" << endl;
                        g_error_free (error);
                    }
                    error = 0;
                    ok = false;
                }
            }
        }
        else
        {
            ThreadWeaver::instance()->queueJob( new IpodWriteDBJob( this, m_itdb, m_isShuffle, &ok ) );
            while( ThreadWeaver::instance()->isJobPending( "IpodWriteDBJob" ) )
            {
                kapp->processEvents();
                usleep( 10000 );
            }
        }

        if( ok )
        {
            m_dbChanged = false;
        }
        else
        {
            amaroK::StatusBar::instance()->longMessage(
                    i18n("Media device: failed to write iPod database"),
                    KDE::StatusBar::Error );
        }

        return ok;
    }
    return true;
}


IpodMediaItem *
IpodMediaDevice::getArtist(const QString &artist)
{
    for(IpodMediaItem *it = dynamic_cast<IpodMediaItem *>(m_view->firstChild());
            it;
            it = dynamic_cast<IpodMediaItem *>(it->nextSibling()))
    {
        if(it->m_type==MediaItem::ARTIST && artist == it->text(0))
            return it;
    }

    return 0;
}

IpodMediaItem *
IpodMediaDevice::getAlbum(const QString &artist, const QString &album)
{
    IpodMediaItem *item = getArtist(artist);
    if(item)
        return dynamic_cast<IpodMediaItem *>(item->findItem(album));

    return 0;
}

IpodMediaItem *
IpodMediaDevice::getTrack(const QString &artist, const QString &album, const QString &title, int trackNumber)
{
    IpodMediaItem *item = getAlbum(artist, album);
    if(item)
    {
        for( IpodMediaItem *track = dynamic_cast<IpodMediaItem *>(item->findItem(title));
                track;
                track = dynamic_cast<IpodMediaItem *>(item->findItem(title, track)) )
        {
            if( trackNumber==-1 || track->bundle()->track() == trackNumber )
                return track;
        }
    }

    item = getAlbum( i18n( "Various Artists" ), album );
    if( item )
    {
        QString t = artist + i18n(" - ") + title;
        for( IpodMediaItem *track = dynamic_cast<IpodMediaItem *>(item->findItem(t));
                track;
                track = dynamic_cast<IpodMediaItem *>(item->findItem(t, track)) )
        {
            if( trackNumber==-1 || track->bundle()->track() == trackNumber )
                return track;
        }
    }

    if(m_podcastItem)
    {
        item = dynamic_cast<IpodMediaItem *>(m_podcastItem->findItem(album));
        if(item)
        {
            for( IpodMediaItem *track = dynamic_cast<IpodMediaItem *>(item->findItem(title));
                    track;
                    track = dynamic_cast<IpodMediaItem *>(item->findItem(title, track)) )
            {
                if( trackNumber==-1 || track->bundle()->track() == trackNumber )
                    return track;
            }
        }
    }

    return 0;
}

IpodMediaItem *
IpodMediaDevice::getTrack( const Itdb_Track *itrack )
{
    QString artist = QString::fromUtf8( itrack->artist );
    QString album = QString::fromUtf8( itrack->album );
    QString title = QString::fromUtf8( itrack->title );

    IpodMediaItem *item = getAlbum( artist, album );
    if(item)
    {
        for( IpodMediaItem *track = dynamic_cast<IpodMediaItem *>(item->findItem( title ) );
                track;
                track = dynamic_cast<IpodMediaItem *>(item->findItem(title, track)) )
        {
            if( track->m_track == itrack )
                return track;
        }
    }

    if(m_podcastItem)
    {
        item = dynamic_cast<IpodMediaItem *>(m_podcastItem->findItem(album));
        if(item)
        {
            for( IpodMediaItem *track = dynamic_cast<IpodMediaItem *>(item->findItem(title));
                    track;
                    track = dynamic_cast<IpodMediaItem *>(item->findItem(title, track)) )
            {
                if( track->m_track == itrack )
                    return track;
            }
        }
    }

    return 0;
}


KURL
IpodMediaDevice::determineURLOnDevice(const MetaBundle &bundle)
{
    if( !m_itdb )
    {
        debug() << "m_itdb is NULL" << endl;
        return KURL();
    }

    QString local = bundle.filename();
    QString type = local.section('.', -1);

    QString trackpath;
    bool exists = false;
    do
    {
        int num = std::rand() % 1000000;
        int music_dirs = itdb_musicdirs_number(m_itdb) > 0 ? itdb_musicdirs_number(m_itdb) : 20; 
        int dir = num % music_dirs;
        QString dirname;
        dirname.sprintf( ":iPod_Control:Music:f%02d", dir );
        QString realdir = realPath(dirname.latin1());
        QDir qdir( realdir );
        if( !qdir.exists() )
        {
            dirname.sprintf( ":iPod_Control:Music:F%02d", dir );
            realdir = realPath(dirname.latin1());
            qdir.setPath( realdir );
            if( !qdir.exists() )
            {
                qdir.mkdir( realdir );
            }
        }
        QString filename;
        filename.sprintf( ":kpod%07d.%s", num, type.latin1() );
        trackpath = dirname + filename;
        QFileInfo finfo(realPath(trackpath.latin1()));
        exists = finfo.exists();
    }
    while(exists);

    return realPath(trackpath.latin1());
}

bool
IpodMediaDevice::removeDBTrack(Itdb_Track *track)
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
IpodMediaDevice::getCapacity( KIO::filesize_t *total, KIO::filesize_t *available )
{
    if(!m_itdb)
        return false;

#ifdef HAVE_STATVFS
    QString path = QFile::decodeName(itdb_get_mountpoint(m_itdb));
    path.append("/iPod_Control");
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
    return false;
#endif
}

void
IpodMediaDevice::rmbPressed( QListViewItem* qitem, const QPoint& point, int )
{
    MediaItem *item = dynamic_cast<MediaItem *>(qitem);
    if ( item )
    {
        bool locked = m_mutex.locked();

        KURL::List urls = m_view->nodeBuildDragList( 0 );
        KPopupMenu menu( m_view );

        enum Actions { APPEND, LOAD, QUEUE,
            COPY_TO_COLLECTION,
            BURN_ARTIST, BURN_ALBUM, BURN_DATACD, BURN_AUDIOCD,
            RENAME, SUBSCRIBE,
            MAKE_PLAYLIST, ADD_TO_PLAYLIST, ADD,
            DELETE_PLAYED, DELETE,
            FIRST_PLAYLIST};

        menu.insertItem( SmallIconSet( amaroK::icon( "playlist" ) ), i18n( "&Load" ), LOAD );
        menu.insertItem( SmallIconSet( amaroK::icon( "1downarrow" ) ), i18n( "&Append to Playlist" ), APPEND );
        menu.insertItem( SmallIconSet( amaroK::icon( "fastforward" ) ), i18n( "&Queue Tracks" ), QUEUE );
        menu.insertSeparator();

        menu.insertItem( SmallIconSet( amaroK::icon( "collection" ) ), i18n( "&Copy Files to Collection..." ), COPY_TO_COLLECTION );
        switch( item->type() )
        {
        case MediaItem::ARTIST:
            menu.insertItem( SmallIconSet( amaroK::icon( "cdrom_unmount" ) ), i18n( "Burn All Tracks by This Artist" ), BURN_ARTIST );
            menu.setItemEnabled( BURN_ARTIST, K3bExporter::isAvailable() );
            break;

        case MediaItem::ALBUM:
            menu.insertItem( SmallIconSet( amaroK::icon( "cdrom_unmount" ) ), i18n( "Burn This Album" ), BURN_ALBUM );
            menu.setItemEnabled( BURN_ALBUM, K3bExporter::isAvailable() );
            break;

        default:
            menu.insertItem( SmallIconSet( amaroK::icon( "cdrom_unmount" ) ), i18n( "Burn to CD as Data" ), BURN_DATACD );
            menu.setItemEnabled( BURN_DATACD, K3bExporter::isAvailable() );
            menu.insertItem( SmallIconSet( amaroK::icon( "cdaudio_unmount" ) ), i18n( "Burn to CD as Audio" ), BURN_AUDIOCD );
            menu.setItemEnabled( BURN_AUDIOCD, K3bExporter::isAvailable() );
            break;
        }

        menu.insertSeparator();

        if( (item->type() == MediaItem::PODCASTITEM
                 || item->type() == MediaItem::PODCASTCHANNEL) )
        {
            IpodMediaItem *it = static_cast<IpodMediaItem *>(item);
            menu.insertItem( SmallIconSet( amaroK::icon( "podcast" ) ), i18n( "Subscribe to This Podcast" ), SUBSCRIBE );
            //menu.setItemEnabled( SUBSCRIBE, item->bundle()->podcastBundle() && item->bundle()->podcastBundle()->parent().isValid() );
            menu.setItemEnabled( SUBSCRIBE, it->m_podcastInfo && !it->m_podcastInfo->rss.isEmpty() );
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
                menu.insertItem( SmallIconSet( amaroK::icon( "playlist" ) ), i18n( "Make Media Device Playlist" ), MAKE_PLAYLIST );
                menu.setItemEnabled( MAKE_PLAYLIST, !locked );

                playlistsMenu = new KPopupMenu(&menu);
                int i=0;
                for(MediaItem *it = dynamic_cast<MediaItem *>(m_playlistItem->firstChild());
                        it;
                        it = dynamic_cast<MediaItem *>(it->nextSibling()))
                {
                    playlistsMenu->insertItem( SmallIconSet( amaroK::icon( "playlist" ) ), it->text(0), FIRST_PLAYLIST+i );
                    i++;
                }
                menu.insertItem( SmallIconSet( amaroK::icon( "playlist" ) ), i18n("Add to Playlist"), playlistsMenu, ADD_TO_PLAYLIST );
                menu.setItemEnabled( ADD_TO_PLAYLIST, !locked && m_playlistItem->childCount()>0 );
                menu.insertSeparator();
            }

            if( item->type() == MediaItem::ARTIST ||
                    item->type() == MediaItem::ALBUM ||
                    item->type() == MediaItem::TRACK )
            {
                menu.insertItem( SmallIconSet( amaroK::icon( "editclear" ) ),
                        i18n( "Edit &Information...", "Edit &Information for %n Tracks...", urls.count()),
                        RENAME );
            }
            break;

        case MediaItem::ORPHANED:
        case MediaItem::ORPHANEDROOT:
            menu.insertItem( SmallIconSet( amaroK::icon( "editrename" ) ), i18n( "Add to Database" ), ADD );
            menu.setItemEnabled( ADD, !locked );
            break;

        case MediaItem::PLAYLIST:
            menu.insertItem( SmallIconSet( amaroK::icon( "editclear" ) ), i18n( "Rename" ), RENAME );
            menu.setItemEnabled( RENAME, !locked );
            break;

        default:
            break;
        }

        if( item->type() == MediaItem::PODCASTSROOT || item->type() == MediaItem::PODCASTCHANNEL )
        {
            menu.insertItem( SmallIconSet( amaroK::icon( "editdelete" ) ), i18n( "Delete Podcasts Already Played" ), DELETE_PLAYED );
            menu.setItemEnabled( DELETE_PLAYED, !locked );
        }
        menu.insertItem( SmallIconSet( amaroK::icon( "editdelete" ) ), i18n( "Delete" ), DELETE );
        menu.setItemEnabled( DELETE, !locked );

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
                    m_view->getSelectedLeaves( 0, &items );

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
            case SUBSCRIBE:
                PlaylistBrowser::instance()->addPodcast( static_cast<IpodMediaItem *>(item)->m_podcastInfo->rss );
                break;
            case RENAME:
                if( item->type() == MediaItem::PLAYLIST )
                {
                    m_view->rename(item, 0);
                }
                else
                {
                    TagDialog *dialog = NULL;
                    if( urls.count() == 1 )
                        dialog = new TagDialog( urls.first(), m_view );
                    else
                        dialog = new TagDialog( urls, m_view );
                    dialog->show();
                }
                break;
            default:
                break;
        }

        if( !m_mutex.locked() )
        {
            switch( id )
            {
            case MAKE_PLAYLIST:
                {
                    QPtrList<MediaItem> items;
                    m_view->getSelectedLeaves( 0, &items );
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
                    m_view->ensureItemVisible(pl);
                    m_view->rename(pl, 0);
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
                        if( !trackExists( *it->bundle() ) )
                        {
                            item->takeItem(it);
                            insertTrackIntoDB(it->url().path(), *it->bundle(), 0);
                            delete it;
                        }
                    }
                }
                else
                {
                    for(m_view->selectedItems().first();
                            m_view->selectedItems().current();
                            m_view->selectedItems().next())
                    {
                        MediaItem *it = dynamic_cast<MediaItem *>(m_view->selectedItems().current());
                        if(it->type() == MediaItem::ORPHANED && !trackExists(*it->bundle()))
                        {
                            it->parent()->takeItem(it);
                            insertTrackIntoDB(it->url().path(), *it->bundle(), 0);
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
                            m_view->getSelectedLeaves( 0, &items );
                            addToPlaylist( list, after, items );
                        }
                    }
                }
                break;
            }
        }
    }
}

QStringList
IpodMediaDevice::supportedFiletypes()
{
    QStringList list;
    list << "m4a";
    list << "m4b";
    list << "m4p";
    list << "mp3";
    list << "wav";
    list << "mp4";
    list << "aa";

    if( m_supportsVideo )
    {
        list << "m4v";
        list << "mp4v";
        list << "mov";
        list << "mpg";
    }

    return list;
}

void
IpodMediaDevice::addConfigElements( QWidget *parent )
{
    m_autoDeletePodcastsCheck = new QCheckBox( parent );
    m_autoDeletePodcastsCheck->setText( i18n( "&Automatically delete podcasts" ) );
    QToolTip::add( m_autoDeletePodcastsCheck, i18n( "Automatically delete podcast shows already played when connecting device" ) );
    m_autoDeletePodcastsCheck->setChecked( m_autoDeletePodcasts );

    m_syncStatsCheck = new QCheckBox( parent );
    m_syncStatsCheck->setText( i18n( "&Synchronize with amaroK statistics" ) );
    QToolTip::add( m_syncStatsCheck, i18n( "Synchronize with amaroK statistics and submit tracks played to last.fm" ) );
    m_syncStatsCheck->setChecked( m_syncStats );
}

void
IpodMediaDevice::removeConfigElements( QWidget * /*parent*/ )
{
    delete m_syncStatsCheck;
    m_syncStatsCheck = 0;

    delete m_autoDeletePodcastsCheck;
    m_autoDeletePodcastsCheck = 0;

}

void
IpodMediaDevice::applyConfig()
{
    m_autoDeletePodcasts = m_autoDeletePodcastsCheck->isChecked();
    m_syncStats = m_syncStatsCheck->isChecked();

    setConfigBool( "SyncStats", m_syncStats );
    setConfigBool( "AutoDeletePodcasts", m_autoDeletePodcasts );
}

void
IpodMediaDevice::loadConfig()
{
    MediaDevice::loadConfig();

    m_syncStats = configBool( "SyncStats", false );
    m_autoDeletePodcasts = configBool( "AutoDeletePodcasts", false );
}

#include "ipodmediadevice.moc"
