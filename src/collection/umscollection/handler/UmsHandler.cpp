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
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "UmsHandler"

#include "UmsHandler.h"

#include "UmsCollection.h"
#include "Debug.h"

#include "SvgHandler.h"

#include "File.h" // for KIO file handling

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
#include <QMutexLocker>
#include <QPixmap>
#include <QProcess>
#include <QRegExp>
#include <QString>
#include <QStringList>
#include <QTime>

using namespace Meta;

/// UmsHandler

UmsHandler::UmsHandler( UmsCollection *mc, const QString& mountPoint )
    : MediaDeviceHandler( mc )
    , m_watcher()
    , m_timer()
    , m_capacity( 0.0 )
    , m_jobcounter( 0 )
    , m_autoConnect( false )
    , m_mountPoint( mountPoint )
    , m_wasMounted( !mountPoint.isEmpty() )
    , m_name()
    , m_wait( false )
    , m_tempdir( new KTempDir() )
{
    DEBUG_BLOCK

    m_copyingthreadsafe = false;
    m_success = false;

}

UmsHandler::~UmsHandler()
{
    DEBUG_BLOCK
    delete m_tempdir;
}

void
UmsHandler::init()
{
    if( m_mountPoint.isEmpty() )
    {
        debug() << "Empty mountpoint, aborting";
        m_memColl->slotAttemptConnectionDone( false );
        return;
    }

    // Get storage access for getting device space capacity/usage

    Solid::Device device = Solid::Device(  m_memColl->udi() );
    if (  device.isValid() )
    {
        Solid::StorageAccess *storage = device.as<Solid::StorageAccess>();
        if ( storage )
            m_filepath = storage->filePath();
        else if ( !m_mountPoint.isEmpty() )
            m_filepath = m_mountPoint;

        if ( !m_filepath.isEmpty() )
            m_capacity = KDiskFreeSpaceInfo::freeSpaceInfo( m_filepath ).size();
        else
            m_capacity = 0.0;
    }
    else
    {
        m_filepath = "";
        m_capacity = 0.0;
    }

    m_formats << "mp3" << "wav" << "asf" << "flac" << "wma" << "ogg" << "aac" << "m4a"
            << "mp4" << "mp2" << "ac3";

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

    m_watcher.addDir( m_mountPoint, KDirWatch::WatchDirOnly | KDirWatch::WatchFiles | KDirWatch::WatchSubDirs );

    m_parsed = false;
    m_parseAction = 0;

    debug() << "Succeeded: true";
    m_memColl->emitCollectionReady();
    //m_memColl->slotAttemptConnectionDone( true );
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

    foreach( QString path, m_dirtylist )
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
    DEBUG_BLOCK
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
        foreach( QString format, m_formats )
            if( info.suffix() == format )
            {
                debug() << "File found is: " << info.canonicalFilePath();
                m_currtracklist << info.canonicalFilePath();
                return 2;
            }
    }

    return 0;
}

bool
UmsHandler::isWritable() const
{
    // TODO: check if writable
    return false;
}

QString
UmsHandler::prettyName() const
{
    Solid::Device device;

    device = Solid::Device( m_memColl->udi() );

    if ( device.isValid() )
    {
        return device.vendor().append( " " ).append( device.product() );
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
            m_parseAction = new QAction( KIcon( "media-track-edit-amarok" ), i18n(  "&Read Device" ), this );
            m_parseAction->setProperty( "popupdropper_svg_id", "edit" );

            connect( m_parseAction, SIGNAL( triggered() ), this, SLOT( parseTracks() ) );
        }

        actions.append( m_parseAction );
    }

    return actions;
}

void
UmsHandler::writeDatabase()
{
    return;
}

#if 0

bool
UmsHandler::libCopyTrack( const Meta::TrackPtr &srcTrack, Meta::MediaDeviceTrackPtr &destTrack )
{
    Q_UNUSED( destTrack )
    DEBUG_BLOCK
//    findPathToCopy( srcTrack );
    KUrl srcurl = KUrl::fromPath( srcTrack->playableUrl().path() );
    m_trackscopying[ srcurl ] = srcTrack;
    return kioCopyTrack( srcurl, m_trackdesturl[ srcTrack ] );
}

bool
UmsHandler::libDeleteTrackFile( const Meta::MediaDeviceTrackPtr &track )
{
    DEBUG_BLOCK
    Itdb_Track *umstrack = m_itdbtrackhash[ track ];

    // delete file
    KUrl url;
    url.setPath( realPath( umstrack->ums_path ) );
    Meta::TrackPtr trackptr = Meta::TrackPtr::staticCast( track );
    m_tracksdeleting[ url ] = trackptr;
    deleteFile( url );

    return true;

}

void
UmsHandler::libDeleteTrack( const Meta::MediaDeviceTrackPtr &track )
{
    DEBUG_BLOCK
    Itdb_Track *umstrack = m_itdbtrackhash[ track ];

    m_itdbtrackhash.remove( track );
    m_files.remove( QString(umstrack->ums_path).toLower() );

    itdb_track_remove( umstrack );
}

bool
UmsHandler::kioCopyTrack( const KUrl &src, const KUrl &dst )
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

    // Limit max number of jobs to 150, make sure more tracks left
    // to copy
    debug() << "Tracks to copy still remain";
    if( m_jobcounter < 150 )
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
        slotFinalizeTrackCopy( track );
}

void
UmsHandler::deleteFile( const KUrl &url )
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
UmsHandler::fileDeleted( KJob *job )  //SLOT
{
    DEBUG_BLOCK
    if( job->error() )
        debug() << "file deletion failed: " << job->errorText();

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
#endif
#if 0
QString
UmsHandler::libGetTitle( const Meta::MediaDeviceTrackPtr &track )
{
    return QString::fromUtf8( m_itdbtrackhash[ track ]->title );
}
#endif
QString
UmsHandler::libGetAlbum( const Meta::MediaDeviceTrackPtr &track )
{
    return m_umstrackhash.value( track )->album()->name();
}

QString
UmsHandler::libGetArtist( const Meta::MediaDeviceTrackPtr &track )
{
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
#if 0
int
UmsHandler::libGetLength( const Meta::MediaDeviceTrackPtr &track )
{
    return ( ( m_itdbtrackhash[ track ]->tracklen ) / 1000 );
}

int
UmsHandler::libGetTrackNumber( const Meta::MediaDeviceTrackPtr &track )
{
    return m_itdbtrackhash[ track ]->track_nr;
}

QString
UmsHandler::libGetComment( const Meta::MediaDeviceTrackPtr &track )
{
    return QString::fromUtf8( m_itdbtrackhash[ track ]->comment );
}

int
UmsHandler::libGetDiscNumber( const Meta::MediaDeviceTrackPtr &track )
{
    return m_itdbtrackhash[ track ]->cd_nr;
}

int
UmsHandler::libGetBitrate( const Meta::MediaDeviceTrackPtr &track )
{
    return m_itdbtrackhash[ track ]->bitrate;
}

int
UmsHandler::libGetSamplerate( const Meta::MediaDeviceTrackPtr &track )
{
    return m_itdbtrackhash[ track ]->samplerate;
}

float
UmsHandler::libGetBpm( const Meta::MediaDeviceTrackPtr &track )
{
    return m_itdbtrackhash[ track ]->BPM;
}
int
UmsHandler::libGetFileSize( const Meta::MediaDeviceTrackPtr &track )
{
    return m_itdbtrackhash[ track ]->size;
}
int
UmsHandler::libGetPlayCount( const Meta::MediaDeviceTrackPtr &track )
{
    return m_itdbtrackhash[ track ]->playcount;
}
uint
UmsHandler::libGetLastPlayed( const Meta::MediaDeviceTrackPtr &track )
{
    return m_itdbtrackhash[ track ]->time_played;
}
int
UmsHandler::libGetRating( const Meta::MediaDeviceTrackPtr &track )
{
    return ( m_itdbtrackhash[ track ]->rating / ITDB_RATING_STEP * 2 );
}
QString
UmsHandler::libGetType( const Meta::MediaDeviceTrackPtr &track )
{
    if( QString::fromUtf8( m_itdbtrackhash[ track ]->filetype ) == "mpeg" )
        return "mp3";

    return QString::fromUtf8( m_itdbtrackhash[ track ]->filetype );
}

KUrl
UmsHandler::libGetPlayableUrl( const Meta::MediaDeviceTrackPtr &track )
{
    return KUrl(m_mountPoint + (QString( m_itdbtrackhash[ track ]->ums_path ).split( ':' ).join( "/" )));
}
#endif

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
    return m_capacity;
}

/// Sets
#if 0
void
UmsHandler::libSetTitle( Meta::MediaDeviceTrackPtr& track, const QString& title )
{
    m_itdbtrackhash[ track ]->title = g_strdup( title.toUtf8() );
}
void
UmsHandler::libSetAlbum( Meta::MediaDeviceTrackPtr &track, const QString& album )
{
    m_itdbtrackhash[ track ]->album = g_strdup( album.toUtf8() );
}
void
UmsHandler::libSetArtist( Meta::MediaDeviceTrackPtr &track, const QString& artist )
{
    m_itdbtrackhash[ track ]->artist = g_strdup( artist.toUtf8() );
}
void
UmsHandler::libSetComposer( Meta::MediaDeviceTrackPtr &track, const QString& composer )
{
    m_itdbtrackhash[ track ]->composer = g_strdup( composer.toUtf8() );
}
void
UmsHandler::libSetGenre( Meta::MediaDeviceTrackPtr &track, const QString& genre )
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
UmsHandler::libSetYear( Meta::MediaDeviceTrackPtr &track, const QString& year )
{
    bool ok;
    int yr = year.toInt( &ok, 10 );
    if( ok )
        m_itdbtrackhash[ track ]->year = yr;
}
void
UmsHandler::libSetLength( Meta::MediaDeviceTrackPtr &track, int length )
{
    m_itdbtrackhash[ track ]->tracklen = length*1000;
}
void
UmsHandler::libSetTrackNumber( Meta::MediaDeviceTrackPtr &track, int tracknum )
{
    m_itdbtrackhash[ track ]->track_nr = tracknum;
}
void
UmsHandler::libSetComment( Meta::MediaDeviceTrackPtr &track, const QString& comment )
{
    m_itdbtrackhash[ track ]->comment = g_strdup( comment.toUtf8() );
}
void
UmsHandler::libSetDiscNumber( Meta::MediaDeviceTrackPtr &track, int discnum )
{
    m_itdbtrackhash[ track ]->cd_nr = discnum;
}
void
UmsHandler::libSetBitrate( Meta::MediaDeviceTrackPtr &track, int bitrate )
{
    m_itdbtrackhash[ track ]->bitrate = bitrate;
}
void
UmsHandler::libSetSamplerate( Meta::MediaDeviceTrackPtr &track, int samplerate )
{
    m_itdbtrackhash[ track ]->samplerate = samplerate;
}
void
UmsHandler::libSetBpm( Meta::MediaDeviceTrackPtr &track, float bpm )
{
    m_itdbtrackhash[ track ]->BPM = static_cast<int>( bpm );
}
void
UmsHandler::libSetFileSize( Meta::MediaDeviceTrackPtr &track, int filesize )
{
    m_itdbtrackhash[ track ]->size = filesize;
}
void
UmsHandler::libSetPlayCount( Meta::MediaDeviceTrackPtr &track, int playcount )
{
    m_itdbtrackhash[ track ]->playcount = playcount;
}
void
UmsHandler::libSetLastPlayed( Meta::MediaDeviceTrackPtr &track, uint lastplayed)
{
    Q_UNUSED( track )
    Q_UNUSED( lastplayed )
}
void
UmsHandler::libSetRating( Meta::MediaDeviceTrackPtr &track, int rating )
{
    m_itdbtrackhash[ track ]->rating = ( rating * ITDB_RATING_STEP / 2 );
}
void
UmsHandler::libSetType( Meta::MediaDeviceTrackPtr &track, const QString& type )
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
UmsHandler::libSetPlayableUrl( Meta::MediaDeviceTrackPtr &destTrack, const Meta::TrackPtr &srcTrack )
{
    KUrl copyurl = m_trackdesturl[ srcTrack ];
    QString pathname = copyurl.path();

    QString type = pathname.section('.', -1).toLower();
    type = type.toLower();

    debug() << "Path before put in ums_path: " << pathname;

    m_itdbtrackhash[ destTrack ]->ums_path = g_strdup( umsPath(pathname).toLatin1() );
    debug() << "on iPod: " << m_itdbtrackhash[ destTrack ]->ums_path;
}


void
UmsHandler::libCreateTrack( const Meta::MediaDeviceTrackPtr& track )
{
    m_umstrackhash[ track ] = itdb_track_new();
}
#endif
void
UmsHandler::prepareToParseTracks()
{
    DEBUG_BLOCK

    QDirIterator it( m_mountPoint, QDirIterator::Subdirectories );
    while( it.hasNext() )
    {
        addPath( it.next() );
    }

    m_parsed = true;
    m_listpos = 0;
    //m_currtrackurllist = m_dirLister->items().urlList();
    debug() << "list size: " << m_currtracklist.size();
}

bool
UmsHandler::isEndOfParseTracksList()
{
        DEBUG_BLOCK
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
        DEBUG_BLOCK
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
UmsHandler::setAssociatePlaylist( const Meta::MediaDevicePlaylistPtr &playlist )
{
    m_itdbplaylisthash[ playlist ] = m_currplaylist;
}

void
UmsHandler::libSavePlaylist( const Meta::MediaDevicePlaylistPtr &playlist, const QString& name )
{
    DEBUG_BLOCK
    // Make new playlist

    Itdb_Playlist *pl = itdb_playlist_new( name.toUtf8(), 0 );
    itdb_playlist_add( m_itdb, pl, -1 );

    Meta::TrackList tracks = const_cast<Meta::MediaDevicePlaylistPtr&> ( playlist )->tracks();

    foreach( const Meta::TrackPtr track, tracks )
    {
        itdb_playlist_add_track( pl, m_itdbtrackhash[ Meta::MediaDeviceTrackPtr::staticCast( track ) ], -1 );
    }

    m_itdbplaylisthash[ playlist ] = pl;

    databaseChanged();
}

void
UmsHandler::deletePlaylist( const Meta::MediaDevicePlaylistPtr &playlist )
{
    DEBUG_BLOCK
    Itdb_Playlist *pl = m_itdbplaylisthash.value( playlist );

    if( pl )
    {
        debug() << "Playlist removed";
        itdb_playlist_remove( pl );
        m_itdbplaylisthash.remove( playlist );
        databaseChanged();
    }
}

void
UmsHandler::renamePlaylist( const Meta::MediaDevicePlaylistPtr &playlist )
{
    DEBUG_BLOCK
    Itdb_Playlist *pl = m_itdbplaylisthash[ playlist ];

    if( pl )
    {
        debug() << "Playlist renamed";
        pl->name = g_strdup( playlist->name().toUtf8() );
        databaseChanged();
    }
}
#endif

QStringList
UmsHandler::supportedFormats()
{
    QStringList formats;

    formats << "mp3" << "aac" << "mp4" << "m4a";

    return formats;
}


/* Private Functions */
#if 0
void
UmsHandler::prepareToCopy()
{
    // Initialize job counter to prepare to keep track of how many
    // copy jobs are going on at once
    //m_jobcounter = 0;

    m_trackdesturl.clear();
    m_trackscopying.clear();
}

void
UmsHandler::prepareToDelete()
{
    // Initialize job counter to prepare to keep track of how many
    // copy jobs are going on at once
    //m_jobcounter = 0;

    m_tracksdeleting.clear();
}
#endif
/// Capability-related functions

bool
UmsHandler::hasCapabilityInterface( Handler::Capability::Type type ) const
{
    switch( type )
    {
        case Handler::Capability::Readable:
            return true;
        case Handler::Capability::Artwork:
        case Handler::Capability::Playlist:
        case Handler::Capability::Writable:
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
            /*
        case Handler::Capability::Artwork:
            return new Handler::UmsArtworkCapability( this );

        case Handler::Capability::Playlist:
            return new Handler::UmsPlaylistCapability( this );
        case Handler::Capability::Writable:
            return new Handler::UmsWriteCapability( this );
*/
        default:
            return 0;
    }
}

#include "UmsHandler.moc"

