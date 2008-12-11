/**************************************************************************
 * Ported to Collection Framework: *
 * copyright            : (C) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com>
 *
 * Original Work: *
 * copyright            : (C) 2005, 2006 by Martin Aumueller <aumuell@reserv.at>
 * copyright            : (C) 2004 by Christian Muehlhaeuser <chris@chris.de>
 *******************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define DEBUG_PREFIX "IpodHandler"

#include "IpodHandler.h"

#include "IpodCollection.h"
#include "IpodMeta.h"
#include "../../../statusbar/StatusBar.h"
#include "Debug.h"

#ifdef FOUND_GDK
extern "C" {
#include <gdk-pixbuf/gdk-pixbuf.h>
}
#endif

#include "File.h" // for KIO file handling
#include "taglib_audiblefile.h"

#include <KIO/Job>
#include <KIO/DeleteJob>
#include "kjob.h"
#include <KTemporaryFile>
#include <KUrl>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPixmap>
#include <QString>
#include <QStringList>
#include <QTime>

using namespace Ipod;
using namespace Meta;

IpodHandler::IpodHandler( IpodCollection *mc, const QString& mountPoint, QObject *parent )
    : QObject( parent )
    , m_memColl( mc )
    , m_device( 0 )
    , m_masterPlaylist( 0 )
    , m_tempdir( new KTempDir() )
    , m_trackCreated( false )
    , m_isShuffle( false )
    , m_isMobile( false )
    , m_isIPhone( false )
    , m_supportsArtwork( false )
    , m_supportsVideo( false )
    , m_rockboxFirmware( false )
    , m_needsFirewireGuid( false )
    , m_autoConnect( false )
    , m_mountPoint( mountPoint )
    , m_name()
    , m_dbChanged( false )
    , m_copyFailed( false )
    , m_isCanceled( false )
    , m_wait( false )
{
    DEBUG_BLOCK

    GError *err = 0;
    m_success = false;

    // Assuming database exists for now, later will port init db code
    debug() << "Calling the db parser";
    m_itdb = itdb_parse( QFile::encodeName( m_mountPoint ),  &err );

    if( err )
    {
        g_error_free( err );
        if ( m_itdb )
        {
            debug() << "There was an error, attempting to free db";
            itdb_free( m_itdb );
            m_itdb = 0;
        }
    }
    else
    {
        // read device info
        debug() << "Grabbing device struct";
        m_device = m_itdb->device;
        debug() << "Reading device info";
        itdb_device_read_sysinfo( m_device );

        debug() << "Getting model information";
        detectModel(); // get relevant info about device

        // set tempdir up
        m_tempdir->setAutoRemove( true );
        qsrand( QTime::currentTime().msec() ); // random number used for folder number generation

        m_success = true;
    }
}

IpodHandler::~IpodHandler()
{
    DEBUG_BLOCK
    delete m_tempdir;
    debug() << "Cleaning up Ipod Database";
    if ( m_itdb )
        itdb_free( m_itdb );

    debug() << "End of destructor reached";
}


bool
IpodHandler::initializeIpod()
{
    DEBUG_BLOCK
    QDir dir( mountPoint() );
    if( !dir.exists() )
    {
        debug() << "Media device: Mount point does not exist!";
        return false;
    }

    debug() << "initializing iPod mounted at " << mountPoint();

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

    debug() << "Init 1";

    QString realPath;
    if(!pathExists( itunesDir(), &realPath) )
    {
        dir.setPath(realPath);
        dir.mkdir(dir.absolutePath());
    }
    if(!dir.exists())
        return false;

    debug() << "Init 2";

    if(!pathExists( itunesDir( "Music" ), &realPath) )
    {
        dir.setPath(realPath);
        dir.mkdir(dir.absolutePath());
    }
    if(!dir.exists())
        return false;

    debug() << "Init 3";

    if(!pathExists( itunesDir( "iTunes" ), &realPath) )
    {
        dir.setPath(realPath);
        dir.mkdir(dir.absolutePath());
    }
    if(!dir.exists())
        return false;

    debug() << "Init 4";

    if( !writeITunesDB( false ) )
        return false;

    debug() << "Init 5";

    return true;
}

void
IpodHandler::detectModel()
{
    DEBUG_BLOCK
    // set some sane default values
    m_isShuffle = false;

    m_supportsArtwork = true;

    m_supportsVideo = false;
    m_isIPhone = false;
    m_needsFirewireGuid = false;
    m_rockboxFirmware = false;

    // needs recent libgpod-0.3.3 from cvs
    bool guess = false;
    if( m_itdb && m_device )
    {
        debug() << "Attempting to get info...";
        const Itdb_IpodInfo *ipodInfo = itdb_device_get_ipod_info( m_device );
        debug() << "Got ipodinfo";
        const gchar *modelString = 0;
        #ifdef FOUND_GDK
        m_supportsArtwork = itdb_device_supports_artwork( m_device );
        #else
        m_supportsArtwork = false;
        #endif
        debug() << "Supports Artwork: " << ( m_supportsArtwork ? "true" : "false" );
        QString musicdirs;
        musicdirs.setNum( itdb_musicdirs_number(m_itdb) );
        debug() << "Musicdirs: " << musicdirs;

        if( ipodInfo )
        {
            debug() << "Checking info...";
            debug() << "Capacity is: " << ipodInfo->capacity;
            modelString = itdb_info_get_ipod_model_name_string ( ipodInfo->ipod_model );

            debug() << "Ipod model: " << QString::fromUtf8( modelString );

            switch( ipodInfo->ipod_model )
            {
            case ITDB_IPOD_MODEL_SHUFFLE:

            case ITDB_IPOD_MODEL_SHUFFLE_SILVER:
            case ITDB_IPOD_MODEL_SHUFFLE_PINK:
            case ITDB_IPOD_MODEL_SHUFFLE_BLUE:
            case ITDB_IPOD_MODEL_SHUFFLE_GREEN:
            case ITDB_IPOD_MODEL_SHUFFLE_ORANGE:
            case ITDB_IPOD_MODEL_SHUFFLE_PURPLE:

                m_isShuffle = true;
                break;

            case ITDB_IPOD_MODEL_IPHONE_1:
            case ITDB_IPOD_MODEL_TOUCH_BLACK:
                m_isIPhone = true;
                debug() << "detected iPhone/iPod Touch" << endl;
                break;
            case ITDB_IPOD_MODEL_CLASSIC_SILVER:
            case ITDB_IPOD_MODEL_CLASSIC_BLACK:
                debug() << "detected iPod classic";

            case ITDB_IPOD_MODEL_VIDEO_WHITE:
            case ITDB_IPOD_MODEL_VIDEO_BLACK:
            case ITDB_IPOD_MODEL_VIDEO_U2:
                m_supportsVideo = true;
                debug() << "detected video-capable iPod";
                break;
            case ITDB_IPOD_MODEL_MOBILE_1:
                m_isMobile = true;
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


            debug() << "Generation is: " << ipodInfo->ipod_generation;
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

        }
        if( modelString )
            m_name = QString( "iPod %1" ).arg( QString::fromUtf8( modelString ) );

        if( m_needsFirewireGuid )
        {
            gchar *fwid = itdb_device_get_sysinfo( m_device, "FirewireGuid" );
            if( !fwid )
            {

            }
            else
               g_free( fwid );
        }
    }
    else
    {
        debug() << "iPod type detection failed, no video support";
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
        #ifdef FOUND_GDK
        m_supportsArtwork = true;
        #else
        m_supportsArtwork = false;
        #endif
        m_supportsVideo = true;
    }

    if( pathExists( ":.rockbox" ) )
    {
        debug() << "RockBox firmware detected" << endl;
        m_rockboxFirmware = true;
    }
}

bool
IpodHandler::pathExists( const QString &ipodPath, QString *realPath )
{
    QDir curDir( mountPoint() );
    QString curPath = mountPoint();
    QStringList components = ipodPath.split( ':' );

    bool found = false;
    QStringList::iterator it = components.begin();
    for( ; it != components.end(); ++it )
    {
        found = false;
        for(uint i=0; i<curDir.count(); i++)
        {
            if( curDir[i].toLower() == (*it).toLower())
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
    {
        curPath += '/' + *it;
    }

    if( realPath )
        *realPath = curPath;

    return found;
}

bool
IpodHandler::writeITunesDB( bool threaded )
{
DEBUG_BLOCK
    if(!m_itdb)
        return false;

    if(m_dbChanged)
    {
        bool ok = false;
        if( !threaded )
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
                        debug() << "itdb_write error: " << error->message;
                    else
                        debug() << "itdb_write error: " << "error->message == 0!";
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
                            debug() << "itdb_shuffle_write error: " << error->message;
                        else
                            debug() << "itdb_shuffle_write error: " << "error->message == 0!";
                        g_error_free (error);
                    }
                    error = 0;
                    ok = false;
                }
            }
        }
        else
        {

        }

        if( ok )
        {
            m_dbChanged = false;
        }
        else
        {
            debug() << "Failed to write iPod database";
        }

        return ok;
    }

    debug() << "writeItunesDB is returning true";

    return true;
}

QString
IpodHandler::itunesDir(const QString &p) const
{
    QString base( ":iPod_Control" );
    if( m_isMobile )
        base = ":iTunes:iTunes_Control";

    if( !p.startsWith( ':' ) )
        base += ':';
    return base + p;
}

bool
IpodHandler::deleteTrackFromDevice( const Meta::IpodTrackPtr &track )
{
    DEBUG_BLOCK
    Itdb_Track *ipodtrack = track->getIpodTrack();

    // delete file
    KUrl url;
    url.setPath( realPath( ipodtrack->ipod_path ) );
    deleteFile( url );

    // remove it from the ipod database, ipod playlists and all

    if( removeDBTrack( ipodtrack ) )
        if( writeITunesDB( false ) )
            return true;

    return false;

}

bool
IpodHandler::deleteTracksFromDevice( const Meta::TrackList &tracks )
{
    DEBUG_BLOCK

    foreach( Meta::TrackPtr track, tracks )
    {
        Itdb_Track *ipodtrack = Meta::IpodTrackPtr::staticCast(track)->getIpodTrack();

        // delete file
        KUrl url;
        url.setPath( realPath( ipodtrack->ipod_path ) );
        deleteFile( url );

        // remove it from the ipod database, ipod playlists and all

        if ( !removeDBTrack( ipodtrack ) )
        {
            debug() << "Error: failed to remove track from db";
            return false;
        }

    }

    return ( writeITunesDB( false ) );

}

void
IpodHandler::copyTrackToDevice( const Meta::TrackPtr &track )
{
    DEBUG_BLOCK

    debug() << "Mountpoint is: " << mountPoint();

    KUrl url = determineURLOnDevice(track);

    debug() << "Url's path is: " << url.path();

    // check if path exists and make it if needed
    QFileInfo finfo( url.path() );
    QDir dir = finfo.dir();
    while ( !dir.exists() )
    {
        QString path = dir.absolutePath();
        QDir parentdir;
        QDir create;
        do
        {
            create.setPath(path);
            path = path.section("/", 0, path.indexOf('/')-1);
            parentdir.setPath(path);
        }
        while( !path.isEmpty() && !(path==mountPoint()) && !parentdir.exists() );
        debug() << "trying to create \"" << path << "\"";
        if(!create.mkdir( create.absolutePath() ))
        {
            break;
        }
    }
    if ( !dir.exists() )
    {
        debug() << "Creating directory failed";
        return;
    }

    debug() << "About to copy from: " << track->playableUrl().path();

    if( !kioCopyTrack( KUrl::fromPath( track->playableUrl().path() ), url ) )
    {
        return;
    }

    // NOTE: PODCASTS NOT YET PORTED

//    PodcastInfo *podcastInfo = 0;


    /*
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
*/
    insertTrackIntoDB( url, track );
    if( !m_trackCreated )
    {
        debug() << "Track failed to create, aborting database write!";
    }

    return;
}

void
IpodHandler::insertTrackIntoDB( const KUrl &url, const Meta::TrackPtr &track )
{
    DEBUG_BLOCK
    Itdb_Track *ipodtrack = itdb_track_new();

    updateTrackInDB( url, track, ipodtrack ); // get information from track

    if( m_trackCreated )
    {

    debug() << "Adding " << QString::fromUtf8( ipodtrack->artist) << " - " << QString::fromUtf8( ipodtrack->title );

    addTrackInDB( ipodtrack );

        // add track to collection
    addIpodTrackToCollection( ipodtrack );
    }
    else
        debug() << "Failed to create track, aborting insertion!";
}

void
IpodHandler::updateTrackInDB( const KUrl &url, const Meta::TrackPtr &track, Itdb_Track *existingIpodTrack )
{
    DEBUG_BLOCK
    if( !m_itdb )
        return;

    m_trackCreated = true;

    QString pathname = url.path();

    Itdb_Track *ipodtrack = 0;
    if( !existingIpodTrack )
    {
        // NOTE: This should never happen!
        debug() << "No track passed in, failure!";
        m_trackCreated = false;
        return;
    }
    else
        ipodtrack = existingIpodTrack;

    QString type = pathname.section('.', -1).toLower();

    debug() << "Path before put in ipod_path: " << pathname;

    ipodtrack->ipod_path = g_strdup( ipodPath(pathname).toLatin1() );
    debug() << "on iPod: " << ipodtrack->ipod_path;

    if( !track->name().isEmpty() )
        ipodtrack->title = g_strdup( track->name().toUtf8() );
    else
        ipodtrack->title = g_strdup( KUrl::fromPath( track->uidUrl() ).fileName().toUtf8() );
    
    ipodtrack->album = g_strdup( track->album()->name().toUtf8() );
    ipodtrack->artist = g_strdup( track->artist()->name().toUtf8() );
    ipodtrack->genre = g_strdup( track->genre()->name().toUtf8() );

    ipodtrack->mediatype = ITDB_MEDIATYPE_AUDIO;
    bool audiobook = false;
    if(type=="wav")
    {
        ipodtrack->filetype = g_strdup( "wav" );
    }
    else if(type=="mp3" || type=="mpeg")
    {
        ipodtrack->filetype = g_strdup( "mpeg" );
    }
    else if(type=="aac" || type=="m4a" || (!m_supportsVideo && type=="mp4"))
    {
        ipodtrack->filetype = g_strdup( "mp4" );
    }
    else if(type=="m4b")
    {
        audiobook = true;
        ipodtrack->filetype = g_strdup( "mp4" );
    }
    else if(type=="m4v" || type=="mp4v" || type=="mov" || type=="mpg" || type=="mp4")
    {
        ipodtrack->filetype = g_strdup( "m4v video" );
        ipodtrack->movie_flag = 0x01; // for videos
        ipodtrack->mediatype = ITDB_MEDIATYPE_MOVIE;
    }
    // TODO: NYI, TagLib calls need to be ported
    /*
    else if(type=="aa")
    {
        audiobook = true;
        ipodtrack->filetype = g_strdup( "audible" );

        TagLib::Audible::File f( QFile::encodeName( url.path() ) );
        TagLib::Audible::Tag *t = f.getAudibleTag();
        if( t )
            ipodtrack->drm_userid = t->userID();
        // libgpod also tries to set those, but this won't work
        ipodtrack->unk126 = 0x01;
        ipodtrack->unk144 = 0x0029;

    }
    */
    else
    {
        ipodtrack->filetype = g_strdup( type.toUtf8() );
    }


    QString genre = track->genre()->name();
    if( genre.startsWith("audiobook", Qt::CaseInsensitive) )
        audiobook = true;
    if( audiobook )
    {
        ipodtrack->remember_playback_position |= 0x01;
        ipodtrack->skip_when_shuffling |= 0x01;
        ipodtrack->mediatype = ITDB_MEDIATYPE_AUDIOBOOK;
    }

    ipodtrack->composer = g_strdup( track->composer()->name().toUtf8() );
    ipodtrack->comment = g_strdup( track->comment().toUtf8() );
    ipodtrack->track_nr = track->trackNumber();
    ipodtrack->cd_nr = track->discNumber();
// BPM isn't present in Amarok 2 at this time Jul. 8 2008
//    ipodtrack->BPM = static_cast<int>( track->bpm() );
    bool ok;
    int year = track->year()->name().toInt( &ok, 10 );
    if(ok)
        ipodtrack->year = year;
    ipodtrack->size = track->filesize();
    if( ipodtrack->size == 0 )
    {
        debug() << "filesize is zero for " << ipodtrack->ipod_path << ", expect strange problems with your ipod";
    }
    ipodtrack->bitrate = track->bitrate();
    ipodtrack->samplerate = track->sampleRate();
    ipodtrack->tracklen = track->length()*1000;

    m_dbChanged = true;

// In Amarok 2, tracks come from many places, no such reliable info
/*
    //Get the createdate from database
    QueryBuilder qb;
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valCreateDate );
    qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valURL, metaBundle.url().path() );
    QStringList values = qb.run();

    //Add to track info if present
    if ( values.count() ) {
        uint createdate = values.first().toUInt();
        ipodtrack->time_added = itdb_time_host_to_mac( createdate );
        ipodtrack->time_modified = itdb_time_host_to_mac( createdate );
    }
*/
// TODO: podcasts/compilations NYI
    /*
    if(podcastInfo)
    {
        ipodtrack->skip_when_shuffling = 0x01; // skip  when shuffling
        ipodtrack->remember_playback_position = 0x01; // remember playback position
        // FIXME: ipodtrack->unk176 = 0x00020000; // for podcasts
        ipodtrack->mark_unplayed = podcastInfo->listened ? 0x01 : 0x02;
        ipodtrack->mediatype =
                ipodtrack->mediatype==ITDB_MEDIATYPE_MOVIE
                ?  ITDB_MEDIATYPE_PODCAST | ITDB_MEDIATYPE_MOVIE
            : ITDB_MEDIATYPE_PODCAST;

        ipodtrack->flag4 = 0x01; // also show description on iPod
        QString plaindesc = podcastInfo->description;
        plaindesc.remove( QRegExp("<[^>]*>") );
        ipodtrack->description = g_strdup( plaindesc.toUtf8() );
        ipodtrack->subtitle = g_strdup( plaindesc.toUtf8() );
        ipodtrack->podcasturl = g_strdup( podcastInfo->url.toUtf8() );
        ipodtrack->podcastrss = g_strdup( podcastInfo->rss.toUtf8() );
        //ipodtrack->category = g_strdup( i18n( "Unknown" ) );
        ipodtrack->time_released = itdb_time_host_to_mac( podcastInfo->date.toTime_t() );
        //ipodtrack->compilation = 0x01; // this should have made the ipod play a sequence of podcasts
    }
    else
    {
        if( metaBundle.compilation() == MetaBundle::CompilationYes )
        {
            ipodtrack->compilation = 0x01;
        }
        else
        {
            ipodtrack->compilation = 0x00;
        }
    }
    */

    if( track->album()->hasImage( 50 ) )
    {
        QPixmap image = track->album()->image( 50 );
        debug() << "Got image of height: " << image.height() << "and width: " << image.width();
        setCoverArt( ipodtrack, image );
    }
    else
        debug() << "No image available";

    debug() << "Adding " << QString::fromUtf8( ipodtrack->artist) << " - " << QString::fromUtf8( ipodtrack->title );
}

void
IpodHandler::addTrackInDB( Itdb_Track *ipodtrack )
{
    DEBUG_BLOCK

    debug() << "Adding " << QString::fromUtf8( ipodtrack->artist) << " - " << QString::fromUtf8( ipodtrack->title );
    itdb_track_add(m_itdb, ipodtrack, -1);

    // TODO: podcasts NYI
    // if(podcastInfo)
    if( false )
    {
        Itdb_Playlist *podcasts = itdb_playlist_podcasts(m_itdb);
        if(!podcasts)
        {
            podcasts = itdb_playlist_new("Podcasts", false);
            itdb_playlist_add(m_itdb, podcasts, -1);
            itdb_playlist_set_podcasts(podcasts);
        }
        itdb_playlist_add_track(podcasts, ipodtrack, -1);
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
        }
        itdb_playlist_add_track(mpl, ipodtrack, -1);
    }
}

bool
IpodHandler::removeDBTrack( Itdb_Track *track )
{
    if( !m_itdb || !track )
        return false;

    if( track->itdb != m_itdb )
        return false;

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
IpodHandler::kioCopyTrack( const KUrl &src, const KUrl &dst )
{
    DEBUG_BLOCK

    debug() << "Copying from *" << src << "* to *" << dst << "*";

    KIO::FileCopyJob *job = KIO::file_copy( src, dst,
                                            -1 /* permissions */,
                                            KIO::HideProgressInfo );

    connect( job, SIGNAL( result( KJob * ) ),
             this,  SLOT( fileTransferred( KJob * ) ) );

    The::statusBar()->newProgressOperation( job, i18n( "Transferring Tracks to iPod" )  );
    job->start();

    return true;
}

void
IpodHandler::fileTransferred( KJob *job )  //SLOT
{
        if ( job->error() )
        {
            m_copyFailed = true;
            debug() << "file transfer failed: " << job->errorText();
        }
        else
        {
            m_copyFailed = false;
        }

        m_wait = false;
}

void
IpodHandler::deleteFile( const KUrl &url )
{
    debug() << "deleting " << url.prettyUrl();

    KIO::DeleteJob *job = KIO::del( url, KIO::HideProgressInfo );
    connect( job, SIGNAL( result( KJob * ) ),
             this,  SLOT( fileDeleted( KJob * ) ) );

    The::statusBar()->newProgressOperation( job, i18n( "Deleting Tracks from iPod" ) );
    job->start();

    return;
}

void
IpodHandler::fileDeleted( KJob *job )  //SLOT
{
    if(job->error())
    {
        debug() << "file deletion failed: " << job->errorText();
    }
}

KUrl
IpodHandler::determineURLOnDevice( const Meta::TrackPtr &track )
{
    DEBUG_BLOCK
    if( !m_itdb )
    {
        debug() << "m_itdb is NULL";
        return KUrl();
    }

    QString type = track->type();

    QString trackpath;
    QString realpath;
    do
    {
        int num = qrand() % 1000000;
        int music_dirs = itdb_musicdirs_number(m_itdb) > 1 ? itdb_musicdirs_number(m_itdb) : 20;
        int dir = num % music_dirs;
        QString dirname;
        debug() << "itunesDir(): " << itunesDir();
        dirname = QString( "%1Music:F%2" ).arg( "iPod_Control:" ).arg( QString::number( dir, 10 ), 2, QLatin1Char( '0' ) );

        debug() << "Copying to dirname: " << dirname;
        if( !pathExists( dirname ) )
        {
            QString realdir = realPath(dirname.toLatin1());
            QDir qdir( realdir );
            qdir.mkdir( realdir );
        }
        QString filename;
        filename = QString( ":kpod%1.%2" ).arg( QString::number( num, 10 ), 7, QLatin1Char( '0' ) ).arg(type);
        trackpath = dirname + filename;
    }
    while( pathExists( trackpath, &realpath ) );

    return realpath;
}

QString
IpodHandler::ipodPath( const QString &realPath )
{
    if( m_itdb )
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

QString
IpodHandler::realPath( const char *ipodPath )
{
    QString path;
    if( m_itdb )
    {
        path = QFile::decodeName(itdb_get_mountpoint(m_itdb));
        path.append(QString(ipodPath).replace(':', "/"));
    }

    return path;
}

void
IpodHandler::addIpodTrackToCollection( Itdb_Track *ipodtrack )
{
    TrackMap trackMap = m_memColl->trackMap();
    ArtistMap artistMap = m_memColl->artistMap();
    AlbumMap albumMap = m_memColl->albumMap();
    GenreMap genreMap = m_memColl->genreMap();
    ComposerMap composerMap = m_memColl->composerMap();
    YearMap yearMap = m_memColl->yearMap();

    IpodTrackPtr track( new IpodTrack( m_memColl ) );

    /* 1-liner info retrieval */

    getBasicIpodTrackInfo( ipodtrack, track );

    /* map-related info retrieval */
    setupArtistMap( ipodtrack, track, artistMap );
    setupAlbumMap( ipodtrack, track, albumMap );
    setupGenreMap( ipodtrack, track, genreMap );
    setupComposerMap( ipodtrack, track, composerMap );
    setupYearMap( ipodtrack, track, yearMap );

    /* trackmap also soon to be subordinated */
    
    trackMap.insert( track->uidUrl(), TrackPtr::staticCast( track ) );

    track->setIpodTrack( ipodtrack ); // convenience pointer
    // NOTE: not supporting adding track that's already on a playlist
    //ipodTrackMap.insert( ipodtrack, track ); // map for playlist formation

    // Finally, assign the created maps to the collection
    m_memColl->acquireWriteLock();
    m_memColl->setTrackMap( trackMap );
    m_memColl->setArtistMap( artistMap );
    m_memColl->setAlbumMap( albumMap );
    m_memColl->setGenreMap( genreMap );
    m_memColl->setComposerMap( composerMap );
    m_memColl->setYearMap( yearMap );
    m_memColl->releaseLock();

    return;
}

void
IpodHandler::getBasicIpodTrackInfo( Itdb_Track *ipodtrack, Meta::IpodTrackPtr track )
{
    /* 1-liner info retrieval */

    track->setTitle( QString::fromUtf8( ipodtrack->title ) );
    track->setLength( ( ipodtrack->tracklen ) / 1000 );
    track->setTrackNumber( ipodtrack->track_nr );
    track->setComment( QString::fromUtf8( ipodtrack->comment ) );
    track->setDiscNumber( ipodtrack->cd_nr );
    track->setBitrate( ipodtrack->bitrate );
    track->setBpm( ipodtrack->BPM );
    track->setFileSize( ipodtrack->size );

    QString path = QString( ipodtrack->ipod_path ).split( ':' ).join( "/" );
    path = m_mountPoint + path;
    track->setPlayableUrl( path );

    QString filetype = QString::fromUtf8( ipodtrack->filetype );


    if( filetype == "mpeg" )
        track->setType( "mp3" );

    return;
}

#ifdef FOUND_GDK

void
IpodHandler::getCoverArt( Itdb_Track *ipodtrack, Meta::IpodTrackPtr track )
{
    KTemporaryFile tempImageFile;

    tempImageFile.setSuffix( ".jpeg" ); // default suffix jpeg
    QFileInfo tempImageFileInfo( tempImageFile ); // get info for path
    QString tempImagePath = tempImageFileInfo.absoluteFilePath(); // path

    Itdb_Thumb *thumb = NULL;
    GdkPixbuf *gpixbuf = NULL;
    QString thumbPath;

    // pull image out of ipod

    if( ipodtrack->has_artwork == 0x01 )
    {
        // try small first

        //debug() << "Attempting to get small cover";
        thumb = itdb_artwork_get_thumb_by_type ( ipodtrack->artwork, ITDB_THUMB_COVER_SMALL );

        // then large if needed
        if( thumb == NULL)
        {
//            //debug() << "Failed to get small cover, trying large";
            thumb = itdb_artwork_get_thumb_by_type ( ipodtrack->artwork, ITDB_THUMB_COVER_LARGE );
        }

        if( thumb != NULL)
        {
//            //debug() << "Got a valid thumb, attempting to fetch pixbuf";
            gpixbuf = (GdkPixbuf*) itdb_thumb_get_gdk_pixbuf( m_device, thumb );
        }
        else
        {
//            //debug() << "No valid thumb gotten, not fetching pixbuf";
//            //debug() << "Attempting to fetch artwork via a thumb";
//            //debug() << "Iterating through thumbs";

            GList *thumbs = ipodtrack->artwork->thumbnails;

            for(; thumbs; thumbs = thumbs->next)
            {
                Itdb_Thumb *curThumb = ( Itdb_Thumb * )thumbs->data;
                if( curThumb == NULL)
                    continue;
                
//                //debug() << "Found valid thumb while iterating";
//                //debug() << "Cover probably set by iTunes";
//                //debug() << "Type is the following:";

                switch( curThumb->type )
                {
                    case ITDB_THUMB_PHOTO_SMALL:
                        //debug() << "ITDB_THUMB_PHOTO_SMALL";
                        break;
                    case ITDB_THUMB_PHOTO_LARGE:
                        //debug() << "ITDB_THUMB_PHOTO_LARGE";
                        break;
                    case ITDB_THUMB_PHOTO_FULL_SCREEN:
                        //debug() << "ITDB_THUMB_PHOTO_FULL_SCREEN";
                        break;
                    case ITDB_THUMB_PHOTO_TV_SCREEN:
                        //debug() << "ITDB_THUMB_PHOTO_TV_SCREEN";
                        break;
                    case ITDB_THUMB_COVER_XLARGE:
                        //debug() << "ITDB_THUMB_COVER_XLARGE";
                        break;
                    case ITDB_THUMB_COVER_MEDIUM:
                        //debug() << "ITDB_THUMB_COVER_MEDIUM";
                        break;
                    case ITDB_THUMB_COVER_SMEDIUM:
                        //debug() << "ITDB_THUMB_COVER_SMEDIUM";
                        break;
                    case ITDB_THUMB_COVER_XSMALL:
                        //debug() << "ITDB_THUMB_COVER_XSMALL";
                        break;

                    default:
                        //debug() << "Unknown Thumb Type";
                        break;
                }
                ////debug() << "Type is: " << curThumb->type;

                thumb = curThumb;
                break;
                
            }

            
            if( thumb != NULL)
            {
                thumbPath = QString::fromUtf8( itdb_thumb_get_filename( m_device, thumb ) );

                gpixbuf = (GdkPixbuf*) itdb_thumb_get_gdk_pixbuf( m_device, thumb );
            }
        }
    }

    if(gpixbuf != NULL)
    {
        // temporarily save to file
        gdk_pixbuf_save( gpixbuf, QFile::encodeName( tempImagePath ), "jpeg", 0, (const char*) 0 );

        // pull temporary file's image out as QImage
        QImage image( tempImagePath );
        track->album()->setImage( image );

        // fix memleak
        gdk_pixbuf_unref ( gpixbuf );
    }

    return;
}

#endif

void
IpodHandler::setCoverArt( Itdb_Track *ipodtrack, const QPixmap &image )
{
    DEBUG_BLOCK
    KTemporaryFile tempImageFile; // create a temp file to save pixmap
    // use tempdir's path
    tempImageFile.setPrefix( m_tempdir->name() );
    tempImageFile.setSuffix( ".jpeg" ); // default suffix jpeg
    // tempdir will nuke the file afterward anyway
    tempImageFile.setAutoRemove( false);

    if( !tempImageFile.open() )
    {
        debug() << "Failed to create temp file";
        return;
    }

    QFileInfo tempImageFileInfo( tempImageFile ); // get info for path
    QString tempImagePath = tempImageFileInfo.absoluteFilePath(); // path
    image.save( tempImagePath ); // temporarily save pixmap

    bool success = false;

    debug() << "Adding image that's temporarily at: " << tempImagePath;

    success = itdb_track_set_thumbnails( ipodtrack, QFile::encodeName( tempImagePath ) );
    
    if( success )
    {
        debug() << "Image added successfully!";
        ipodtrack->has_artwork = 0x01;
    }
    else
    {
        debug() << "Image failed to add!";
        ipodtrack->has_artwork = 0x02;
    }
}

void
IpodHandler::setupArtistMap( Itdb_Track *ipodtrack, Meta::IpodTrackPtr track, ArtistMap &artistMap )
{
    QString artist( QString::fromUtf8( ipodtrack->artist ) );
    IpodArtistPtr artistPtr;

    if ( artistMap.contains( artist ) )
    {
        artistPtr = IpodArtistPtr::staticCast( artistMap.value( artist ) );
    }
    else
    {
        artistPtr = IpodArtistPtr( new IpodArtist( artist ) );
        artistMap.insert( artist, ArtistPtr::staticCast( artistPtr ) );
    }

    artistPtr->addTrack( track );
    track->setArtist( artistPtr );
}

void
IpodHandler::setupAlbumMap( Itdb_Track *ipodtrack, Meta::IpodTrackPtr track, AlbumMap &albumMap )
{
    QString album( QString::fromUtf8( ipodtrack->album ) );
    IpodAlbumPtr albumPtr;

    if ( albumMap.contains( album ) )
        albumPtr = IpodAlbumPtr::staticCast( albumMap.value( album ) );

    else
    {
        albumPtr = IpodAlbumPtr( new IpodAlbum( album ) );
        albumMap.insert( album, AlbumPtr::staticCast( albumPtr ) );
    }

    albumPtr->addTrack( track );
    track->setAlbum( albumPtr );
}

void
IpodHandler::setupGenreMap( Itdb_Track *ipodtrack, Meta::IpodTrackPtr track, GenreMap &genreMap )
{
    QString genre = ipodtrack->genre;
    IpodGenrePtr genrePtr;

    if ( genreMap.contains( genre ) )
        genrePtr = IpodGenrePtr::staticCast( genreMap.value( genre ) );

    else
    {
        genrePtr = IpodGenrePtr( new IpodGenre( genre ) );
        genreMap.insert( genre, GenrePtr::staticCast( genrePtr ) );
    }

    genrePtr->addTrack( track );
    track->setGenre( genrePtr );
}

void
IpodHandler::setupComposerMap( Itdb_Track *ipodtrack, Meta::IpodTrackPtr track, ComposerMap &composerMap )
{
    QString composer ( QString::fromUtf8( ipodtrack->composer ) );
    IpodComposerPtr composerPtr;

    if ( composerMap.contains( composer ) )
    {
        composerPtr = IpodComposerPtr::staticCast( composerMap.value( composer ) );
    }
    else
    {
        composerPtr = IpodComposerPtr( new IpodComposer( composer ) );
        composerMap.insert( composer, ComposerPtr::staticCast( composerPtr ) );
    }

    composerPtr->addTrack( track );
    track->setComposer( composerPtr );
}

void
IpodHandler::setupYearMap( Itdb_Track *ipodtrack, Meta::IpodTrackPtr track, YearMap &yearMap )
{
    QString year;
    year = year.setNum( ipodtrack->year );
    IpodYearPtr yearPtr;
    if ( yearMap.contains( year ) )
        yearPtr = IpodYearPtr::staticCast( yearMap.value( year ) );
    else
    {
        yearPtr = IpodYearPtr( new IpodYear( year ) );
        yearMap.insert( year, YearPtr::staticCast( yearPtr ) );
    }
    yearPtr->addTrack( track );
    track->setYear( yearPtr );
}

void
IpodHandler::parseTracks()
{
    DEBUG_BLOCK

    TrackMap trackMap;
    ArtistMap artistMap;
    AlbumMap albumMap;
    GenreMap genreMap;
    ComposerMap composerMap;
    YearMap yearMap;

    QMap<Itdb_Track*, IpodTrackPtr> ipodTrackMap;

    GList *cur;

    /* iterate through tracklist and add to appropriate map */
    for( cur = m_itdb->tracks; cur; cur = cur->next )
    {
        /* ipodtrack - provides libgpod itdb info */
        /* track - the new track whose data is being set up */
        Itdb_Track *ipodtrack = (Itdb_Track*) cur->data;

        IpodTrackPtr track( new IpodTrack( m_memColl ) );

        getBasicIpodTrackInfo( ipodtrack, track );

        /* map-related info retrieval */

        setupArtistMap( ipodtrack, track, artistMap );
        setupAlbumMap( ipodtrack, track, albumMap );
        setupGenreMap( ipodtrack, track, genreMap );
        setupComposerMap( ipodtrack, track, composerMap );
        setupYearMap( ipodtrack, track, yearMap );

        /* cover art */

        //debug() << "Supports artwork: " << ( m_supportsArtwork ? "true" : "false" );
        #ifdef FOUND_GDK
        if( m_supportsArtwork )
            getCoverArt( ipodtrack, track );
        #endif

        //getCoverArt( ipodtrack, track );
        
        /* TrackMap stuff to be subordinated later */
        trackMap.insert( track->uidUrl(), TrackPtr::staticCast( track ) );

        track->setIpodTrack( ipodtrack ); // convenience pointer
        ipodTrackMap.insert( ipodtrack, track ); // map for playlist formation
    }

    // Iterate through ipod's playlists to set track's playlists

    GList *member = 0;

    for ( cur = m_itdb->playlists; cur; cur = cur->next )
    {
        Itdb_Playlist *ipodplaylist = ( Itdb_Playlist * ) cur->data;
        for ( member = ipodplaylist->members; member; member = member->next )
        {
            Itdb_Track *ipodtrack = ( Itdb_Track * )member->data;
            IpodTrackPtr track = ipodTrackMap.value( ipodtrack );
            track->addIpodPlaylist( ipodplaylist );
        }
    }

    // Finally, assign the created maps to the collection

    m_memColl->acquireWriteLock();
    m_memColl->setTrackMap( trackMap );
    m_memColl->setArtistMap( artistMap );
    m_memColl->setAlbumMap( albumMap );
    m_memColl->setGenreMap( genreMap );
    m_memColl->setComposerMap( composerMap );
    m_memColl->setYearMap( yearMap );
    m_memColl->releaseLock();
}

