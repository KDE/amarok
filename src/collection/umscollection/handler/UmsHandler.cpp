/****************************************************************************************
 * Copyright (c) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "UmsHandler"

#include "UmsHandler.h"

#include "UmsCollection.h"
#include "UmsPodcastProvider.h"
#include "core/support/Debug.h"
#include "playlistmanager/PlaylistManager.h"

#include "SvgHandler.h"

#include "core-implementations/meta/file/File.h" // for KIO file handling

#include <KCodecs> // KMD5
#include <KDirLister>
#include <kdiskfreespaceinfo.h>
#include <KIO/Job>
#include <KIO/CopyJob>
#include <KIO/DeleteJob>
#include <KIO/Scheduler>
#include <KIO/NetAccess>
#include <kinputdialog.h>
#include "kjob.h"
#include <KMessageBox>
#include <kmimetype.h>
#include <KPasswordDialog>
#include <KStandardDirs>
#include <KUrl>
#include <threadweaver/ThreadWeaver.h>

#include <solid/device.h>
#include <solid/storageaccess.h>

#include <QAction>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QMutex>
#include <QMutexLocker>
#include <QPixmap>
#include <QProcess>
#include <QRegExp>
#include <QString>
#include <QStringList>
#include <QTime>
#include <QWaitCondition>

using namespace Meta;

/// UmsHandler

// Define the maximum number of concurrent kio jobs allowed.
// This used to be 150, but flash media doesn't handle parallel
// writes well, so by forcing this constant to 1 the jobs
// are run sequentially.
// BUG: 218152
#ifndef UMS_MAX_CONCURRENT_JOBS
#define UMS_MAX_CONCURRENT_JOBS 1
#endif

QString UmsHandler::s_settingsFileName( ".is_audio_player" );
QString UmsHandler::s_audioFolderKey( "audio_folder" );
QString UmsHandler::s_podcastFolderKey( "podcast_folder" );
QString UmsHandler::s_autoConnectKey( "use_automatically" );

UmsHandler::UmsHandler( Collections::UmsCollection *mc, const QString& mountPoint )
    : MediaDeviceHandler( mc )
    , m_watcher()
    , m_listpos( 0 )
    , m_dirs( 0 )
    , m_timer()
    , m_dirtytimer()
    , m_filepath()
    , m_capacity( 0.0 )
    , m_jobcounter( 0 )
    , m_autoConnect( false )
    , m_mountPoint( mountPoint )
    , m_wasMounted( !mountPoint.isEmpty() )
    , m_name()
    , m_parsed( false )
    , m_parseAction( 0 )
    , m_dbChanged( false )
    , m_copyFailed( false )
    , m_isCanceled( false )
    , m_wait( false )
    , m_tempdir( new KTempDir() )
    , m_podcastProvider( 0 )
    , m_configureAction( 0 )
    , m_settings( 0 )
    , m_umsSettingsDialog( 0 )
{
    DEBUG_BLOCK

    m_copyingthreadsafe = false;
    m_success = false;

}

UmsHandler::~UmsHandler()
{
    DEBUG_BLOCK
    delete m_tempdir;
    if( m_podcastProvider )
    {
        The::playlistManager()->removeProvider( m_podcastProvider );
        delete m_podcastProvider;
    }
}

void
UmsHandler::init()
{
    DEBUG_BLOCK
    if( m_mountPoint.isEmpty() )
    {
        debug() << "Empty mountpoint, aborting";
        m_memColl->slotAttemptConnectionDone( false );
        return;
    }

    KUrl playerFilePath( m_mountPoint );
    playerFilePath.addPath( s_settingsFileName );
    QFile playerFile( playerFilePath.toLocalFile() );

    if (playerFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        debug() << QString( "Got %1 file").arg( s_settingsFileName );
        QTextStream in(&playerFile);
        while( !in.atEnd() )
        {
            QString line = in.readLine();
            if( line.startsWith( s_audioFolderKey + "=" ) )
            {
                debug() << QString( "Found %1" ).arg( s_audioFolderKey );
                debug() << line;
                m_musicPath = KUrl( m_mountPoint );
                m_musicPath.addPath( line.section( '=', 1, 1 ) );
                m_musicPath.cleanPath();
                debug() << "Scan for music in " << m_musicPath.toLocalFile();
                if( !QDir( m_musicPath.toLocalFile() ).exists() )
                {
                    debug() << "Music path doesn't exist! Using the mountpoint instead";
                    //TODO: add user-visible warning after string freeze.
                    m_musicPath = m_mountPoint;
                }
            }
            else if( line.startsWith( s_podcastFolderKey + "=" ) )
            {
                debug() << QString( "Found %1, initializing UMS podcast provider" )
                                .arg( s_podcastFolderKey );
                debug() << line;
                m_podcastPath = KUrl( m_mountPoint );
                m_podcastPath.addPath( line.section( '=', 1, 1 ) );
                m_podcastPath.cleanPath();
                debug() << "scan for podcasts in " <<
                        m_podcastPath.toLocalFile( KUrl::AddTrailingSlash );
                //HACK initialize a real PodcastProvider since I failed to add it to the MD framework
                m_podcastProvider = new Podcasts::UmsPodcastProvider( this, m_podcastPath );
                The::playlistManager()->addProvider( m_podcastProvider,
                                                     Playlists::PodcastChannelPlaylist );
            }
            else if( line.startsWith( s_autoConnectKey + "=" ) )
            {
                debug() << "Use automatically: " << line.section( '=', 1, 1 );
                m_autoConnect = ( line.section( '=', 1, 1 ) == "true" );
            }
        }

    }
/*
    m_formats << "mp3" << "wav" << "asf" << "flac" << "wma" << "ogg" << "aac" << "m4a" << "m4b"
            << "mp4" << "mp2" << "ac3";

*/
    m_mimetypes << "audio/mpeg" << "audio/x-wav" << "video/x-ms-asf" << "audio/x-flac"
            << "audio/x-ms-wma" << "application/ogg" << "audio/mp4" << "audio/mp4a-latm"
            << "video/mp4" << "audio/ac3";


    m_timer.setSingleShot( true );
    //m_dirtytimer.setSingleShot( true );

    connect( &m_timer, SIGNAL( timeout() ),
             m_memColl, SLOT( collectionUpdated() ) );
    connect( &m_dirtytimer, SIGNAL( timeout() ),
             this, SLOT( slotCheckDirty() ) );

    connect( &m_watcher, SIGNAL( created( const QString & ) ),
             this, SLOT(slotCreateEntry( const QString& ) ), Qt::QueuedConnection );
    connect( &m_watcher, SIGNAL( dirty( const QString & ) ),
             this, SLOT(slotDirtyEntry( const QString&) ), Qt::QueuedConnection );
    connect( &m_watcher, SIGNAL( deleted( const QString & ) ),
             this, SLOT(slotDeleteEntry( const QString& ) ), Qt::QueuedConnection );

    m_parsed = false;
    m_parseAction = 0;

    // Get storage access for getting device space capacity/usage
    Solid::Device device = Solid::Device(  m_memColl->udi() );
    if(  device.isValid() )
    {
        Solid::StorageAccess *storage = device.as<Solid::StorageAccess>();
        if( storage )
            m_filepath = storage->filePath();
        else if( !m_mountPoint.isEmpty() )
            m_filepath = m_mountPoint;

        if ( !m_filepath.isEmpty() )
            m_capacity = KDiskFreeSpaceInfo::freeSpaceInfo( m_filepath ).size();
        else
        {
            debug() << "capacity = 0.0 because m_filepath.isEmpty()";
            m_capacity = 0.0;
        }
    }
    else
    {
        debug() << "device is not valid";
        m_filepath = "";
        m_capacity = 0.0;
    }

    debug() << "Succeeded: true";
    m_memColl->emitCollectionReady();
    //m_memColl->slotAttemptConnectionDone( true );
    if( m_autoConnect )
    {
        debug() << "Automatically start to parse for tracks";
        m_parsed = true;
        parseTracks();
    }
}

void
UmsHandler::slotCreateEntry( const QString &path )
{
    DEBUG_BLOCK

    // Check if it's a file, and if not, abort

    if( addPath( path ) != 2 )
        return;

    debug() << "adding to dirty list";
    m_dirtylist << path;

    if( !m_dirtytimer.isActive() )
    {
        debug() << "timer inactive, starting...";
        m_dirtytimer.start( 5000 );
    }
    else
    {
        m_dirtytimer.stop();
        m_dirtytimer.start( 5000 );
    }
}

void
UmsHandler::slotCheckDirty()
{
    m_dirtytimer.stop();

    if( m_dirtylist.isEmpty() )
    {
        return;
    }

    foreach( const QString &path, m_dirtylist )
    {
        // Skip dupes.  Can happen when new file is
        // being added outside of Amarok, and
        // device is being parsed at the same time
        if( m_files.contains( path ) )
            continue;
        // Create track based on URL

        //QString path = m_dirtylist.takeFirst();

        Meta::TrackPtr srcTrack( new MetaFile::Track( path ) );

        m_currtrack = srcTrack;

        // Create new track

        Meta::MediaDeviceTrackPtr destTrack ( new Meta::MediaDeviceTrack( m_memColl ) );

        // associate it to track

        setAssociateTrack( destTrack );

        // Fill the track struct of the destTrack with info from the filetrack as source

        getBasicMediaDeviceTrackInfo( srcTrack, destTrack );

        // Add the new Meta::MediaDeviceTrackPtr into the device collection

        // add track to collection
        addMediaDeviceTrackToCollection( destTrack );

    }

    m_dirtylist.clear();

    // only send collection updates every 5 seconds, to avoid constant refresh
    // on each new entry

    if( !m_timer.isActive() )
    {
        m_timer.start( 5000 );
    }
}

void
UmsHandler::slotDirtyEntry( const QString &path )
{
    Q_UNUSED( path )
    if( !m_dirtytimer.isActive() )
    {
        debug() << "timer inactive, starting...";
        m_dirtytimer.start( 5000 );
    }
    else
    {
        m_dirtytimer.stop();
        m_dirtytimer.start( 5000 );
    }
}

void
UmsHandler::slotDeleteEntry( const QString &path )
{
    DEBUG_BLOCK

    if( !m_files.contains( path ) )
        return;

    Meta::MediaDeviceTrackPtr devicetrack = m_files.value( path );

    Meta::TrackPtr track = Meta::TrackPtr::staticCast( devicetrack );

    // remove from titlemap

    m_titlemap.remove( track->name(), track );

    // remove from collection

    removeMediaDeviceTrackFromCollection( devicetrack );

    m_files.remove( path );
    m_currtracklist.removeOne( path );

    // only send collection updates every 5 seconds, to avoid constant refresh
    // on each new entry

    if( !m_timer.isActive() )
    {
        m_timer.start( 5000 );
    }
}

int
UmsHandler::addPath( const QString &path )
{
    DEBUG_BLOCK
    int acc = 0;
    KMimeType::Ptr mime = KMimeType::findByFileContent( path, &acc );
    if( !mime || mime->name() == KMimeType::defaultMimeType() )
    {
        debug() << "Trying again with findByPath:" ;
        mime = KMimeType::findByPath( path, 0, true, &acc );
        if( mime->name() == KMimeType::defaultMimeType() )
            return 0;
    }
    debug() << "Got type: " << mime->name() << "For file: " << path << ", with accuracy: " << acc;

    QFileInfo info( path );
    if( info.isDir() )
    {
        if( m_dirList.contains( path ) )
            return 0;
        m_dirList << info.canonicalPath();
        return 1;
    }
    else if( info.isFile() )
    {
        if( m_currtracklist.contains( path ) )
            return 0;

        foreach( const QString &mimetype, m_mimetypes )
        {
            if( mime->is( mimetype ) )
            {
                m_currtracklist << info.canonicalFilePath();
                return 2;
            }
        }
        /*
        foreach( QString format, m_formats )
            if( info.suffix() == format )
            {
                m_currtracklist << info.canonicalFilePath();
                return 2;
            }
            */
    }

    return 0;
}

QString
UmsHandler::baseMusicFolder() const
{
    return m_musicPath.isEmpty() ? m_mountPoint : m_musicPath.toLocalFile();
}

bool
UmsHandler::isWritable() const
{
    // TODO: check if writable
    return true;
}

bool
UmsHandler::isOrganizable() const
{
    return true;
}

QString
UmsHandler::prettyName() const
{
    Solid::Device device;

    device = Solid::Device( m_memColl->udi() );

    if( device.isValid() )
    {
        QString name = device.vendor().simplified();
        if( !name.isEmpty() )
            name += " ";
        name += device.product().simplified();
        return name;
    }

    return m_mountPoint;
}

QList<QAction *>
UmsHandler::collectionActions()
{
    QList< QAction* > actions;

    // Button to start parse

    if( !m_parsed )
    {
        if( !m_parseAction )
        {
            m_parseAction = new QAction( KIcon( "checkbox" ), i18n(  "&Use as Collection" ), this );
            m_parseAction->setProperty( "popupdropper_svg_id", "edit" );

            connect( m_parseAction, SIGNAL( triggered() ), this, SLOT( parseTracks() ) );
        }

        //if already parsed no need to add this.
        if( !m_parsed )
            actions.append( m_parseAction );
    }
    
    if( !m_configureAction )
    {
        m_configureAction = new QAction( KIcon( "configure" ),
            i18n( "&Configure %1", prettyName() ),
            this
        );
        m_configureAction->setProperty( "popupdropper_svg_id", "configure" );
        connect( m_configureAction, SIGNAL( triggered() ), SLOT( slotConfigure() ) );
    }
    actions << m_configureAction;

    return actions;
}

void
UmsHandler::slotConfigure()
{
    DEBUG_BLOCK
    m_umsSettingsDialog = new KDialog( The::mainWindow() );
    QWidget *settingsWidget = new QWidget( m_umsSettingsDialog );

    m_settings = new Ui::UmsConfiguration();
    m_settings->setupUi( settingsWidget );

    m_settings->m_autoConnect->setChecked( m_autoConnect );

    m_settings->m_musicFolder->setMode( KFile::Directory );
    m_settings->m_musicFolder->setUrl( m_musicPath.isEmpty() ? KUrl( m_mountPoint ) : m_musicPath );

    m_settings->m_podcastFolder->setMode( KFile::Directory );
    m_settings->m_podcastFolder->setUrl( m_podcastPath.isEmpty() ? KUrl( m_mountPoint )
                                         : m_podcastPath );

    m_umsSettingsDialog->setButtons( KDialog::Ok | KDialog::Cancel | KDialog::Apply );
    m_umsSettingsDialog->setMainWidget( settingsWidget );

    m_umsSettingsDialog->setWindowTitle( i18n( "Configure USB Mass Storage Device" ) );
    m_umsSettingsDialog->enableButtonApply( false );

    connect( m_settings->m_musicFolder, SIGNAL( textChanged(QString) ), SLOT( slotConfigChanged() ) );
    connect( m_settings->m_podcastFolder, SIGNAL( textChanged(QString) ), SLOT( slotConfigChanged() ) );
    connect( m_settings->m_autoConnect, SIGNAL( stateChanged( int ) ), SLOT( slotConfigChanged() ) );

    if( m_umsSettingsDialog->exec() == QDialog::Accepted )
    {
        debug() << "accepted";

        if(  m_settings->m_musicFolder->url() != m_musicPath )
        {
            debug() << "music location changed from " << m_musicPath.toLocalFile() << " to ";
            debug() << m_settings->m_musicFolder->url().toLocalFile();
            //TODO: reparse music
        }

        if( m_settings->m_podcastFolder->url() != m_podcastPath )
        {
            debug() << "podcast location changed from " << m_podcastPath << " to ";
            debug() << m_settings->m_podcastFolder->url().url();
            //TODO: reparse podcasts
        }

        m_autoConnect = m_settings->m_autoConnect->isChecked();
        if( m_autoConnect && !m_parsed )
            parseTracks();

        //write the date to the on-disk file
        KUrl localFile = KUrl( m_mountPoint );
        localFile.addPath( s_settingsFileName );
        QFile settingsFile( localFile.toLocalFile() );
        if( settingsFile.open( QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text ) )
        {
            QTextStream s( &settingsFile );
            QString keyValuePair( "%1=%2\n" );
            KUrl musicPath = m_settings->m_musicFolder->url();
            if( musicPath != m_mountPoint )
            {
                s << keyValuePair.arg( s_audioFolderKey, KUrl::relativePath( m_mountPoint,
                    musicPath.toLocalFile() ) );
            }
            KUrl podcastPath = m_settings->m_podcastFolder->url();
            if( podcastPath != m_mountPoint )
            {
                s << keyValuePair.arg( s_podcastFolderKey, KUrl::relativePath( m_mountPoint,
                    podcastPath.toLocalFile() ) );
            }
            if( m_autoConnect )
                s << keyValuePair.arg( s_autoConnectKey, "true" );

            settingsFile.close();
        }
        else
            error() << "Could not open settingsfile " << localFile.toLocalFile();
    }

    delete m_umsSettingsDialog;
    m_umsSettingsDialog = 0;
    delete m_settings;
    m_settings = 0;
}

void
UmsHandler::slotConfigChanged()
{
    if( !m_settings )
        return;

    if( m_settings->m_autoConnect->isChecked() != m_autoConnect
        || m_settings->m_musicFolder->url() != m_musicPath
        || m_settings->m_podcastFolder->url() != m_podcastPath )
    {
        m_umsSettingsDialog->enableButtonApply( true );
    }
}

void
UmsHandler::findPathToCopy( const Meta::TrackPtr &srcTrack, const Meta::MediaDeviceTrackPtr &destTrack )
{
    Q_UNUSED( destTrack )
        debug() << "Mountpoint is: " << mountPoint();

    KUrl url = KUrl( m_destinations.value( srcTrack ) );

    debug() << "Url's path is: " << url.path();

    QFileInfo finfo( url.path() );
    QDir dir = finfo.dir();
    QDir root( QDir::rootPath() );
    // Check if directory exists
    if ( !dir.exists() )
    {
        // If it doesn't exist, make it and the path to it
        if ( !root.mkpath( dir.absolutePath() ) )
        {
            debug() << "Creating directory failed";
            url = "";
        }
        // If fails to create, set its url to blank so the copying will fail
        else
            debug() << "Directory created!";
    }

    debug() << "About to copy from: " << srcTrack->playableUrl().path();
    debug() << "to: " << url;

    m_trackdesturl[ srcTrack ] = url;
}

bool
UmsHandler::libCopyTrack( const Meta::TrackPtr &srcTrack, Meta::MediaDeviceTrackPtr &destTrack )
{
    Q_UNUSED( destTrack )
    DEBUG_BLOCK

    KUrl srcurl = KUrl::fromPath( srcTrack->playableUrl().path() );
    m_trackscopying[ srcurl ] = srcTrack;
    m_srctodest.insert( srcTrack, destTrack );
    return kioCopyTrack( srcurl, m_trackdesturl[ srcTrack ] );
}

bool
UmsHandler::libDeleteTrackFile( const Meta::MediaDeviceTrackPtr &track )
{
    DEBUG_BLOCK

    Meta::TrackPtr metafiletrack = m_umstrackhash.value( track );

    // delete file
    KUrl url = metafiletrack->playableUrl().path();
    Meta::TrackPtr trackptr = Meta::TrackPtr::staticCast( track );
    m_tracksdeleting[ url ] = trackptr;
    deleteFile( url );

    return true;
}

bool
UmsHandler::kioCopyTrack( const KUrl &src, const KUrl &dst )
{
    DEBUG_BLOCK

    debug() << "Copying from *" << src << "* to *" << dst << "*";



    KIO::CopyJob *job = KIO::copy( src, dst, KIO::HideProgressInfo );
    m_jobcounter++;

    if( m_jobcounter < UMS_MAX_CONCURRENT_JOBS )
        copyNextTrackToDevice();


    connect( job, SIGNAL( result( KJob * ) ),
             this,  SLOT( fileTransferred( KJob * ) ), Qt::QueuedConnection );

    connect( job, SIGNAL( copyingDone(KIO::Job*,KUrl,KUrl,time_t,bool,bool)),
             this, SLOT(slotCopyingDone(KIO::Job*,KUrl,KUrl,time_t,bool,bool)) );

    //return KIO::NetAccess::file_copy( src, dst );

    return true;
}

void
UmsHandler::fileTransferred( KJob *job )  //SLOT
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

    // Limit max number of jobs to 1, make sure more tracks left
    // to copy
    debug() << "Tracks to copy still remain";
    if( m_jobcounter < 1 )
    {
        debug() << "Jobs: " << m_jobcounter;
        copyNextTrackToDevice();
    }
}

void
UmsHandler::slotCopyingDone( KIO::Job* job, KUrl from, KUrl to, time_t mtime, bool directory, bool renamed)
{
    Q_UNUSED( job )
    Q_UNUSED( to )
    Q_UNUSED( mtime )
    Q_UNUSED( directory )
    Q_UNUSED( renamed )

    DEBUG_BLOCK
    Meta::TrackPtr track = m_trackscopying[from];

    if( !job->error() )
    {
        Meta::TrackPtr metafiletrack( new MetaFile::Track( to ) );
        Meta::MediaDeviceTrackPtr destTrack = m_srctodest.value( track );
        m_umstrackhash.insert( destTrack, metafiletrack );
        m_files.insert( to.path(), destTrack );
        slotFinalizeTrackCopy( track );
    }
}

void
UmsHandler::deleteFile( const KUrl &url )
{
    DEBUG_BLOCK
    debug() << "deleting " << url.prettyUrl();

    KIO::DeleteJob *job = KIO::del( url, KIO::HideProgressInfo );

    m_jobcounter++;

    if( m_jobcounter < UMS_MAX_CONCURRENT_JOBS )
        removeNextTrackFromDevice();

    connect( job, SIGNAL( result( KJob * ) ),
             this,  SLOT( fileDeleted( KJob * ) ) );

    return;
}

void
UmsHandler::fileDeleted( KJob *job )  //SLOT
{
    DEBUG_BLOCK
    if( job->error() )
        debug() << "file deletion failed: " << job->errorText();

    m_jobcounter--;

    debug() << "Tracks to delete still remain";
    if( m_jobcounter < UMS_MAX_CONCURRENT_JOBS )
    {
        debug() << "Jobs: " << m_jobcounter;
        removeNextTrackFromDevice();
    }

    KIO::DeleteJob *djob = reinterpret_cast<KIO::DeleteJob*> (job);

    if( djob )
    {
        KUrl url = djob->urls().first();

        Meta::TrackPtr track = m_tracksdeleting[ url ];
        Meta::MediaDeviceTrackPtr devtrack = Meta::MediaDeviceTrackPtr::staticCast( track );
        m_umstrackhash.remove( devtrack );
        m_files.remove( devtrack->playableUrl().path() );

        debug() << "emitting libRemoveTrackDone";

        slotFinalizeTrackRemove( track );
    }
}

QString
UmsHandler::libGetAlbum( const Meta::MediaDeviceTrackPtr &track )
{
    return m_umstrackhash.value( track )->album()->name();
}

QString
UmsHandler::libGetArtist( const Meta::MediaDeviceTrackPtr &track )
{
    DEBUG_BLOCK
    if ( !m_umstrackhash.contains( track ) )
        debug() << "Error!  track not in hash!";
    return m_umstrackhash.value( track )->artist()->name();
}

QString
UmsHandler::libGetComposer( const Meta::MediaDeviceTrackPtr &track )
{
    return m_umstrackhash.value( track )->composer()->name();
}

QString
UmsHandler::libGetGenre( const Meta::MediaDeviceTrackPtr &track )
{
    return m_umstrackhash.value( track )->genre()->name();
}

int
UmsHandler::libGetYear( const Meta::MediaDeviceTrackPtr &track )
{
    return m_umstrackhash.value( track )->year()->name().toInt();
}

float
UmsHandler::usedCapacity() const
{
    if ( !m_filepath.isEmpty() )
        return KDiskFreeSpaceInfo::freeSpaceInfo( m_filepath ).used();
    else
        return 0.0;
}

float
UmsHandler::totalCapacity() const
{
    DEBUG_BLOCK
    return m_capacity;
}

/// Sets

void
UmsHandler::libSetPlayableUrl( Meta::MediaDeviceTrackPtr &destTrack, const Meta::TrackPtr &srcTrack )
{
    destTrack->setPlayableUrl( KUrl( m_destinations.value( srcTrack ) ) );
}

void
UmsHandler::prepareToParseTracks()
{
    DEBUG_BLOCK

    debug() << "Scanning for music in " << m_musicPath.toLocalFile();
    m_watcher.addDir( m_musicPath.toLocalFile(), KDirWatch::WatchDirOnly | KDirWatch::WatchFiles | KDirWatch::WatchSubDirs );

    QDirIterator it( m_musicPath.toLocalFile(), QDirIterator::Subdirectories );
    while( it.hasNext() )
    {
        addPath( it.next() );
    }

    if( m_podcastProvider )
        m_podcastProvider->scan();

    m_parsed = true;
    m_listpos = 0;
    //m_currtrackurllist = m_dirLister->items().urlList();
    debug() << "list size: " << m_currtracklist.size();
}

bool
UmsHandler::isEndOfParseTracksList()
{
    return !( m_listpos < m_currtracklist.size() );
}

void
UmsHandler::prepareToParseNextTrack()
{
    m_listpos++;
}

void
UmsHandler::nextTrackToParse()
{
    Meta::TrackPtr track( new MetaFile::Track( m_currtracklist.at( m_listpos ) ) );
    m_currtrack = track;
}

void
UmsHandler::setAssociateTrack( const Meta::MediaDeviceTrackPtr track )
{
    m_umstrackhash.insert( track, m_currtrack );
    m_files.insert( m_currtrack->playableUrl().path(), track );
}

Meta::TrackPtr
UmsHandler::sourceTrack()
{
    return m_currtrack;
}

/// Playlist Parsing
#if 0
void
UmsHandler::prepareToParsePlaylists()
{
    m_currplaylistlist = m_itdb->playlists;
}


bool
UmsHandler::isEndOfParsePlaylistsList()
{
    return (m_currplaylistlist ? false : true);
}


void
UmsHandler::prepareToParseNextPlaylist()
{
    m_currplaylistlist = m_currplaylistlist->next;
}


void
UmsHandler::nextPlaylistToParse()
{
    m_currplaylist = ( Itdb_Playlist * ) m_currplaylistlist->data;
}

bool
UmsHandler::shouldNotParseNextPlaylist()
{
    // NOTE: skip the master playlist
    return ( itdb_playlist_is_mpl( m_currplaylist ) || itdb_playlist_is_podcasts( m_currplaylist ) );
}


void
UmsHandler::prepareToParsePlaylistTracks()
{
    m_currtracklist = m_currplaylist->members;
}


bool
UmsHandler::isEndOfParsePlaylist()
{
    return (m_currtracklist ? false : true );
}


void
UmsHandler::prepareToParseNextPlaylistTrack()
{
    prepareToParseNextTrack();
}


void
UmsHandler::nextPlaylistTrackToParse()
{
    nextTrackToParse();
}

Meta::MediaDeviceTrackPtr
UmsHandler::libGetTrackPtrForTrackStruct()
{
    return m_itdbtrackhash.key( m_currtrack );
}

QString
UmsHandler::libGetPlaylistName()
{
    return QString::fromUtf8( m_currplaylist->name );
}

void
UmsHandler::setAssociatePlaylist( const Playlists::MediaDevicePlaylistPtr &playlist )
{
    m_itdbplaylisthash[ playlist ] = m_currplaylist;
}

void
UmsHandler::libSavePlaylist( const Playlists::MediaDevicePlaylistPtr &playlist, const QString& name )
{
    DEBUG_BLOCK
    // Make new playlist

    Itdb_Playlist *pl = itdb_playlist_new( name.toUtf8(), 0 );
    itdb_playlist_add( m_itdb, pl, -1 );

    Meta::TrackList tracks = const_cast<Playlists::MediaDevicePlaylistPtr&> ( playlist )->tracks();

    foreach( const Meta::TrackPtr track, tracks )
    {
        itdb_playlist_add_track( pl, m_itdbtrackhash[ Meta::MediaDeviceTrackPtr::staticCast( track ) ], -1 );
    }

    m_itdbplaylisthash[ playlist ] = pl;

    setDatabaseChanged();
}

void
UmsHandler::deletePlaylist( const Playlists::MediaDevicePlaylistPtr &playlist )
{
    DEBUG_BLOCK
    Itdb_Playlist *pl = m_itdbplaylisthash.value( playlist );

    if( pl )
    {
        debug() << "Playlist removed";
        itdb_playlist_remove( pl );
        m_itdbplaylisthash.remove( playlist );
        setDatabaseChanged();
    }
}

void
UmsHandler::renamePlaylist( const Playlists::MediaDevicePlaylistPtr &playlist )
{
    DEBUG_BLOCK
    Itdb_Playlist *pl = m_itdbplaylisthash[ playlist ];

    if( pl )
    {
        debug() << "Playlist renamed";
        pl->name = g_strdup( playlist->name().toUtf8() );
        setDatabaseChanged();
    }
}
#endif

QStringList
UmsHandler::supportedFormats()
{
    QStringList formats;

    formats << "mp3" << "aac" << "mp4" << "m4a" << "m4b" << "flac" << "ogg" << "wma";

    return formats;
}


/* Private Functions */

void
UmsHandler::prepareToCopy()
{
    // Initialize job counter to prepare to keep track of how many
    // copy jobs are going on at once
    //m_jobcounter = 0;

    m_trackdesturl.clear();
    m_trackscopying.clear();
    m_srctodest.clear();
}

void
UmsHandler::prepareToDelete()
{
    // Initialize job counter to prepare to keep track of how many
    // copy jobs are going on at once
    //m_jobcounter = 0;

    m_tracksdeleting.clear();
}

void
UmsHandler::updateTrack( Meta::MediaDeviceTrackPtr &track )
{
    MetaFile::TrackPtr metafile = MetaFile::TrackPtr::staticCast( m_umstrackhash.value( track ) );
    if( !metafile )
        return;

    // Data has changed, update the MetaFile::Track
    metafile->setTitle( track->name() );
    if ( track->album() )
        metafile->setAlbum( track->album()->name() );
    if ( track->artist() )
        metafile->setArtist( track->artist()->name() );
    if ( track->composer() )
        metafile->setComposer( track->composer()->name() );
    if ( track->genre() )
        metafile->setGenre( track->genre()->name() );
    if ( track->year() )
        metafile->setYear( track->year()->name() );
    metafile->setTrackNumber( track->trackNumber() );
    metafile->setComment( track->comment() );
    metafile->setDiscNumber( track->discNumber() );

    metafile->setPlayCount( track->playCount() );
    metafile->setLastPlayed( track->lastPlayed() );
    metafile->setRating( track->rating() );
}

void
UmsHandler::endTrackRemove()
{
    // TODO: remove empty directories of deleted tracks
    /*
    /// Look for empty directories that should now be deleted
    //QMap<KUrl, Meta::TrackPtr> m_tracksdeleting;
    foreach( KUrl url, m_tracksdeleting.keys() )
    {
        QDir currDir( url.directory() );
        QDir dirup(

        // Pull out the directory one deeper than the mount root

        while( currDir.exists() && !( currDir.canonicalPath() == m_mountPoint ) )
        {
            // if it's empty, delete
            if( currDir.count() == 0 )
                currDir.rmdir( "
        }


    }
    */
}

/// Capability-related functions

bool
UmsHandler::hasCapabilityInterface( Handler::Capability::Type type ) const
{
    switch( type )
    {
        case Handler::Capability::Readable:
        case Handler::Capability::Writable:
            return true;
        case Handler::Capability::Artwork:
        case Handler::Capability::Playlist:
            return false;


        default:
            return false;
    }
}

Handler::Capability*
UmsHandler::createCapabilityInterface( Handler::Capability::Type type )
{
    switch( type )
    {
        case Handler::Capability::Readable:
            return new Handler::UmsReadCapability( this );
        case Handler::Capability::Writable:
            return new Handler::UmsWriteCapability( this );
            /*
        case Handler::Capability::Artwork:
            return new Handler::UmsArtworkCapability( this );

        case Handler::Capability::Playlist:
            return new Handler::UmsPlaylistCapability( this );

*/
        default:
            return 0;
    }
}

#include "UmsHandler.moc"

