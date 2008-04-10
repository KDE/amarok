/***************************************************************************
    copyright            : (C) 2005, 2006 by Martin Aumueller
    email                : aumuell@reserv.at

    copyright            : (C) 2004 by Christian Muehlhaeuser
    email                : chris@chris.de
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it  under the terms of the GNU General Public License version 2 as    *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this library; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA  02110-1301  USA                                                   *
 ***************************************************************************/

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
#include <threadmanager.h>
#include <metadata/tplugins.h>
#include <hintlineedit.h>

#include <kactionclasses.h>
#include <kapplication.h>
#include <kmountpoint.h>
#include <kpushbutton.h>
#include <kprogress.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <kpopupmenu.h>

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
#include <unistd.h>

#ifndef HAVE_ITDB_MEDIATYPE
#define mediatype unk208
#endif


#include "metadata/audible/taglib_audiblefile.h"

struct PodcastInfo
{
    // per show
    QString url;
    QString description;
    QDateTime date;
    QString author;
    bool listened;

    // per channel
    QString rss;

    PodcastInfo() { listened = false; }
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
        IpodMediaItem( QListView *parent, MediaDevice *dev )
            : MediaItem( parent ) { init( dev ); }

        IpodMediaItem( QListViewItem *parent, MediaDevice *dev )
            : MediaItem( parent ) { init( dev ); }

        IpodMediaItem( QListView *parent, QListViewItem *after, MediaDevice *dev )
            : MediaItem( parent, after ) { init( dev ); }

        IpodMediaItem( QListViewItem *parent, QListViewItem *after, MediaDevice *dev )
            : MediaItem( parent, after ) { init( dev ); }

        virtual ~IpodMediaItem() { delete m_podcastInfo; }

        void init( MediaDevice *dev )
        {
            m_track       = 0;
            m_playlist    = 0;
            m_device      = dev;
            m_podcastInfo = 0;
        }

        void bundleFromTrack( Itdb_Track *track, const QString& path )
        {
            MetaBundle *bundle = new MetaBundle();

            bundle->setArtist    ( QString::fromUtf8( track->artist ) );
            bundle->setComposer  ( QString::fromUtf8( track->composer ) );
            bundle->setAlbum     ( QString::fromUtf8( track->album ) );
            bundle->setTitle     ( QString::fromUtf8( track->title ) );
            bundle->setComment   ( QString::fromUtf8( track->comment ) );
            bundle->setGenre     ( QString::fromUtf8( track->genre ) );
            bundle->setYear      ( track->year );
            bundle->setTrack     ( track->track_nr );
            bundle->setDiscNumber( track->cd_nr );
            bundle->setBpm       ( track->BPM );
            bundle->setLength    ( track->tracklen/1000 );
            bundle->setBitrate   ( track->bitrate );
            bundle->setSampleRate( track->samplerate );
            bundle->setPath      ( path );
            bundle->setFilesize  ( track->size );

            QString rss( track->podcastrss );
            QString url( track->podcasturl );
            QString desc( track->description );
            QString subtitle( track->subtitle );
            QDateTime date;
            date.setTime_t( itdb_time_mac_to_host( track->time_released) );

            if( !rss.isEmpty() || !url.isEmpty() )
            {
                PodcastEpisodeBundle peb( KURL::fromPathOrURL(url), KURL::fromPathOrURL(rss),
                        track->title, track->artist, desc, date.toString(Qt::ISODate), QString::null /*type*/,
                        bundle->length(), QString::null /*guid*/, track->playcount<=0 );
                bundle->setPodcastBundle( peb );
            }

            setBundle( bundle );
        }

        Itdb_Track      *m_track;
        Itdb_Playlist   *m_playlist;
        PodcastInfo     *m_podcastInfo;

        int played()         const { return m_track ? m_track->playcount        : 0; }
        int recentlyPlayed() const { return m_track ? m_track->recent_playcount : 0; }
        int rating()         const { return m_track ? m_track->rating           : 0; }

        void setRating( int rating )
        {
            if( m_track ) m_track->rating = m_track->app_rating = rating;
            if( dynamic_cast<IpodMediaDevice *>(device()) )
                static_cast<IpodMediaDevice *>(device())->m_dbChanged = true;
        }

        void setPlayCount( int playcount )
        {
            if ( m_track )
                m_track->playcount = playcount;
            if( dynamic_cast<IpodMediaDevice *>(device()) )
                static_cast<IpodMediaDevice *>(device())->m_dbChanged = true;
        }

        void setLastPlayed( uint lastplay )
        {
            if ( m_track )
                m_track->time_played = itdb_time_host_to_mac( lastplay );
            if( dynamic_cast<IpodMediaDevice *>(device()) )
                static_cast<IpodMediaDevice *>(device())->m_dbChanged = true;
        }

        bool ratingChanged() const { return m_track ? m_track->rating != m_track->app_rating : false; }

        void setListened( bool l )
        {
            MediaItem::setListened( l );
            if( type() == PODCASTITEM )
            {
                if( m_podcastInfo )
                    m_podcastInfo->listened = listened();
                if( m_track )
                    m_track->mark_unplayed = listened() ? 0x01 : 0x02;
            }
        }

        QDateTime playTime() const
        {
            QDateTime t;
            if( m_track )
                t.setTime_t( itdb_time_mac_to_host( m_track->time_played ) );
            return t;
        }

        IpodMediaItem *findTrack( Itdb_Track *track )
        {
            if( m_track == track )
                return this;

            for( IpodMediaItem *it = dynamic_cast<IpodMediaItem *>( firstChild() );
                    it;
                    it = dynamic_cast<IpodMediaItem *>( it->nextSibling()) )
            {
                IpodMediaItem *found = it->findTrack(track);
                if( found )
                    return found;
            }

            return 0;
        }
};


IpodMediaDevice::IpodMediaDevice()
    : MediaDevice()
    , m_masterPlaylist( 0 )
    , m_podcastPlaylist( 0 )
    , m_lockFile( 0 )
    , m_customAction( 0 )
{
    registerTaglibPlugins();

    m_podcastItem = 0;
    m_staleItem = 0;
    m_orphanedItem = 0;
    m_invisibleItem = 0;
    m_playlistItem = 0;

    m_dbChanged = false;
    m_itdb = 0;
    m_podcastItem = 0;
    m_staleItem = 0;
    m_orphanedItem = 0;
    m_invisibleItem = 0;
    m_playlistItem = 0;
    m_supportsArtwork = true;
    m_supportsVideo = false;
    m_rockboxFirmware = false;
    m_isShuffle = false;
    m_isMobile = false;
    m_isIPhone = false;
    m_needsFirewireGuid = false;

    m_requireMount = true;
    m_name = "iPod";

    // config stuff
    m_autoConnect = true;
    m_syncStatsCheck = 0;
    m_autoDeletePodcastsCheck = 0;

    KActionCollection *ac = new KActionCollection( this );
    KActionMenu *am = new KActionMenu( i18n( "iPod" ), Amarok::icon( "device" ), ac );
    m_customAction = am;
    m_customAction->setEnabled( false );
    am->setDelayed( false );
    KPopupMenu *menu = am->popupMenu();
    connect( menu, SIGNAL(activated(int)), SLOT(slotIpodAction(int)) );
    menu->insertItem( i18n( "Stale and Orphaned" ), CHECK_INTEGRITY );
    menu->insertItem( i18n( "Update Artwork" ), UPDATE_ARTWORK );

    KPopupMenu *ipodGen = new KPopupMenu( menu );
    menu->insertItem( i18n( "Set iPod Model" ), ipodGen );
    const Itdb_IpodInfo *table = itdb_info_get_ipod_info_table();
    if( !table )
        return;

    bool infoFound = false;
    int generation = ITDB_IPOD_GENERATION_FIRST;
    do
    {
        const Itdb_IpodInfo *info = table;
        infoFound = false;
        KPopupMenu *gen = 0;
        int index = SET_IPOD_MODEL;
        while( info->model_number )
        {
            if( info->ipod_generation == generation )
            {
                if (!infoFound)
                {
                    infoFound = true;
                    gen = new KPopupMenu( ipodGen );
                    connect( gen, SIGNAL(activated(int)), SLOT(slotIpodAction(int)) );
                    ipodGen->insertItem(
                            itdb_info_get_ipod_generation_string( info->ipod_generation),
                            gen );
                }
                if( info->capacity > 0.f )
                    gen->insertItem( i18n( "%1 GB %2 (x%3)" )
                            .arg( QString::number( info->capacity ),
                                itdb_info_get_ipod_model_name_string( info->ipod_model ),
                                info->model_number ),
                            index );
                else
                    gen->insertItem( i18n( "%1 (x%2)" )
                            .arg( itdb_info_get_ipod_model_name_string( info->ipod_model ),
                                info->model_number ), 
                            index );
            }
            ++info;
            ++index;
        }
        ++generation;
    }
    while( infoFound );
}

void
IpodMediaDevice::slotIpodAction( int id )
{
    switch( id )
    {
        case CHECK_INTEGRITY:
            checkIntegrity();
            break;
        case UPDATE_ARTWORK:
            updateArtwork();
            break;
        default:
            if( const Itdb_IpodInfo *table = itdb_info_get_ipod_info_table() )
            {
                int index = id - SET_IPOD_MODEL;
                if( m_itdb && m_itdb->device )
                {
                    gchar model[PATH_MAX];
                    g_snprintf (model, PATH_MAX, "x%s", table[index].model_number);

                    itdb_device_set_sysinfo( m_itdb->device, "ModelNumStr", model );
                    detectModel();

                    if( m_isIPhone )
                    {
                       m_autoConnect = false;
                       setConfigBool( "AutoConnect", m_autoConnect );
                    }

                    // try to make sure that the Device directory exists
                    QDir dir;
                    QString realPath;
                    if(!pathExists( itunesDir(), &realPath) )
                    {
                        dir.setPath(realPath);
                        dir.mkdir(dir.absPath());
                    }
                    if(!pathExists( itunesDir( "Device" ), &realPath) )
                    {
                        dir.setPath(realPath);
                        dir.mkdir(dir.absPath());
                    }

                    GError *err = 0; 
                    gboolean success = itdb_device_write_sysinfo(m_itdb->device, &err);
                    debug() << "success writing sysinfo to ipod? (return value " << success << ")" << endl;
                    if( !success && err )
                    {
                        g_error_free(err);
                        //FIXME: update i18n files for next message
                        Amarok::StatusBar::instance()->longMessage( 
                                i18n( "Could not write SysInfo file to iPod (check the permissions of the file \"%1\" on your iPod)" ).arg( itunesDir( "Device:SysInfo" ) ) );

                        //FIXME: update i18n files for next message
                        Amarok::StatusBar::instance()->shortMessage(
                                i18n( "Unable to set iPod model to %1 GB %2 (x%3)" )
                                .arg( QString::number( table[index].capacity ),
                                    itdb_info_get_ipod_model_name_string( table[index].ipod_model ),
                                    table[index].model_number ) );
                    } 
                    else
                    {
                        Amarok::StatusBar::instance()->shortMessage(
                                i18n( "Setting iPod model to %1 GB %2 (x%3)" )
                                .arg( QString::number( table[index].capacity ),
                                    itdb_info_get_ipod_model_name_string( table[index].ipod_model ),
                                    table[index].model_number ) );
                    }
                    MediaBrowser::instance()->updateDevices();
                }
            }
            break;
    }
}

void
IpodMediaDevice::init( MediaBrowser* parent )
{
    MediaDevice::init( parent );
}

IpodMediaDevice::~IpodMediaDevice()
{
    if( m_itdb )
        itdb_free(m_itdb);

    m_files.clear();
}

bool
IpodMediaDevice::isConnected()
{
    return ( m_itdb != 0 );
}

MediaItem *
IpodMediaDevice::insertTrackIntoDB( const QString &pathname,
        const MetaBundle &metaBundle, const MetaBundle &propertiesBundle,
        const PodcastInfo *podcastInfo )
{
    return updateTrackInDB( 0, pathname, metaBundle, propertiesBundle, podcastInfo );
}

MediaItem *
IpodMediaDevice::updateTrackInDB( IpodMediaItem *item, const QString &pathname,
                                  const MetaBundle &metaBundle, const MetaBundle &propertiesBundle,
                                  const PodcastInfo *podcastInfo )
{
    if( !m_itdb )
        return 0;

    Itdb_Track *track = 0;
    if( item )
        track = item->m_track;
    if( !track )
        track = itdb_track_new();
    if( !track )
    {
        delete item;
        return 0;
    }

    QString type = pathname.section('.', -1).lower();

    track->ipod_path = g_strdup( ipodPath(pathname).latin1() );
    debug() << "on iPod: " << track->ipod_path << ", podcast=" << podcastInfo << endl;

    if( metaBundle.isValidMedia() || !metaBundle.title().isEmpty() )
        track->title = g_strdup( metaBundle.title().utf8() );
    else
        track->title = g_strdup( metaBundle.url().filename().utf8() );
    track->album = g_strdup( metaBundle.album()->utf8() );
    track->artist = g_strdup( metaBundle.artist()->utf8() );
    track->genre = g_strdup( metaBundle.genre()->utf8() );

    track->mediatype = ITDB_MEDIATYPE_AUDIO;
    bool audiobook = false;
    if(type=="wav")
    {
        track->filetype = g_strdup( "wav" );
    }
    else if(type=="mp3" || type=="mpeg")
    {
        track->filetype = g_strdup( "mpeg" );
    }
    else if(type=="aac" || type=="m4a" || (!m_supportsVideo && type=="mp4"))
    {
        track->filetype = g_strdup( "mp4" );
    }
    else if(type=="m4b")
    {
        audiobook = true;
        track->filetype = g_strdup( "mp4" );
    }
    else if(type=="m4v" || type=="mp4v" || type=="mov" || type=="mpg" || type=="mp4")
    {
        track->filetype = g_strdup( "m4v video" );
        track->movie_flag = 0x01; // for videos
        track->mediatype = ITDB_MEDIATYPE_MOVIE;
    }
    else if(type=="aa")
    {
        audiobook = true;
        track->filetype = g_strdup( "audible" );

        TagLib::Audible::File f( QFile::encodeName( propertiesBundle.url().path() ) );
        TagLib::Audible::Tag *t = f.getAudibleTag();
        if( t )
            track->drm_userid = t->userID();
        // libgpod also tries to set those, but this won't work
        track->unk126 = 0x01;
        track->unk144 = 0x0029;

    }
    else
    {
        track->filetype = g_strdup( type.utf8() );
    }


    QString genre = metaBundle.genre();
    if( genre.startsWith("audiobook", false) )
        audiobook = true;
    if( audiobook )
    {
        track->remember_playback_position |= 0x01;
        track->skip_when_shuffling |= 0x01;
        track->mediatype = ITDB_MEDIATYPE_AUDIOBOOK;
    }

    track->composer = g_strdup( metaBundle.composer()->utf8() );
    track->comment = g_strdup( metaBundle.comment()->utf8() );
    track->track_nr = metaBundle.track();
    track->cd_nr = metaBundle.discNumber();
    track->BPM = static_cast<int>( metaBundle.bpm() );
    track->year = metaBundle.year();
    track->size = propertiesBundle.filesize();
    if( track->size == 0 )
    {
        debug() << "filesize is zero for " << track->ipod_path << ", expect strange problems with your ipod" << endl;
    }
    track->bitrate = propertiesBundle.bitrate();
    track->samplerate = propertiesBundle.sampleRate();
    track->tracklen = propertiesBundle.length()*1000;

    //Get the createdate from database
    QueryBuilder qb;
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valCreateDate );
    qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valURL, metaBundle.url().path() );
    QStringList values = qb.run();

    //Add to track info if present
    if ( values.count() ) {
        uint createdate = values.first().toUInt();
        track->time_added = itdb_time_host_to_mac( createdate );
        track->time_modified = itdb_time_host_to_mac( createdate );
    }

    if(podcastInfo)
    {
        track->skip_when_shuffling = 0x01; // skip  when shuffling
        track->remember_playback_position = 0x01; // remember playback position
        // FIXME: track->unk176 = 0x00020000; // for podcasts
        track->mark_unplayed = podcastInfo->listened ? 0x01 : 0x02;
        track->mediatype =
           track->mediatype==ITDB_MEDIATYPE_MOVIE
           ?  ITDB_MEDIATYPE_PODCAST | ITDB_MEDIATYPE_MOVIE
           : ITDB_MEDIATYPE_PODCAST;

        track->flag4 = 0x01; // also show description on iPod
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
        if( metaBundle.compilation() == MetaBundle::CompilationYes )
        {
            track->compilation = 0x01;
        }
        else
        {
            track->compilation = 0x00;
        }
    }

    m_dbChanged = true;

    if( m_supportsArtwork )
    {
        QString image;
        if( metaBundle.podcastBundle() )
        {
            PodcastChannelBundle pcb;
            if( CollectionDB::instance()->getPodcastChannelBundle( metaBundle.podcastBundle()->parent(), &pcb ) )
                image = CollectionDB::instance()->podcastImage( pcb.imageURL().url(), 0 );
        }
        if( image.isEmpty() )
            image  = CollectionDB::instance()->albumImage(metaBundle.artist(), metaBundle.album(), false, 0);
        if( !image.endsWith( "@nocover.png" ) )
        {
            debug() << "adding image " << image << " to " << metaBundle.artist() << ":" << metaBundle.album() << endl;
            itdb_track_set_thumbnails( track, g_strdup( QFile::encodeName(image) ) );
        }
    }

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
                mpl = itdb_playlist_new( "iPod", false );
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
        Amarok::StatusBar::instance()->longMessage(
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
        podcastInfo->date = peb->dateTime();
        podcastInfo->listened = !peb->isNew();
    }

    MetaBundle propertiesBundle( url );
    MediaItem *ret = insertTrackIntoDB( url.path(), bundle, propertiesBundle, podcastInfo );
    delete podcastInfo;
    return ret;
}

MediaItem *
IpodMediaDevice::tagsChanged( MediaItem *item, const MetaBundle &bundle )
{
    return updateTrackInDB( dynamic_cast<IpodMediaItem *>(item), item->url().path(), bundle, bundle, NULL );
}

void
IpodMediaDevice::synchronizeDevice()
{
#if 1
    debug() << "Syncing iPod!" << endl;
    Amarok::StatusBar::instance()->newProgressOperation( this )
        .setDescription( i18n( "Flushing iPod filesystem transfer cache" ) )
        .setTotalSteps( 1 );
    writeITunesDB();
    Amarok::StatusBar::instance()->endProgressOperation( this );
#else
    m_dbChanged = true;
    debug() << "Deferring sync of iPod!" << endl;
#endif
}

MediaItem *
IpodMediaDevice::trackExists( const MetaBundle& bundle )
{
    return getTrack( bundle.artist(),
                bundle.album(),
                bundle.title(),
                bundle.discNumber(),
                bundle.track(),
                bundle.podcastBundle() );
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
        add->bundleFromTrack( add->m_track, realPath(add->m_track->ipod_path) );
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
IpodMediaDevice::deleteItemFromDevice(MediaItem *mediaitem, int flags )
{
    IpodMediaItem *item = dynamic_cast<IpodMediaItem *>(mediaitem);
    if(!item)
        return -1;

    if( isCanceled() )
        return 0;

    if( !item->isVisible() )
        return 0;

    int count = 0;

    switch(item->type())
    {
    case MediaItem::PLAYLISTITEM:
        if( !(flags & DeleteTrack) )
        {
            // FIXME possibly wrong instance of track is removed
            itdb_playlist_remove_track(item->m_playlist, item->m_track);
            delete item;
            m_dbChanged = true;
            break;
        }
        // else fall through
    case MediaItem::STALE:
    case MediaItem::TRACK:
    case MediaItem::INVISIBLE:
    case MediaItem::PODCASTITEM:
        if(!(flags & OnlyPlayed) || item->played() > 0)
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

            // delete all other occurrences
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
                if( isCanceled() )
                    break;

                next = dynamic_cast<IpodMediaItem *>(it->nextSibling());
                int ret = deleteItemFromDevice(it, flags);
                if( ret >= 0 && count >= 0 )
                    count += ret;
                else
                    count = -1;
            }
        }
        if(item->type() == MediaItem::PLAYLIST && !isCanceled())
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
            if(!(flags & OnlyPlayed) || item->played() > 0 || item->childCount() == 0)
            {
                if(item->childCount() > 0)
                    debug() << "recursive deletion should have removed all children from " << item << "(" << item->text(0) << ")" << endl;
                else
                    delete item;
            }
        }
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
IpodMediaDevice::createLockFile( bool silent )
{
    QString lockFilePath;
    pathExists( itunesDir( "iTunes:iTunesLock" ), &lockFilePath );

    m_lockFile = new QFile( lockFilePath );
    QString msg;
    bool ok = true;
    if( m_lockFile->exists() )
    {
        ok = false;
        msg = i18n( "Media Device: iPod mounted at %1 already locked. " ).arg( mountPoint() );
        msg += i18n( "If you are sure that this is an error, then remove the file %1 and try again." )
           .arg( lockFilePath );

        if( !silent )
        {
            if( KMessageBox::warningContinueCancel( m_parent, msg, i18n( "Remove iTunes Lock File?" ),
                        KGuiItem(i18n("&Remove"), "editdelete"), QString::null, KMessageBox::Dangerous )
                    == KMessageBox::Continue )
            {
                msg = i18n( "Media Device: removing lockfile %1 failed: %2. " )
                    .arg( lockFilePath, m_lockFile->errorString() );
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
            .arg(mountPoint(), m_lockFile->errorString());
    }

    if( ok )
        return true;

    delete m_lockFile;
    m_lockFile = 0;

    if( !msg.isEmpty() )
        Amarok::StatusBar::instance()->longMessage( msg, KDE::StatusBar::Sorry );
    return false;
}

bool
IpodMediaDevice::initializeIpod()
{
    QDir dir( mountPoint() );
    if( !dir.exists() )
    {
        Amarok::StatusBar::instance()->longMessage(
                i18n("Media device: Mount point %1 does not exist").arg(mountPoint()),
                KDE::StatusBar::Error );
        return false;
    }

    debug() << "initializing iPod mounted at " << mountPoint() << endl;

    // initialize iPod
    m_itdb = itdb_new();
    if( m_itdb == 0 )
        return false;

    // in order to get directories right
    detectModel();

    itdb_set_mountpoint(m_itdb, QFile::encodeName(mountPoint()));

    Itdb_Playlist *mpl = itdb_playlist_new("iPod", false);
    itdb_playlist_set_mpl(mpl);
    Itdb_Playlist *podcasts = itdb_playlist_new("Podcasts", false);
    itdb_playlist_set_podcasts(podcasts);
    itdb_playlist_add(m_itdb, podcasts, -1);
    itdb_playlist_add(m_itdb, mpl, 0);

    QString realPath;
    if(!pathExists( itunesDir(), &realPath) )
    {
        dir.setPath(realPath);
        dir.mkdir(dir.absPath());
    }
    if(!dir.exists())
        return false;

    if(!pathExists( itunesDir( "Music" ), &realPath) )
    {
        dir.setPath(realPath);
        dir.mkdir(dir.absPath());
    }
    if(!dir.exists())
        return false;

    if(!pathExists( itunesDir( "iTunes" ), &realPath) )
    {
        dir.setPath(realPath);
        dir.mkdir(dir.absPath());
    }
    if(!dir.exists())
        return false;

    if( !writeITunesDB( false ) )
        return false;

    Amarok::StatusBar::instance()->longMessage(
            i18n("Media Device: Initialized iPod mounted at %1").arg(mountPoint()),
            KDE::StatusBar::Information );

    return true;
}

bool
IpodMediaDevice::openDevice( bool silent )
{
    m_isShuffle = false;
    m_isMobile = false;
    m_isIPhone = false;
    m_supportsArtwork = false;
    m_supportsVideo = false;
    m_needsFirewireGuid = false;
    m_rockboxFirmware = false;
    m_dbChanged = false;
    m_files.clear();

    if( m_itdb )
    {
        Amarok::StatusBar::instance()->longMessage(
                i18n("Media Device: iPod at %1 already opened").arg(mountPoint()),
                KDE::StatusBar::Sorry );
        return false;
    }

    // try to find a mounted ipod
    bool ipodFound = false;
    bool canInitialize = false;
    KMountPoint::List currentmountpoints = KMountPoint::currentMountPoints();
    for( KMountPoint::List::Iterator mountiter = currentmountpoints.begin();
        mountiter != currentmountpoints.end();
        ++mountiter )
    {
        canInitialize = false;
        QString devicenode = (*mountiter)->mountedFrom();
        QString mountpoint = (*mountiter)->mountPoint();

        if( mountpoint.startsWith( "/proc" ) ||
            mountpoint.startsWith( "/sys" )  ||
            mountpoint.startsWith( "/dev" )  ||
            mountpoint.startsWith( "/boot" ) )
            continue;

        if( !mountPoint().isEmpty() )
        {
            if( mountpoint != mountPoint() )
                continue;
            canInitialize = true;
        }

        else if( !deviceNode().isEmpty() )
        {
            if( devicenode != deviceNode() )
                continue;
            canInitialize = true;
        }

        GError *err = 0;
        m_itdb = itdb_parse(QFile::encodeName(mountpoint), &err);
        if( err )
        {
            g_error_free(err);
            if( m_itdb )
            {
                itdb_free( m_itdb );
                m_itdb = 0;
            }

            if( !canInitialize )
                continue;
        }

        if( mountPoint().isEmpty() )
            m_medium.setMountPoint( mountpoint );
        ipodFound = true;
        break;
    }

    if( !ipodFound && !canInitialize )
    {
        if( !silent )
        {
            Amarok::StatusBar::instance()->longMessage(
                    i18n("Media Device: No mounted iPod found" ),
                    KDE::StatusBar::Sorry );
        }
        return false;
    }

    if( !m_itdb && canInitialize )
    {
        QString msg = i18n( "Media Device: could not find iTunesDB on device mounted at %1. "
                "Should I try to initialize your iPod?" ).arg( mountPoint() );

        if( !silent
                && KMessageBox::warningContinueCancel( m_parent, msg, i18n( "Initialize iPod?" ),
                    KGuiItem(i18n("&Initialize"), "new") ) == KMessageBox::Continue )
        {
            if( !initializeIpod() )
            {
                if( m_itdb )
                {
                    itdb_free( m_itdb );
                    m_itdb = 0;
                }

                Amarok::StatusBar::instance()->longMessage(
                        i18n("Media Device: Failed to initialize iPod mounted at %1").arg(mountPoint()),
                        KDE::StatusBar::Sorry );

                return false;
            }
        }
        else
           return false;
    }

    detectModel();

    if( !createLockFile( silent ) )
    {
        if( m_itdb )
        {
            itdb_free( m_itdb );
            m_itdb = 0;
        }
        return false;
    }

    for( int i=0; i < itdb_musicdirs_number(m_itdb); i++)
    {
        QString real;
        QString ipod;
        ipod.sprintf( itunesDir( "Music:f%02d" ).latin1(), i );
        if(!pathExists( ipod, &real ) )
        {
            QDir dir( real );
            dir.mkdir( real );
            dir.setPath( real );
            if( !dir.exists() )
            {
                debug() << "failed to create hash dir " << real << endl;
                Amarok::StatusBar::instance()->longMessage(
                        i18n("Media device: Failed to create directory %1").arg(real),
                        KDE::StatusBar::Error );
                return false;
            }
        }
    }

    if( !silent )
        kapp->processEvents( 100 );

    initView();
    GList *cur = m_itdb->playlists;
    for( ; cur; cur = cur->next )
    {
        Itdb_Playlist *playlist = (Itdb_Playlist *)cur->data;
        addPlaylistToView( playlist );
    }

    if( !silent )
        kapp->processEvents( 100 );

    for( cur = m_itdb->tracks; cur; cur = cur->next )
    {
        Itdb_Track *track = (Itdb_Track *)cur->data;
        addTrackToView( track, 0 /*parent*/, false /*checkintegrity*/, true /*batchmode*/ );
    }

    if( !silent )
        kapp->processEvents( 100 );

    updateRootItems();
    m_customAction->setEnabled( true );

    m_dbChanged = true; // write at least once for synchronising new stats

    return true;
}

void
IpodMediaDevice::detectModel()
{
    // set some sane default values
    m_isShuffle = false;
    m_supportsArtwork = true;
    m_supportsVideo = false;
    m_isIPhone = false;
    m_needsFirewireGuid = false;
    m_rockboxFirmware = false;
    
    // needs recent libgpod-0.3.3 from cvs
    bool guess = false;
    if( m_itdb && m_itdb->device )
    {
        const Itdb_IpodInfo *ipodInfo = itdb_device_get_ipod_info( m_itdb->device );
        const gchar *modelString = 0;

        m_supportsArtwork = itdb_device_supports_artwork( m_itdb->device );

        if( ipodInfo )
        {
            modelString = itdb_info_get_ipod_model_name_string ( ipodInfo->ipod_model );

            switch( ipodInfo->ipod_model )
            {
            case ITDB_IPOD_MODEL_SHUFFLE:
#ifdef HAVE_LIBGPOD_060
            case ITDB_IPOD_MODEL_SHUFFLE_SILVER:
            case ITDB_IPOD_MODEL_SHUFFLE_PINK:
            case ITDB_IPOD_MODEL_SHUFFLE_BLUE:
            case ITDB_IPOD_MODEL_SHUFFLE_GREEN:
            case ITDB_IPOD_MODEL_SHUFFLE_ORANGE:
            case ITDB_IPOD_MODEL_SHUFFLE_PURPLE:
#endif
                m_isShuffle = true;
                break;
#ifdef HAVE_LIBGPOD_060
            case ITDB_IPOD_MODEL_IPHONE_1:
            case ITDB_IPOD_MODEL_TOUCH_BLACK:
                m_isIPhone = true;
                debug() << "detected iPhone/iPod Touch" << endl;
                break;
            case ITDB_IPOD_MODEL_CLASSIC_SILVER:
            case ITDB_IPOD_MODEL_CLASSIC_BLACK:
#endif
            case ITDB_IPOD_MODEL_VIDEO_WHITE:
            case ITDB_IPOD_MODEL_VIDEO_BLACK:
            case ITDB_IPOD_MODEL_VIDEO_U2:
                m_supportsVideo = true;
                debug() << "detected video-capable iPod" << endl;
                break;
            case ITDB_IPOD_MODEL_MOBILE_1:
                m_isMobile = true;
                m_supportsArtwork = true;
                debug() << "detected iTunes phone" << endl;
                break;
            case ITDB_IPOD_MODEL_INVALID:
            case ITDB_IPOD_MODEL_UNKNOWN:
                modelString = 0;
                guess = true;
                break;
            default:
                break;
            }

#ifdef HAVE_LIBGPOD_060
            switch( ipodInfo->ipod_generation )
            {
               case ITDB_IPOD_GENERATION_CLASSIC_1:
               case ITDB_IPOD_GENERATION_NANO_3:
               case ITDB_IPOD_GENERATION_TOUCH_1:
                  m_needsFirewireGuid = true;
                  m_supportsVideo = true;
                  break;
               case ITDB_IPOD_GENERATION_VIDEO_1:
               case ITDB_IPOD_GENERATION_VIDEO_2:
                  m_supportsVideo = true;
                  break;
               case ITDB_IPOD_GENERATION_SHUFFLE_1:
               case ITDB_IPOD_GENERATION_SHUFFLE_2:
               case ITDB_IPOD_GENERATION_SHUFFLE_3:
                  m_isShuffle = true;
                  break;
               default:
                  break;
            }
#endif
        }
        if( modelString )
            m_name = QString( "iPod %1" ).arg( QString::fromUtf8( modelString ) );

        if( m_needsFirewireGuid )
        {
            gchar *fwid = itdb_device_get_sysinfo( m_itdb->device, "FirewireGuid" );
            if( !fwid )
            {
                Amarok::StatusBar::instance()->longMessage(
                        i18n("Your iPod's Firewire GUID is required for correctly updating its music database, but it is not known. See %1 for more information.").arg( "http://amarok.kde.org/wiki/Media_Device:IPod" ) );
            }
            else
               g_free( fwid );
        }
    }
    else
    {
        debug() << "iPod type detection failed, no video support" << endl;
        Amarok::StatusBar::instance()->longMessage(
                i18n("iPod type detection failed: no support for iPod Shuffle, for artwork or video") );
        guess = true;
    }

    if( guess )
    {
        if( pathExists( ":iTunes:iTunes_Control" ) )
        {
            debug() << "iTunes/iTunes_Control found - assuming itunes phone" << endl;
            m_isMobile = true;
        }
        else if( pathExists( ":iTunes_Control" ) )
        {
            debug() << "iTunes_Control found - assuming iPhone/iPod Touch" << endl;
            m_isIPhone = true;
        }
    }

    if( m_isIPhone )
    {
        m_supportsVideo = true;
        m_supportsArtwork = true;
    }

    if( pathExists( ":.rockbox" ) )
    {
        debug() << "RockBox firmware detected" << endl;
        m_rockboxFirmware = true;
    }
}

void
IpodMediaDevice::initView()
{
    m_view->clear();

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

    updateRootItems();
}

void
IpodMediaDevice::updateArtwork()
{
    if( !m_supportsArtwork )
        return;

    QPtrList<MediaItem> items;
    m_view->getSelectedLeaves( 0, &items, false );

    int updateCount = 0;
    for( QPtrList<MediaItem>::iterator it = items.begin();
            it != items.end();
            it++ )
    {
        IpodMediaItem *i = dynamic_cast<IpodMediaItem *>( *it );
        if( !i || i->type() == MediaItem::PLAYLISTITEM )
            continue;

        const MetaBundle *bundle = i->bundle();
        QString image;
        if( i->m_podcastInfo && !i->m_podcastInfo->rss.isEmpty() )
        {
            PodcastChannelBundle pcb;
            if( CollectionDB::instance()->getPodcastChannelBundle( i->m_podcastInfo->rss, &pcb ) )
                image = CollectionDB::instance()->podcastImage( pcb.imageURL().url(), 0 );
        }
        if( image.isEmpty() )
            image  = CollectionDB::instance()->albumImage(bundle->artist(), bundle->album(), false, 0);
        if( !image.endsWith( "@nocover.png" ) )
        {
            debug() << "adding image " << image << " to " << bundle->artist() << ":"
                << bundle->album() << endl;
            itdb_track_set_thumbnails( i->m_track, g_strdup( QFile::encodeName(image) ) );
            ++updateCount;
        }
    }

    Amarok::StatusBar::instance()->shortMessage(
            i18n( "Updated artwork for one track", "Updated artwork for %n tracks", updateCount ) );

    if(!m_dbChanged)
       m_dbChanged = updateCount > 0;
}


bool
IpodMediaDevice::checkIntegrity()
{
    if( !m_itdb )
        return false;

    initView();

    GList *cur = m_itdb->tracks;
    while(cur)
    {
        Itdb_Track *track = (Itdb_Track *)cur->data;

        addTrackToView( track, 0, true );

        cur = cur->next;
    }

    cur = m_itdb->playlists;
    for( ; cur; cur = cur->next )
    {
        Itdb_Playlist *playlist = (Itdb_Playlist *)cur->data;
        addPlaylistToView( playlist );
    }

    QString musicpath;
    if (!pathExists( itunesDir( "Music" ), &musicpath ))
        return false;

    QDir dir( musicpath, QString::null, QDir::Unsorted, QDir::Dirs );
    for(unsigned i=0; i<dir.count(); i++)
    {
        if(dir[i] == "." || dir[i] == "..")
            continue;

        QString hashpath = musicpath + '/' + dir[i];
        QDir hashdir( hashpath, QString::null, QDir::Unsorted, QDir::Files );
        for(unsigned j=0; j<hashdir.count(); j++)
        {
            QString filename = hashpath + '/' + hashdir[j];
            QString ipodPath = itunesDir( "Music:" ) + dir[i] + ':' + hashdir[j];
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

    updateRootItems();

    Amarok::StatusBar::instance()->shortMessage(
            i18n( "Scanning for stale and orphaned tracks finished" ) );

    return true;
}

bool
IpodMediaDevice::closeDevice()  //SLOT
{
    m_customAction->setEnabled( false );

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
IpodMediaDevice::addTrackToView(Itdb_Track *track, IpodMediaItem *item, bool checkIntegrity, bool batchmode )
{
    bool visible = false;
    bool stale = false;

    if( checkIntegrity )
    {
        if( !pathExists( track->ipod_path ) )
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
    }

    if(!stale && m_masterPlaylist && itdb_playlist_contains_track(m_masterPlaylist, track)
          && (!m_podcastPlaylist || !itdb_playlist_contains_track(m_podcastPlaylist, track)))
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
            item = new IpodMediaItem( channel, this );
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

    if( !stale && !visible )
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

    if ( !batchmode )
        updateRootItems();

    return item;
}

void
IpodMediaDevice::addPlaylistToView( Itdb_Playlist *pl )
{
    if( itdb_playlist_is_mpl( pl ) )
    {
        m_masterPlaylist = pl;
        return;
    }

    if( itdb_playlist_is_podcasts( pl ) )
    {
        m_podcastPlaylist = pl;
        return;
    }

    if( pl->is_spl )
    {
        debug() << "playlist " << pl->name << " is a smart playlist" << endl;
    }

    QString name( QString::fromUtf8(pl->name) );
    IpodMediaItem *playlist = dynamic_cast<IpodMediaItem *>(m_playlistItem->findItem(name));
    if( !playlist )
    {
        playlist = new IpodMediaItem( m_playlistItem, this );
        playlist->setText( 0, name );
        playlist->setType( MediaItem::PLAYLIST );
        playlist->m_playlist = pl;
    }

    int i=0;
    GList *cur = pl->members;
    while( cur )
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
IpodMediaDevice::itunesDir(const QString &p) const
{
    QString base( ":iPod_Control" );
    if( m_isMobile )
        base = ":iTunes:iTunes_Control";
    else if( m_isIPhone )
        base = ":iTunes_Control";

    if( !p.startsWith( ":" ) )
        base += ':';
    return base + p;
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

    return QString();
}

class IpodWriteDBJob : public ThreadManager::DependentJob
{
    public:
        IpodWriteDBJob( QObject *parent, Itdb_iTunesDB *itdb, bool isShuffle, bool *resultPtr )
        : ThreadManager::DependentJob( parent, "IpodWriteDBJob" )
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
            {   /* an error occurred */
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
                {   /* an error occurred */
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
    if(!m_itdb)
        return false;

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
            {   /* an error occurred */
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
                {   /* an error occurred */
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
            ThreadManager::instance()->queueJob( new IpodWriteDBJob( this, m_itdb, m_isShuffle, &ok ) );
            while( ThreadManager::instance()->isJobPending( "IpodWriteDBJob" ) )
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
            Amarok::StatusBar::instance()->longMessage(
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
IpodMediaDevice::getTrack(const QString &artist, const QString &album, const QString &title,
        int discNumber, int trackNumber, const PodcastEpisodeBundle *peb)
{
    IpodMediaItem *item = getAlbum(artist, album);
    if(item)
    {
        for( IpodMediaItem *track = dynamic_cast<IpodMediaItem *>(item->findItem(title));
                track;
                track = dynamic_cast<IpodMediaItem *>(item->findItem(title, track)) )
        {
            if( ( discNumber==-1 || track->bundle()->discNumber()==discNumber )
                    && ( trackNumber==-1 || track->bundle()->track()==trackNumber ) )
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
            if( ( discNumber==-1 || track->bundle()->discNumber()==discNumber )
                    && ( trackNumber==-1 || track->bundle()->track()==trackNumber ) )
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
                if( ( discNumber==-1 || track->bundle()->discNumber()==discNumber )
                        && ( trackNumber==-1 || track->bundle()->track()==trackNumber )
                        && ( !track->bundle()->podcastBundle() || !peb
                             || track->bundle()->podcastBundle()->url() == peb->url() ) )
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

    item = getAlbum( i18n( "Various Artists" ), album );
    if( item )
    {
        QString t = artist + i18n(" - ") + title;
        for( IpodMediaItem *track = dynamic_cast<IpodMediaItem *>(item->findItem(t));
                track;
                track = dynamic_cast<IpodMediaItem *>(item->findItem(t, track)) )
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
    QString type = local.section('.', -1).lower();

    QString trackpath;
    QString realpath;
    do
    {
        int num = std::rand() % 1000000;
        int music_dirs = itdb_musicdirs_number(m_itdb) > 1 ? itdb_musicdirs_number(m_itdb) : 20;
        int dir = num % music_dirs;
        QString dirname;
        dirname.sprintf( "%s:Music:f%02d", itunesDir().latin1(), dir );
        if( !pathExists( dirname ) )
        {
            QString realdir = realPath(dirname.latin1());
            QDir qdir( realdir );
            qdir.mkdir( realdir );
        }
        QString filename;
        filename.sprintf( ":kpod%07d.%s", num, type.latin1() );
        trackpath = dirname + filename;
    }
    while( pathExists( trackpath, &realpath ) );

    return realpath;
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
    QString path;
    if ( !pathExists( itunesDir(), &path ) )
        return false;

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
    bool locked = m_mutex.locked();

    KURL::List urls = m_view->nodeBuildDragList( 0 );
    KPopupMenu menu( m_view );

    enum Actions { CREATE_PLAYLIST, APPEND, LOAD, QUEUE,
        COPY_TO_COLLECTION,
        BURN_ARTIST, BURN_ALBUM, BURN_DATACD, BURN_AUDIOCD,
        RENAME, SUBSCRIBE,
        MAKE_PLAYLIST, ADD_TO_PLAYLIST, ADD,
        DELETE_PLAYED, DELETE_FROM_IPOD, REMOVE_FROM_PLAYLIST,
        FIRST_PLAYLIST};

    KPopupMenu *playlistsMenu = 0;
    if ( item )
    {
        if( item->type() == MediaItem::PLAYLISTSROOT )
        {
            menu.insertItem( SmallIconSet(Amarok::icon( "add_playlist" )),
                    i18n("Create Playlist..."), CREATE_PLAYLIST );
        }
        else
        {
            menu.insertItem( SmallIconSet( Amarok::icon( "playlist" ) ), i18n( "&Load" ), LOAD );
            menu.insertItem( SmallIconSet( Amarok::icon( "add_playlist" ) ), i18n( "&Append to Playlist" ), APPEND );
            menu.insertItem( SmallIconSet( Amarok::icon( "fastforward" ) ), i18n( "&Queue Tracks" ), QUEUE );
        }
        menu.insertSeparator();

        menu.insertItem( SmallIconSet( Amarok::icon( "collection" ) ), i18n( "&Copy Files to Collection..." ), COPY_TO_COLLECTION );
        switch( item->type() )
        {
        case MediaItem::ARTIST:
            menu.insertItem( SmallIconSet( Amarok::icon( "cdrom_unmount" ) ), i18n( "Burn All Tracks by This Artist" ), BURN_ARTIST );
            menu.setItemEnabled( BURN_ARTIST, K3bExporter::isAvailable() );
            break;

        case MediaItem::ALBUM:
            menu.insertItem( SmallIconSet( Amarok::icon( "cdrom_unmount" ) ), i18n( "Burn This Album" ), BURN_ALBUM );
            menu.setItemEnabled( BURN_ALBUM, K3bExporter::isAvailable() );
            break;

        default:
            menu.insertItem( SmallIconSet( Amarok::icon( "cdrom_unmount" ) ), i18n( "Burn to CD as Data" ), BURN_DATACD );
            menu.setItemEnabled( BURN_DATACD, K3bExporter::isAvailable() );
            menu.insertItem( SmallIconSet( Amarok::icon( "cdaudio_unmount" ) ), i18n( "Burn to CD as Audio" ), BURN_AUDIOCD );
            menu.setItemEnabled( BURN_AUDIOCD, K3bExporter::isAvailable() );
            break;
        }

        menu.insertSeparator();

        if( (item->type() == MediaItem::PODCASTITEM
                 || item->type() == MediaItem::PODCASTCHANNEL) )
        {
            IpodMediaItem *it = static_cast<IpodMediaItem *>(item);
            menu.insertItem( SmallIconSet( Amarok::icon( "podcast" ) ), i18n( "Subscribe to This Podcast" ), SUBSCRIBE );
            //menu.setItemEnabled( SUBSCRIBE, item->bundle()->podcastBundle() && item->bundle()->podcastBundle()->parent().isValid() );
            menu.setItemEnabled( SUBSCRIBE, it->m_podcastInfo && !it->m_podcastInfo->rss.isEmpty() );
            menu.insertSeparator();
        }

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
                menu.insertItem( SmallIconSet( Amarok::icon( "playlist" ) ), i18n( "Make Media Device Playlist" ), MAKE_PLAYLIST );
                menu.setItemEnabled( MAKE_PLAYLIST, !locked );

                playlistsMenu = new KPopupMenu(&menu);
                int i=0;
                for(MediaItem *it = dynamic_cast<MediaItem *>(m_playlistItem->firstChild());
                        it;
                        it = dynamic_cast<MediaItem *>(it->nextSibling()))
                {
                    playlistsMenu->insertItem( SmallIconSet( Amarok::icon( "playlist" ) ), it->text(0), FIRST_PLAYLIST+i );
                    i++;
                }
                menu.insertItem( SmallIconSet( Amarok::icon( "playlist" ) ), i18n("Add to Playlist"), playlistsMenu, ADD_TO_PLAYLIST );
                menu.setItemEnabled( ADD_TO_PLAYLIST, !locked && m_playlistItem->childCount()>0 );
                menu.insertSeparator();
            }

            if( item->type() == MediaItem::ARTIST ||
                    item->type() == MediaItem::ALBUM ||
                    item->type() == MediaItem::TRACK ||
                    item->type() == MediaItem::ORPHANED )
            {
                menu.insertItem( SmallIconSet( Amarok::icon( "edit" ) ),
                        i18n( "Edit &Information...", "Edit &Information for %n Tracks...", urls.count()),
                        RENAME );
            }
            break;

        case MediaItem::ORPHANED:
        case MediaItem::ORPHANEDROOT:
            menu.insertItem( SmallIconSet( Amarok::icon( "edit" ) ), i18n( "Add to Database" ), ADD );
            menu.setItemEnabled( ADD, !locked );
            break;

        case MediaItem::PLAYLIST:
            menu.insertItem( SmallIconSet( Amarok::icon( "edit" ) ), i18n( "Rename" ), RENAME );
            menu.setItemEnabled( RENAME, !locked );
            break;

        default:
            break;
        }

        if( item->type() == MediaItem::PLAYLIST || item->type() == MediaItem::PLAYLISTITEM )
        {
            menu.insertItem( SmallIconSet( Amarok::icon( "remove_from_playlist" ) ),
                    item->type()==MediaItem::PLAYLIST ? i18n( "Remove Playlist" ) : i18n( "Remove from Playlist" ),
                    REMOVE_FROM_PLAYLIST );
            menu.setItemEnabled( REMOVE_FROM_PLAYLIST, !locked );
        }
        if( item->type() == MediaItem::PODCASTSROOT || item->type() == MediaItem::PODCASTCHANNEL )
        {
            menu.insertItem( SmallIconSet( Amarok::icon( "remove" ) ), i18n( "Delete Podcasts Already Played" ), DELETE_PLAYED );
            menu.setItemEnabled( DELETE_PLAYED, !locked );
        }
        menu.insertItem( SmallIconSet( Amarok::icon( "remove" ) ),
                i18n( "Delete Track from iPod", "Delete %n Tracks from iPod", urls.count() ),
                DELETE_FROM_IPOD );
        menu.setItemEnabled( DELETE_FROM_IPOD, !locked && urls.count() > 0 );
    }

    int id =  menu.exec( point );
    switch( id )
    {
        case CREATE_PLAYLIST:
            break;
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

                CollectionView::instance()->organizeFiles( urls, i18n("Copy Files To Collection"), true );
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
            case CREATE_PLAYLIST:
            case MAKE_PLAYLIST:
                {
                    QPtrList<MediaItem> items;
                    if( id == MAKE_PLAYLIST )
                        m_view->getSelectedLeaves( 0, &items );
                    QString base(i18n("New Playlist"));
                    QString name = base;
                    int i=1;
                    while(m_playlistItem->findItem(name))
                    {
                        QString num;
                        num.setNum(i);
                        name = base + ' ' + num;
                        i++;
                    }
                    MediaItem *pl = newPlaylist(name, m_playlistItem, items);
                    m_view->ensureItemVisible(pl);
                    m_view->rename(pl, 0);
                }
                break;
            case ADD:
                {
                    int dupes = 0;

                    if(item->type() == MediaItem::ORPHANEDROOT)
                    {
                        MediaItem *next = 0;
                        for(MediaItem *it = dynamic_cast<MediaItem *>(item->firstChild());
                                it;
                                it = next)
                        {
                            next = dynamic_cast<MediaItem *>(it->nextSibling());
                            if( trackExists( *it->bundle() ) )
                            {
                                dupes++;
                                continue;
                            }

                            item->takeItem(it);
                            insertTrackIntoDB(it->url().path(), *it->bundle(), *it->bundle(), 0);
                            delete it;
                        }
                    }
                    else
                    {
                        QPtrList<MediaItem> items;
                        m_view->getSelectedLeaves( 0, &items );
                        for( QPtrList<MediaItem>::iterator it = items.begin();
                                it != items.end();
                                it++ )
                        {
                            IpodMediaItem *i = dynamic_cast<IpodMediaItem *>( *it );
                            if( !i || i->type() != MediaItem::ORPHANED )
                                continue;

                            if( trackExists( *i->bundle() ) )
                            {
                                dupes++;
                                continue;
                            }

                            i->parent()->takeItem(i);
                            insertTrackIntoDB(i->url().path(), *i->bundle(), *i->bundle(), 0);
                            delete i;
                        }
                    }

                    if( dupes > 0 )
                        Amarok::StatusBar::instance()->shortMessage( i18n(
                                    "One duplicate track not added to database",
                                    "%n duplicate tracks not added to database", dupes ) );
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
            case REMOVE_FROM_PLAYLIST:
                deleteFromDevice(m_playlistItem, None);
                break;
            case DELETE_FROM_IPOD:
                deleteFromDevice();
                break;
           default:
                if( playlistsMenu && id >= FIRST_PLAYLIST )
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

        if( m_dbChanged && lockDevice( true ) )
        {
            synchronizeDevice();
            unlockDevice();
        }
    }
}

QStringList
IpodMediaDevice::supportedFiletypes()
{
    QStringList list;
    list << "mp3";
    list << "m4a";
    list << "m4b";
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

    if( m_rockboxFirmware )
    {
        list << "ogg";
        list << "mpc";
        list << "ac3";
        list << "adx";
        list << "aiff";
        list << "flac";
        list << "mid";
        list << "midi";
        list << "shn";
        list << "wv";
        list << "ape";
        list << "tta";
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
    m_syncStatsCheck->setText( i18n( "&Synchronize with Amarok statistics" ) );
    QToolTip::add( m_syncStatsCheck, i18n( "Synchronize with Amarok statistics and submit tracks played to last.fm" ) );
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
    m_autoConnect = configBool( "AutoConnect", true );
}

bool
IpodMediaDevice::pathExists( const QString &ipodPath, QString *realPath )
{
    QDir curDir( mountPoint() );
    curDir.setFilter(curDir.filter() | QDir::Hidden);
    QString curPath = mountPoint();
    QStringList components = QStringList::split( ":", ipodPath );

    bool found = false;
    QStringList::iterator it = components.begin();
    for( ; it != components.end(); ++it )
    {
        found = false;
        for(uint i=0; i<curDir.count(); i++)
        {
            if( curDir[i].lower() == (*it).lower())
            {
                curPath += '/' + curDir[i];
                curDir.cd( curPath );
                found = true;
                break;
            }
        }
        if(!found)
            break;
    }

    for( ; it != components.end(); ++it )
        curPath += '/' + *it;

    //debug() << ipodPath << ( found ? "" : " not" ) << " found, actually " << curPath << endl;

    if( realPath )
        *realPath = curPath;

    return found;
}

void
IpodMediaDevice::fileDeleted( KIO::Job *job )  //SLOT
{
    if(job->error())
    {
        debug() << "file deletion failed: " << job->errorText() << endl;
    }
    m_waitForDeletion = false;
    m_parent->updateStats();
}

void
IpodMediaDevice::deleteFile( const KURL &url )
{
    debug() << "deleting " << url.prettyURL() << endl;
    m_waitForDeletion = true;
    KIO::Job *job = KIO::file_delete( url, false );
    connect( job, SIGNAL( result( KIO::Job * ) ),
            this,  SLOT( fileDeleted( KIO::Job * ) ) );
    do
    {
        kapp->processEvents( 100 );
        if( isCanceled() )
            break;
        usleep( 10000 );
    } while( m_waitForDeletion );

    if(!isTransferring())
        setProgress( progress() + 1 );
}

#include "ipodmediadevice.moc"
