/****************************************************************************************
 * Copyright (c) 2005,2006 Martin Aumueller <aumuell@reserv.at>                         *
 * Copyright (c) 2004 Christian Muehlhaeuser <chris@chris.de>                           *
 * Copyright (c) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
 * Copyright (c) 2009 Seb Ruiz <ruiz@kde.org>                                           *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "IpodHandler"

#include "IpodHandler.h"

#include "IpodCollection.h"
#include "Debug.h"

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
#include <KIO/NetAccess>
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

    m_copyingthreadsafe = false;

    g_type_init();
    m_success = false;

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

void
IpodHandler::init()
{
    GError *err = 0;


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

    m_memColl->slotAttemptConnectionDone( m_success );
}

bool
IpodHandler::isWritable() const
{
    // TODO: check if read-only
    return true;
}

QString
IpodHandler::prettyName() const
{
    return QString::fromUtf8( itdb_playlist_mpl( m_itdb )->name );
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
            //emit databaseWritten( this );
        }

        if( ok )
            m_dbChanged = false;
        else
            debug() << "Failed to write iPod database";

        return ok;
    }

    debug() << "writeItunesDB is returning false because db wasn't changed";

    return false;
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

/// Finds path to copy track to on Ipod
void
IpodHandler::findPathToCopy( const Meta::TrackPtr &srcTrack, const Meta::MediaDeviceTrackPtr &destTrack )
{
    Q_UNUSED( destTrack )
        debug() << "Mountpoint is: " << mountPoint();

    KUrl url = determineURLOnDevice(srcTrack);

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

    debug() << "About to copy from: " << srcTrack->playableUrl().path();

    m_trackdesturl[ srcTrack ] = url;
}

bool
IpodHandler::libCopyTrack( const Meta::TrackPtr &srcTrack, Meta::MediaDeviceTrackPtr &destTrack )
{
    Q_UNUSED( destTrack )
    DEBUG_BLOCK
//    findPathToCopy( srcTrack );
    KUrl srcurl = KUrl::fromPath( srcTrack->playableUrl().path() );
    m_trackscopying[ srcurl ] = srcTrack;
    return kioCopyTrack( srcurl, m_trackdesturl[ srcTrack ] );
}

void
IpodHandler::writeDatabase()
{
    ThreadWeaver::Weaver::instance()->enqueue( new DBWorkerThread( this ) );
}

void
IpodHandler::addTrackInDB(const Meta::MediaDeviceTrackPtr& track)
{
    DEBUG_BLOCK

    debug() << "Adding " << QString::fromUtf8( m_itdbtrackhash[ track ]->artist) << " - " << QString::fromUtf8( m_itdbtrackhash[ track ]->title );
    itdb_track_add(m_itdb, m_itdbtrackhash[ track ], -1);

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
        itdb_playlist_add_track(mpl, m_itdbtrackhash[ track ], -1);
    }


}

bool
IpodHandler::libDeleteTrackFile( const Meta::MediaDeviceTrackPtr &track )
{
    DEBUG_BLOCK
    Itdb_Track *ipodtrack = m_itdbtrackhash[ track ];

    // delete file
    KUrl url;
    url.setPath( realPath( ipodtrack->ipod_path ) );
    Meta::TrackPtr trackptr = Meta::TrackPtr::staticCast( track );
    m_tracksdeleting[ url ] = trackptr;
    deleteFile( url );

    return true;

}

void
IpodHandler::libDeleteTrack( const Meta::MediaDeviceTrackPtr &track )
{
    DEBUG_BLOCK
    Itdb_Track *ipodtrack = m_itdbtrackhash[ track ];

    m_itdbtrackhash.remove( track );

    itdb_track_remove( ipodtrack );
}

void
IpodHandler::removeTrackFromDB( const Meta::MediaDeviceTrackPtr &track )
{
    DEBUG_BLOCK
    Itdb_Track *ipodtrack = m_itdbtrackhash[ track ];

    removeDBTrack( ipodtrack );
}

void
IpodHandler::databaseChanged()
{
    m_dbChanged = true;
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
        itdb_playlist_remove_track(mpl, track);

    GList *cur = m_itdb->playlists;
    while(cur)
    {
        Itdb_Playlist *pl = (Itdb_Playlist *)cur->data;
        while(itdb_playlist_contains_track(pl, track))
            itdb_playlist_remove_track(pl, track);
        cur = cur->next;
    }

    return true;
}

bool
IpodHandler::kioCopyTrack( const KUrl &src, const KUrl &dst )
{
    DEBUG_BLOCK

    debug() << "Copying from *" << src << "* to *" << dst << "*";



    KIO::CopyJob *job = KIO::copy( src, dst, KIO::HideProgressInfo );
    m_jobcounter++;

    if( m_jobcounter < 150 )
        copyNextTrackToDevice();


    connect( job, SIGNAL( result( KJob * ) ),
             this,  SLOT( fileTransferred( KJob * ) ), Qt::QueuedConnection );

    connect( job, SIGNAL( copyingDone(KIO::Job*,KUrl,KUrl,time_t,bool,bool)),
             this, SLOT(slotCopyingDone(KIO::Job*,KUrl,KUrl,time_t,bool,bool)) );

    //return KIO::NetAccess::file_copy( src, dst );

    return true;
}

void
IpodHandler::fileTransferred( KJob *job )  //SLOT
{
    DEBUG_BLOCK
    QMutexLocker locker(&m_joblocker);

    m_wait = false;

    m_jobcounter--;

    if ( job->error() )
    {
        m_copyFailed = true;
        debug() << "file transfer failed: " << job->errorText();
        return;
    }




    // Limit max number of jobs to 150, make sure more tracks left
    // to copy


        debug() << "Tracks to copy still remain";
        if( m_jobcounter < 150 )
        {
            debug() << "Jobs: " << m_jobcounter;
            copyNextTrackToDevice();
        }


    /*
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
    */
}

void
IpodHandler::slotCopyingDone( KIO::Job* job, KUrl from, KUrl to, time_t mtime, bool directory, bool renamed)
{
    Q_UNUSED( job )
    Q_UNUSED( to )
    Q_UNUSED( mtime )
    Q_UNUSED( directory )
    Q_UNUSED( renamed )

    DEBUG_BLOCK
    Meta::TrackPtr track = m_trackscopying[from];

    if( job->error() )
    {
        emit libCopyTrackFailed( track );
    }
    else
    {
        slotFinalizeTrackCopy( track );
    }

}

void
IpodHandler::deleteFile( const KUrl &url )
{
    DEBUG_BLOCK
    debug() << "deleting " << url.prettyUrl();

    KIO::DeleteJob *job = KIO::del( url, KIO::HideProgressInfo );

    m_jobcounter++;

    if( m_jobcounter < 150 )
        removeNextTrackFromDevice();

    connect( job, SIGNAL( result( KJob * ) ),
             this,  SLOT( fileDeleted( KJob * ) ) );

    return;
}

void
IpodHandler::fileDeleted( KJob *job )  //SLOT
{
    DEBUG_BLOCK
    if(job->error())
    {
        debug() << "file deletion failed: " << job->errorText();
    }

    m_jobcounter--;

    // Limit max number of jobs to 150, make sure more tracks left
    // to delete


    debug() << "Tracks to delete still remain";
        if( m_jobcounter < 150 )
        {
            debug() << "Jobs: " << m_jobcounter;
            removeNextTrackFromDevice();
        }

    KIO::DeleteJob *djob = reinterpret_cast<KIO::DeleteJob*> (job);

    if( djob )
    {
        KUrl url = djob->urls().first();

        Meta::TrackPtr track = m_tracksdeleting[ url ];

        debug() << "emitting libRemoveTrackDone";

        slotFinalizeTrackRemove( track );
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


QString
IpodHandler::libGetTitle( const Meta::MediaDeviceTrackPtr &track )
{
    return QString::fromUtf8( m_itdbtrackhash[ track ]->title );
}

QString
IpodHandler::libGetAlbum( const Meta::MediaDeviceTrackPtr &track )
{
    return QString::fromUtf8( m_itdbtrackhash[ track ]->album );
}

QString
IpodHandler::libGetArtist( const Meta::MediaDeviceTrackPtr &track )
{
    return QString::fromUtf8( m_itdbtrackhash[ track ]->artist );
}

QString
IpodHandler::libGetComposer( const Meta::MediaDeviceTrackPtr &track )
{
    return QString::fromUtf8( m_itdbtrackhash[ track ]->composer );
}

QString
IpodHandler::libGetGenre( const Meta::MediaDeviceTrackPtr &track )
{
    return QString::fromUtf8( m_itdbtrackhash[ track ]->genre );
}

int
IpodHandler::libGetYear( const Meta::MediaDeviceTrackPtr &track )
{
    return m_itdbtrackhash[ track ]->year;
}

int
IpodHandler::libGetLength( const Meta::MediaDeviceTrackPtr &track )
{
    return ( ( m_itdbtrackhash[ track ]->tracklen ) / 1000 );
}

int
IpodHandler::libGetTrackNumber( const Meta::MediaDeviceTrackPtr &track )
{
    return m_itdbtrackhash[ track ]->track_nr;
}

QString
IpodHandler::libGetComment( const Meta::MediaDeviceTrackPtr &track )
{
    return QString::fromUtf8( m_itdbtrackhash[ track ]->comment );
}

int
IpodHandler::libGetDiscNumber( const Meta::MediaDeviceTrackPtr &track )
{
    return m_itdbtrackhash[ track ]->cd_nr;
}

int
IpodHandler::libGetBitrate( const Meta::MediaDeviceTrackPtr &track )
{
    return m_itdbtrackhash[ track ]->bitrate;
}

int
IpodHandler::libGetSamplerate( const Meta::MediaDeviceTrackPtr &track )
{
    return m_itdbtrackhash[ track ]->samplerate;
}

float
IpodHandler::libGetBpm( const Meta::MediaDeviceTrackPtr &track )
{
    return m_itdbtrackhash[ track ]->BPM;
}
int
IpodHandler::libGetFileSize( const Meta::MediaDeviceTrackPtr &track )
{
    return m_itdbtrackhash[ track ]->size;
}
int
IpodHandler::libGetPlayCount( const Meta::MediaDeviceTrackPtr &track )
{
    return m_itdbtrackhash[ track ]->playcount;
}
uint
IpodHandler::libGetLastPlayed( const Meta::MediaDeviceTrackPtr &track )
{
    return m_itdbtrackhash[ track ]->time_played;
}
int
IpodHandler::libGetRating( const Meta::MediaDeviceTrackPtr &track )
{
    return ( m_itdbtrackhash[ track ]->rating / ITDB_RATING_STEP * 2 );
}
QString
IpodHandler::libGetType( const Meta::MediaDeviceTrackPtr &track )
{
    if( QString::fromUtf8( m_itdbtrackhash[ track ]->filetype ) == "mpeg" )
        return "mp3";

    return QString::fromUtf8( m_itdbtrackhash[ track ]->filetype );
}

QString
IpodHandler::libGetPlayableUrl( const Meta::MediaDeviceTrackPtr &track )
{
    return (m_mountPoint + QString( m_itdbtrackhash[ track ]->ipod_path ).split( ':' ).join( "/" ));
}

/// Sets

void
IpodHandler::libSetTitle( Meta::MediaDeviceTrackPtr& track, const QString& title )
{
    m_itdbtrackhash[ track ]->title = g_strdup( title.toUtf8() );
}
void
IpodHandler::libSetAlbum( Meta::MediaDeviceTrackPtr &track, const QString& album )
{
    m_itdbtrackhash[ track ]->album = g_strdup( album.toUtf8() );
}
void
IpodHandler::libSetArtist( Meta::MediaDeviceTrackPtr &track, const QString& artist )
{
    m_itdbtrackhash[ track ]->artist = g_strdup( artist.toUtf8() );
}
void
IpodHandler::libSetComposer( Meta::MediaDeviceTrackPtr &track, const QString& composer )
{
    m_itdbtrackhash[ track ]->composer = g_strdup( composer.toUtf8() );
}
void
IpodHandler::libSetGenre( Meta::MediaDeviceTrackPtr &track, const QString& genre )
{
    if( genre.startsWith("audiobook", Qt::CaseInsensitive) )
    {
        m_itdbtrackhash[ track ]->remember_playback_position |= 0x01;
        m_itdbtrackhash[ track ]->skip_when_shuffling |= 0x01;
        m_itdbtrackhash[ track ]->mediatype = ITDB_MEDIATYPE_AUDIOBOOK;
    }

    m_itdbtrackhash[ track ]->genre = g_strdup( genre.toUtf8() );
}
void
IpodHandler::libSetYear( Meta::MediaDeviceTrackPtr &track, const QString& year )
{
    bool ok;
    int yr = year.toInt( &ok, 10 );
    if( ok )
        m_itdbtrackhash[ track ]->year = yr;
}
void
IpodHandler::libSetLength( Meta::MediaDeviceTrackPtr &track, int length )
{
    m_itdbtrackhash[ track ]->tracklen = length*1000;
}
void
IpodHandler::libSetTrackNumber( Meta::MediaDeviceTrackPtr &track, int tracknum )
{
    m_itdbtrackhash[ track ]->track_nr = tracknum;
}
void
IpodHandler::libSetComment( Meta::MediaDeviceTrackPtr &track, const QString& comment )
{
    m_itdbtrackhash[ track ]->comment = g_strdup( comment.toUtf8() );
}
void
IpodHandler::libSetDiscNumber( Meta::MediaDeviceTrackPtr &track, int discnum )
{
    m_itdbtrackhash[ track ]->cd_nr = discnum;
}
void
IpodHandler::libSetBitrate( Meta::MediaDeviceTrackPtr &track, int bitrate )
{
    m_itdbtrackhash[ track ]->bitrate = bitrate;
}
void
IpodHandler::libSetSamplerate( Meta::MediaDeviceTrackPtr &track, int samplerate )
{
    m_itdbtrackhash[ track ]->samplerate = samplerate;
}
void
IpodHandler::libSetBpm( Meta::MediaDeviceTrackPtr &track, float bpm )
{
    m_itdbtrackhash[ track ]->BPM = static_cast<int>( bpm );
}
void
IpodHandler::libSetFileSize( Meta::MediaDeviceTrackPtr &track, int filesize )
{
    m_itdbtrackhash[ track ]->size = filesize;
}
void
IpodHandler::libSetPlayCount( Meta::MediaDeviceTrackPtr &track, int playcount )
{
    m_itdbtrackhash[ track ]->playcount = playcount;
}
void
IpodHandler::libSetLastPlayed( Meta::MediaDeviceTrackPtr &track, uint lastplayed)
{
    Q_UNUSED( track )
    Q_UNUSED( lastplayed )
}
void
IpodHandler::libSetRating( Meta::MediaDeviceTrackPtr &track, int rating )
{
    m_itdbtrackhash[ track ]->rating = ( rating * ITDB_RATING_STEP / 2 );
}
void
IpodHandler::libSetType( Meta::MediaDeviceTrackPtr &track, const QString& type )
{
    m_itdbtrackhash[ track ]->mediatype = ITDB_MEDIATYPE_AUDIO;
    bool audiobook = false;
    if(type=="wav")
    {
        m_itdbtrackhash[ track ]->filetype = g_strdup( "wav" );
    }
    else if(type=="mp3" || type=="mpeg")
    {
        m_itdbtrackhash[ track ]->filetype = g_strdup( "mpeg" );
    }
    else if(type=="aac" || type=="m4a" || (!m_supportsVideo && type=="mp4"))
    {
        m_itdbtrackhash[ track ]->filetype = g_strdup( "mp4" );
    }
    else if(type=="m4b")
    {
        audiobook = true;
        m_itdbtrackhash[ track ]->filetype = g_strdup( "mp4" );
    }
    else if(type=="m4v" || type=="mp4v" || type=="mov" || type=="mpg" || type=="mp4")
    {
        m_itdbtrackhash[ track ]->filetype = g_strdup( "m4v video" );
        m_itdbtrackhash[ track ]->movie_flag = 0x01; // for videos
        m_itdbtrackhash[ track ]->mediatype = ITDB_MEDIATYPE_MOVIE;
    }
    // TODO: NYI, TagLib calls need to be ported
    /*
    else if(type=="aa")
    {
        audiobook = true;
        m_itdbtrackhash[ track ]->filetype = g_strdup( "audible" );

        TagLib::Audible::File f( QFile::encodeName( url.path() ) );
        TagLib::Audible::Tag *t = f.getAudibleTag();
        if( t )
            m_itdbtrackhash[ track ]->drm_userid = t->userID();
        // libgpod also tries to set those, but this won't work
        m_itdbtrackhash[ track ]->unk126 = 0x01;
        m_itdbtrackhash[ track ]->unk144 = 0x0029;

    }
    */
    else
    {
        m_itdbtrackhash[ track ]->filetype = g_strdup( type.toUtf8() );
    }

    if( audiobook )
    {
        m_itdbtrackhash[ track ]->remember_playback_position |= 0x01;
        m_itdbtrackhash[ track ]->skip_when_shuffling |= 0x01;
        m_itdbtrackhash[ track ]->mediatype = ITDB_MEDIATYPE_AUDIOBOOK;
    }
}

void
IpodHandler::libSetPlayableUrl( Meta::MediaDeviceTrackPtr &destTrack, const Meta::TrackPtr &srcTrack )
{
    KUrl copyurl = m_trackdesturl[ srcTrack ];
    QString pathname = copyurl.path();

    QString type = pathname.section('.', -1).toLower();
    type = type.toLower();

    debug() << "Path before put in ipod_path: " << pathname;

    m_itdbtrackhash[ destTrack ]->ipod_path = g_strdup( ipodPath(pathname).toLatin1() );
    debug() << "on iPod: " << m_itdbtrackhash[ destTrack ]->ipod_path;
}

void
IpodHandler::libCreateTrack( const Meta::MediaDeviceTrackPtr& track )
{
    m_itdbtrackhash[ track ] = itdb_track_new();
}

#if 0
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


QPixmap
IpodHandler::libGetCoverArt( Meta::MediaDeviceTrackPtr track ) const
{
#ifdef GDK_FOUND

    getCoverArt( m_itdbtrackhash[ track ];
    return QPixmap( filename );
#else
    Q_UNUSED( track );
    return QPixmap();
#endif
}


void
IpodHandler::setCoverArt( Itdb_Track *ipodtrack, const QString &path )
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
IpodHandler::libSetCoverArt( Itdb_Track *ipodtrack, const QPixmap &image )
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
#endif

void
IpodHandler::prepareToParseTracks()
{
    m_currtracklist = m_itdb->tracks;
}

bool
IpodHandler::isEndOfParseTracksList()
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

/// Playlist Parsing

void
IpodHandler::prepareToParsePlaylists()
{
    m_currplaylistlist = m_itdb->playlists;
}


bool
IpodHandler::isEndOfParsePlaylistsList()
{
    return (m_currplaylistlist ? false : true);
}


void
IpodHandler::prepareToParseNextPlaylist()
{
    m_currplaylistlist = m_currplaylistlist->next;
}


void
IpodHandler::nextPlaylistToParse()
{
    m_currplaylist = ( Itdb_Playlist * ) m_currplaylistlist->data;
}

bool
IpodHandler::shouldNotParseNextPlaylist()
{
    // NOTE: skip the master playlist
    return ( itdb_playlist_is_mpl( m_currplaylist ) );
}


void
IpodHandler::prepareToParsePlaylistTracks()
{
    m_currtracklist = m_currplaylist->members;
}


bool
IpodHandler::isEndOfParsePlaylist()
{
    return (m_currtracklist ? false : true );
}


void
IpodHandler::prepareToParseNextPlaylistTrack()
{
    prepareToParseNextTrack();
}


void
IpodHandler::nextPlaylistTrackToParse()
{
    nextTrackToParse();
}

Meta::MediaDeviceTrackPtr
IpodHandler::libGetTrackPtrForTrackStruct()
{
    return m_itdbtrackhash.key( m_currtrack );
}

QString
IpodHandler::libGetPlaylistName()
{
    return QString::fromUtf8( m_currplaylist->name );
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
    //m_jobcounter = 0;

    m_trackdesturl.clear();
    m_trackscopying.clear();
}

void
IpodHandler::prepareToDelete()
{
    // Initialize job counter to prepare to keep track of how many
    // copy jobs are going on at once
    //m_jobcounter = 0;

    m_tracksdeleting.clear();
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
    if( job->success() )
    {
        debug() << "Writing to DB succeeded!";
        emit databaseWritten( true );
    }
    else
        debug() << "Writing to DB did not happen or failed";
}

DBWorkerThread::DBWorkerThread( IpodHandler* handler )
    : ThreadWeaver::Job()
    , m_success( false )
    , m_handler( handler )
{
    connect( this, SIGNAL( failed( ThreadWeaver::Job* ) ), m_handler, SLOT( slotDBWriteFailed( ThreadWeaver::Job* ) ), Qt::QueuedConnection );
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), m_handler, SLOT( slotDBWriteSucceeded( ThreadWeaver::Job* ) ), Qt::QueuedConnection );
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), this, SLOT( deleteLater() ), Qt::QueuedConnection );
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

