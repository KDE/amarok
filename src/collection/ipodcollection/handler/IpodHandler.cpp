/**************************************************************************
 * Ported to Collection Framework: *
 * copyright            : (C) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com>
 * copyright            : (C) 2009 Seb Ruiz <ruiz@kde.org>
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

#include "MetaQueryMaker.h"
#include "QueryMaker.h"

#ifdef GDK_FOUND
extern "C" {
#include <glib-object.h> // g_type_init
#include <gdk-pixbuf/gdk-pixbuf.h>
}
#endif

#include "File.h" // for KIO file handling

#include <KCodecs> // KMD5
#include <KIO/Job>
#include <KIO/CopyJob>
#include <KIO/DeleteJob>
#include <KIO/Scheduler>
#include "kjob.h"
#include <threadweaver/ThreadWeaver.h>
#include <KUrl>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>
#include <QPixmap>
#include <QString>
#include <QStringList>
#include <QTime>

using namespace Meta;

IpodHandler::IpodHandler( IpodCollection *mc, const QString& mountPoint )
    : MediaDeviceHandler( mc )
    //, m_memColl( mc )
    , m_masterPlaylist( 0 )
    , m_jobcounter( 0 )
    , m_statusbar( 0 )
    , m_autoConnect( false )
    , m_mountPoint( mountPoint )
    , m_name()
    , m_isShuffle( false )
    , m_isMobile( false )
    , m_isIPhone( false )
    , m_supportsArtwork( false )
    , m_supportsVideo( false )
    , m_rockboxFirmware( false )
    , m_needsFirewireGuid( false )
    , m_dbChanged( false )
    , m_copyFailed( false )
    , m_isCanceled( false )
    , m_wait( false )
    , m_trackCreated( false )
    , m_tempdir( new KTempDir() )
{
    DEBUG_BLOCK

    g_type_init();

    GError *err = 0;
    m_success = false;

    // Assuming database exists for now, later will port init db code
    debug() << "Calling the db parser";
    m_itdb = itdb_parse( QFile::encodeName( m_mountPoint ),  &err );

    if( err )
    {
        debug() << "There was an error, attempting to free db: " << err->message;
        g_error_free( err );
        if ( m_itdb )
        {
            itdb_free( m_itdb );
            m_itdb = 0;
        }
    }
    else
    {
        m_tempdir->setAutoRemove( true );

        // read device info
        debug() << "Getting model information";
        detectModel(); // get relevant info about device

        qsrand( QTime::currentTime().msec() ); // random number used for folder number generation

        m_success = true;

        debug() << "Succeeded: " << m_success;
    }
}

IpodHandler::~IpodHandler()
{
    DEBUG_BLOCK
    delete m_tempdir;
    // Write to DB before closing, for ratings updates etc.
    //debug() << "Writing to Ipod DB";
    //writeDatabase();
    debug() << "Cleaning up Ipod Database";
    if ( m_itdb )
        itdb_free( m_itdb );

    debug() << "End of destructor reached";
}

bool
IpodHandler::isWritable() const
{
    // TODO: check if read-only
    return true;
}

#if 0
/** Observer Methods **/
void
IpodHandler::metadataChanged( TrackPtr track )
{
    DEBUG_BLOCK

    Meta::IpodTrackPtr trackPtr = Meta::IpodTrackPtr::staticCast( track );
    KUrl trackUrl = KUrl::fromPath( trackPtr->uidUrl() );

    // TODO: database methods abstraction needed
    //updateTrackInDB( trackUrl, track, trackPtr->getIpodTrack() );
}

void
IpodHandler::metadataChanged( ArtistPtr artist )
{
    Q_UNUSED( artist );
}

void
IpodHandler::metadataChanged( AlbumPtr album )
{
    Q_UNUSED( album );
}

void
IpodHandler::metadataChanged( GenrePtr genre )
{
    Q_UNUSED( genre );
}

void
IpodHandler::metadataChanged( ComposerPtr composer )
{
    Q_UNUSED( composer );
}

void
IpodHandler::metadataChanged( YearPtr year )
{
    Q_UNUSED( year );
}
#endif
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
    if( !m_itdb )
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
        dir.setPath( realPath );
        dir.mkdir( dir.absolutePath() );
    }
    if( !dir.exists() )
        return false;

    if( !pathExists( itunesDir( "Music" ), &realPath ) )
    {
        dir.setPath( realPath );
        dir.mkdir( dir.absolutePath() );
    }
    if( !dir.exists() )
        return false;

    if( !pathExists( itunesDir( "iTunes" ), &realPath ) )
    {
        dir.setPath( realPath );
        dir.mkdir( dir.absolutePath() );
    }
    if( !dir.exists() )
        return false;

    // NOTE: writing itunes DB allowed to block since
    // initializing a device is rare, and requires focus
    // to minimize possible error
    // TODO: database methods abstraction needed
    //if( !writeITunesDB( false ) )
     //   return false;

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
    if( m_itdb && m_itdb->device )
    {
        debug() << "Attempting to get info...";
        const Itdb_IpodInfo *ipodInfo = itdb_device_get_ipod_info( m_itdb->device );
        debug() << "Got ipodinfo";
        const gchar *modelString = 0;
        #ifdef GDK_FOUND
        m_supportsArtwork = itdb_device_supports_artwork( m_itdb->device );
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
            gchar *fwid = itdb_device_get_sysinfo( m_itdb->device, "FirewireGuid" );
            if( fwid )
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
        #ifdef GDK_FOUND
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
        for( uint i = 0;i < curDir.count(); i++ )
        {
            if( curDir[i].toLower() == (*it).toLower() )
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

    QMutexLocker locker( &m_dbLocker );
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
                        debug() << "itdb_write error: error->message == 0!";
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
                            debug() << "itdb_shuffle_write error: error->message == 0!";
                        g_error_free (error);
                    }
                    error = 0;
                    ok = false;
                }
            }
            // Kill status bar only once DB is written
            emit endProgressOperation( this );
        }

        if( ok )
            m_dbChanged = false;
        else
            debug() << "Failed to write iPod database";

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
#if 0
void
IpodHandler::deleteTrackListFromDevice( const Meta::TrackList &tracks )
{
    DEBUG_BLOCK

    // Init the list of tracks to be deleted

    m_tracksToDelete = tracks;

    // Set up statusbar for deletion operation

    m_statusbar = The::statusBar()->newProgressOperation( this, i18n( "Deleting Tracks from iPod" ) );

    m_statusbar->setMaximum( tracks.size() );

    connect( this, SIGNAL( incrementProgress() ),
             The::statusBar(), SLOT( incrementProgress() ) );
    connect( this, SIGNAL( endProgressOperation( const QObject*) ),
             The::statusBar(), SLOT( endProgressOperation( const QObject* ) ) );

    deleteNextTrackFromDevice();
}

void
IpodHandler::deleteNextTrackFromDevice()
{
    Meta::TrackPtr track;
    // If there are more tracks to delete, delete the next one
    if( !m_tracksToDelete.isEmpty() )
    {
        // Pop the track off the front of the list

        track = m_tracksToDelete.first();
        m_tracksToDelete.removeFirst();

        // Delete the track

        privateDeleteTrackFromDevice( track );

        emit incrementProgress();
    }
    // No tracks left to delete, emit done
    else
    {
        emit incrementProgress();
        emit deleteTracksDone();
    }
}

void
IpodHandler::privateDeleteTrackFromDevice( const Meta::TrackPtr &track )
{
    Itdb_Track *ipodtrack = Meta::IpodTrackPtr::staticCast(track)->getIpodTrack();

    // delete file
    KUrl url;
    url.setPath( realPath( ipodtrack->ipod_path ) );
    deleteFile( url );

    // remove it from the ipod database, ipod playlists and all

    if ( !removeDBTrack( ipodtrack ) )
        debug() << "Error: failed to remove track from db";

    // remove from titlemap

    m_titlemap.remove( track->name(), track );
}
#endif

/// Finds path to copy track to on Ipod, sets m_copyurl for later
void
IpodHandler::findPathToCopy( const Meta::TrackPtr &track )
{
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
            path = path.section('/', 0, path.indexOf('/')-1);
            parentdir.setPath(path);
        }
        while( !path.isEmpty() && !(path==mountPoint()) && !parentdir.exists() );
        debug() << "trying to create \"" << path << "\"";
        if( !create.mkdir( create.absolutePath() ) )
            break;
    }
    if ( !dir.exists() )
    {
        debug() << "Creating directory failed";
        return;
    }

    debug() << "About to copy from: " << track->playableUrl().path();

    m_copyurl = url;
}

bool
IpodHandler::libCopyTrack( const Meta::TrackPtr &track )
{
    return kioCopyTrack( KUrl::fromPath( track->playableUrl().path() ), m_copyurl );
}

void
IpodHandler::writeDatabase()
{
    ThreadWeaver::Weaver::instance()->enqueue( new DBWorkerThread( this ) );
}

void
IpodHandler::addTrackInDB()
{
    DEBUG_BLOCK

    debug() << "Adding " << QString::fromUtf8( m_libtrack->artist) << " - " << QString::fromUtf8( m_libtrack->title );
    itdb_track_add(m_itdb, m_libtrack, -1);

    // TODO: podcasts NYI
    // if(podcastInfo)
#if 0
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
#endif
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
        itdb_playlist_add_track(mpl, m_libtrack, -1);
    }
}
#if 0
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
        itdb_playlist_remove_track(mpl, track);

    GList *cur = m_itdb->playlists;
    while(cur)
    {
        Itdb_Playlist *pl = (Itdb_Playlist *)cur->data;
        while(itdb_playlist_contains_track(pl, track))
            itdb_playlist_remove_track(pl, track);
        cur = cur->next;
    }

    // also frees track's memory
    itdb_track_remove(track);

    return true;
}
#endif
bool
IpodHandler::kioCopyTrack( const KUrl &src, const KUrl &dst )
{
    DEBUG_BLOCK

    debug() << "Copying from *" << src << "* to *" << dst << "*";

    KIO::CopyJob *job = KIO::copy( src, dst, KIO::HideProgressInfo );
    m_jobcounter++;

    connect( job, SIGNAL( result( KJob * ) ),
             this,  SLOT( fileTransferred( KJob * ) ) );

    return true;
}

void
IpodHandler::fileTransferred( KJob *job )  //SLOT
{
    DEBUG_BLOCK
    QMutexLocker locker(&m_joblocker);

    if ( job->error() )
    {
        m_copyFailed = true;
        debug() << "file transfer failed: " << job->errorText();
    }
    m_wait = false;

    m_jobcounter--;

    // Limit max number of jobs to 10, make sure more tracks left
    // to copy

    if( !m_tracksToCopy.empty() )
    {
        debug() << "Tracks to copy still remain";
        if( m_jobcounter < 10 )
        {
            debug() << "Jobs: " << m_jobcounter;
            copyNextTrackToDevice();
        }
    }
    else
    {
        debug() << "Tracklist empty";
        // Empty copy queue, this is last job
        if( m_jobcounter == 0 )
        {
            emit incrementProgress();
            emit copyTracksDone( !m_copyFailed );
        }
    }
}
#if 0
void
IpodHandler::deleteFile( const KUrl &url )
{
    debug() << "deleting " << url.prettyUrl();

    KIO::DeleteJob *job = KIO::del( url, KIO::HideProgressInfo );
    connect( job, SIGNAL( result( KJob * ) ),
             this,  SLOT( fileDeleted( KJob * ) ) );

    return;
}

void
IpodHandler::fileDeleted( KJob *job )  //SLOT
{
    if(job->error())
    {
        debug() << "file deletion failed: " << job->errorText();
    }

    deleteNextTrackFromDevice();
}
#endif
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
        QString mp = QFile::decodeName( itdb_get_mountpoint(m_itdb) );
        if( realPath.startsWith(mp) )
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
#if 0
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

    m_titlemap.insert( track->name(), TrackPtr::staticCast( track ) );

    track->setIpodTrack( ipodtrack ); // convenience pointer

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
#endif
QString
IpodHandler::libGetTitle() const
{
    return QString::fromUtf8( m_currtrack->title );
}

QString
IpodHandler::libGetAlbum() const
{
    return QString::fromUtf8( m_currtrack->album );
}

QString
IpodHandler::libGetArtist() const
{
    return QString::fromUtf8( m_currtrack->artist );
}

QString
IpodHandler::libGetComposer() const
{
    return QString::fromUtf8( m_currtrack->composer );
}

QString
IpodHandler::libGetGenre() const
{
    return QString::fromUtf8( m_currtrack->genre );
}

int
IpodHandler::libGetYear() const
{
    return m_currtrack->year;
}

int
IpodHandler::libGetLength() const
{
    return ( ( m_currtrack->tracklen ) / 1000 );
}

int
IpodHandler::libGetTrackNumber() const
{
    return m_currtrack->track_nr;
}

QString
IpodHandler::libGetComment() const
{
    return QString::fromUtf8( m_currtrack->comment );
}

int
IpodHandler::libGetDiscNumber() const
{
    return m_currtrack->cd_nr;
}

int
IpodHandler::libGetBitrate() const
{
    return m_currtrack->bitrate;
}

int
IpodHandler::libGetSamplerate() const
{
    return m_currtrack->samplerate;
}

float
IpodHandler::libGetBpm() const
{
    return m_currtrack->BPM;
}
int
IpodHandler::libGetFileSize() const
{
    return m_currtrack->size;
}
int
IpodHandler::libGetPlayCount() const
{
    return m_currtrack->playcount;
}
uint
IpodHandler::libGetLastPlayed() const
{
    return m_currtrack->time_played;
}
int
IpodHandler::libGetRating() const
{
    return ( m_currtrack->rating / ITDB_RATING_STEP * 2 );
}
QString
IpodHandler::libGetType() const
{
    if( QString::fromUtf8( m_currtrack->filetype ) == "mpeg" )
        return "mp3";
}

QString
IpodHandler::libGetPlayableUrl() const
{
    return (m_mountPoint + QString( m_currtrack->ipod_path ).split( ':' ).join( "/" ));
}

/// Sets

void
IpodHandler::libSetTitle( const QString& title )
{
    m_libtrack->title = g_strdup( title.toUtf8() );
}
void
IpodHandler::libSetAlbum( const QString& album )
{
    m_libtrack->album = g_strdup( album.toUtf8() );
}
void
IpodHandler::libSetArtist( const QString& artist )
{
    m_libtrack->artist = g_strdup( artist.toUtf8() );
}
void
IpodHandler::libSetComposer( const QString& composer )
{
    m_libtrack->composer = g_strdup( composer.toUtf8() );
}
void
IpodHandler::libSetGenre( const QString& genre )
{
    if( genre.startsWith("audiobook", Qt::CaseInsensitive) )
    {
        m_libtrack->remember_playback_position |= 0x01;
        m_libtrack->skip_when_shuffling |= 0x01;
        m_libtrack->mediatype = ITDB_MEDIATYPE_AUDIOBOOK;
    }
}
void
IpodHandler::libSetYear( const QString& year )
{
    bool ok;
    int yr = year.toInt( &ok, 10 );
    if( ok )
        m_libtrack->year = yr;
}
void
IpodHandler::libSetLength( int length )
{
    m_libtrack->tracklen = length*1000;
}
void
IpodHandler::libSetTrackNumber( int tracknum )
{
    m_libtrack->track_nr = tracknum;
}
void
IpodHandler::libSetComment( const QString& comment )
{
    m_libtrack->comment = g_strdup( comment.toUtf8() );
}
void
IpodHandler::libSetDiscNumber( int discnum )
{
    m_libtrack->cd_nr = discnum;
}
void
IpodHandler::libSetBitrate( int bitrate )
{
    m_libtrack->bitrate = bitrate;
}
void
IpodHandler::libSetSamplerate( int samplerate )
{
    m_libtrack->samplerate = samplerate;
}
void
IpodHandler::libSetBpm( float bpm )
{
    m_libtrack->BPM = static_cast<int>( bpm );
}
void
IpodHandler::libSetFileSize( int filesize )
{
    m_libtrack->size = filesize;
}
void
IpodHandler::libSetPlayCount( int playcount )
{
    m_libtrack->playcount = playcount;
}
void
IpodHandler::libSetLastPlayed( uint lastplayed)
{
}
void
IpodHandler::libSetRating( int rating )
{
    m_libtrack->rating = ( rating * ITDB_RATING_STEP / 2 );
}
void
IpodHandler::libSetType( const QString& type )
{
    m_libtrack->mediatype = ITDB_MEDIATYPE_AUDIO;
    bool audiobook = false;
    if(type=="wav")
    {
        m_libtrack->filetype = g_strdup( "wav" );
    }
    else if(type=="mp3" || type=="mpeg")
    {
        m_libtrack->filetype = g_strdup( "mpeg" );
    }
    else if(type=="aac" || type=="m4a" || (!m_supportsVideo && type=="mp4"))
    {
        m_libtrack->filetype = g_strdup( "mp4" );
    }
    else if(type=="m4b")
    {
        audiobook = true;
        m_libtrack->filetype = g_strdup( "mp4" );
    }
    else if(type=="m4v" || type=="mp4v" || type=="mov" || type=="mpg" || type=="mp4")
    {
        m_libtrack->filetype = g_strdup( "m4v video" );
        m_libtrack->movie_flag = 0x01; // for videos
        m_libtrack->mediatype = ITDB_MEDIATYPE_MOVIE;
    }
    // TODO: NYI, TagLib calls need to be ported
    /*
    else if(type=="aa")
    {
        audiobook = true;
        m_libtrack->filetype = g_strdup( "audible" );

        TagLib::Audible::File f( QFile::encodeName( url.path() ) );
        TagLib::Audible::Tag *t = f.getAudibleTag();
        if( t )
            m_libtrack->drm_userid = t->userID();
        // libgpod also tries to set those, but this won't work
        m_libtrack->unk126 = 0x01;
        m_libtrack->unk144 = 0x0029;

    }
    */
    else
    {
        m_libtrack->filetype = g_strdup( type.toUtf8() );
    }

    if( audiobook )
    {
        m_libtrack->remember_playback_position |= 0x01;
        m_libtrack->skip_when_shuffling |= 0x01;
        m_libtrack->mediatype = ITDB_MEDIATYPE_AUDIOBOOK;
    }
}

void
IpodHandler::libSetPlayableUrl()
{
    QString pathname = m_copyurl.path();

    QString type = pathname.section('.', -1).toLower();
    type = type.toLower();

    debug() << "Path before put in ipod_path: " << pathname;

    m_libtrack->ipod_path = g_strdup( ipodPath(pathname).toLatin1() );
    debug() << "on iPod: " << m_libtrack->ipod_path;
}

void
IpodHandler::libCreateTrack()
{
    m_libtrack = itdb_track_new();
}

void
IpodHandler::setCopyTrackForParse()
{
    m_currtrack = m_libtrack;
}

/*
QString
IpodHandler::ipodArtFilename( const Itdb_Track *ipodtrack ) const
{
    const QString artist = QString::fromUtf8( ipodtrack->artist );
    const QString album  = QString::fromUtf8( ipodtrack->album  );
    KMD5 context( artist.toLower().toLocal8Bit() + album.toLower().toLocal8Bit() );
    const QString imageKey = context.hexDigest();
    return m_tempdir->name() + imageKey + ".png";
}

// TODO: This is sloooow. Need to implement on-demand fetching.
void
IpodHandler::getCoverArt( const Itdb_Track *ipodtrack )
{
#ifdef GDK_FOUND
    if( !ipodtrack )
        return;

    const QString filename = ipodArtFilename( ipodtrack );

    if( m_coverArt.contains(filename) )
        return;

    if( ipodtrack->has_artwork == 0x02 )
        return;

    GdkPixbuf *pixbuf = (GdkPixbuf*) itdb_artwork_get_pixbuf( ipodtrack->itdb->device, ipodtrack->artwork, -1, -1 );
    if( !pixbuf )
        return;

    gdk_pixbuf_save( pixbuf, QFile::encodeName(filename), "png", NULL, (const char*)(NULL));
    gdk_pixbuf_unref( pixbuf );

    m_coverArt.insert( filename );
#else
    Q_UNUSED(ipodtrack);
#endif
}
*/
/*
QPixmap
IpodHandler::getCover( Meta::MediaDeviceTrackPtr trackk ) const
{
#ifdef GDK_FOUND
    const Itdb_Track *ipodTrack = track->getIpodTrack();
    const QString filename = ipodArtFilename( ipodTrack );
    return QPixmap( filename );
#else
    Q_UNUSED( track );
    return QPixmap();
#endif
}
*/
/*
void
IpodHandler::setCoverArt( Itdb_Track *ipodtrack, const QString &path ) const
{
#ifdef GDK_FOUND
    DEBUG_BLOCK

    if( !m_supportsArtwork )
        return;

    itdb_artwork_remove_thumbnails( ipodtrack->artwork );
    itdb_track_set_thumbnails( ipodtrack, QFile::encodeName(path) );
    ipodtrack->has_artwork = 0x01;
#else
    Q_UNUSED( ipodtrack );
    Q_UNUSED( path );
#endif
}

void
IpodHandler::setCoverArt( Itdb_Track *ipodtrack, const QPixmap &image ) const
{
#ifdef GDK_FOUND
    DEBUG_BLOCK

    if( image.isNull() || !m_supportsArtwork )
        return;

    const QString filename = ipodArtFilename( ipodtrack );
    bool saved = image.save( filename );
    if( !saved )
        return;

    setCoverArt( ipodtrack, filename );
#else
    Q_UNUSED( ipodtrack );
    Q_UNUSED( image );
#endif
}
*/

void
IpodHandler::prepareToParse()
{
    m_currtracklist = m_itdb->tracks;
}

bool
IpodHandler::isEndOfParseList()
{
    return (m_currtracklist ? false : true);
}

void
IpodHandler::prepareToParseNextTrack()
{
    m_currtracklist = m_currtracklist->next;
}

void
IpodHandler::nextTrackToParse()
{
    m_currtrack = (Itdb_Track*) m_currtracklist->data;
}

void
IpodHandler::setAssociateTrack( const Meta::MediaDeviceTrackPtr track )
{
    m_itdbtrackhash[ track ] = m_currtrack;
}

QStringList
IpodHandler::supportedFormats()
{
    QStringList formats;

    formats << "mp3" << "aac" << "mp4";

    return formats;
}


/* Private Functions */

void
IpodHandler::prepareToCopy()
{
    // Initialize job counter to prepare to keep track of how many
    // copy jobs are going on at once
    m_jobcounter = 0;
}

void
IpodHandler::slotDBWriteFailed( ThreadWeaver::Job* job )
{
    Q_UNUSED( job );
    debug() << "Writing to DB failed!";
    emit databaseWritten( false );
}

void
IpodHandler::slotDBWriteSucceeded( ThreadWeaver::Job* job )
{
    Q_UNUSED( job );
    debug() << "Writing to DB succeeded!";
    emit databaseWritten( true );
}

DBWorkerThread::DBWorkerThread( IpodHandler* handler )
    : ThreadWeaver::Job()
    , m_success( false )
    , m_handler( handler )
{
    connect( this, SIGNAL( failed( ThreadWeaver::Job* ) ), m_handler, SLOT( slotDBWriteFailed( ThreadWeaver::Job* ) ) );
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), m_handler, SLOT( slotDBWriteSucceeded( ThreadWeaver::Job* ) ) );
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), this, SLOT( deleteLater() ) );
}

DBWorkerThread::~DBWorkerThread()
{
    //nothing to do
}

bool
DBWorkerThread::success() const
{
    return m_success;
}

void
DBWorkerThread::run()
{
    m_success = m_handler->writeITunesDB( false );
}


#include "IpodHandler.moc"

