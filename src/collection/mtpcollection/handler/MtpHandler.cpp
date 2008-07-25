/***************************************************************************
 * Ported to Collection Framework: *
 * copyright            : (C) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com>

 * Original Work: *
 * copyright            : (C) 2006 Andy Kelk <andy@mopoke.co.uk>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

#define DEBUG_PREFIX "MtpHandler"

#include "MtpHandler.h"

#include "MtpCollection.h"
#include "MtpMeta.h"
#include "../../../statusbar/StatusBar.h"
#include "Debug.h"

#include "File.h" // for KIO file handling

#include <KIO/Job>
#include <KIO/DeleteJob>
#include "kjob.h"
#include <KUniqueApplication> // needed for KIO processes
#include <KUrl>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QStringList>
#include <QTime>

using namespace Mtp;
using namespace Meta;

MtpHandler::MtpHandler( MtpCollection *mc, QObject *parent )
    : QObject( parent )
    , m_memColl( mc )
{
    DEBUG_BLOCK

    QString genericError = i18n( "Could not connect to MTP Device" );

    m_success = false;

    

    // begin checking connected devices

    LIBMTP_raw_device_t * rawdevices;
    int numrawdevices;
    LIBMTP_error_number_t err;
    int i;

    LIBMTP_Init();

    // get list of raw devices

    err = LIBMTP_Detect_Raw_Devices(&rawdevices, &numrawdevices);

    debug() << "Error is: " << err;

    switch(err) {
        case LIBMTP_ERROR_NO_DEVICE_ATTACHED:
            fprintf(stdout, "   No raw devices found.\n");
            break;
            
        case LIBMTP_ERROR_CONNECTING:
            fprintf(stderr, "Detect: There has been an error connecting. Exiting\n");
            break;
            
        case LIBMTP_ERROR_MEMORY_ALLOCATION:
            fprintf(stderr, "Detect: Encountered a Memory Allocation Error. Exiting\n");
            break;
            
        case LIBMTP_ERROR_NONE:
        {
            LIBMTP_mtpdevice_t *device;
            // test raw device for connectability
            for(i = 0; i < numrawdevices; i++)
            {
                

                device = LIBMTP_Open_Raw_Device(&rawdevices[i]);
                if (device == NULL) {
                    debug() << "Unable to open raw device: " << i;
                    continue;
                }
            }
    
    m_device = device;
    
    if( m_device == 0 ) {
        // TODO: error protection
        m_success = false;
        break;
    }

    QString modelname = QString( LIBMTP_Get_Modelname( m_device ) );
    QString ownername = QString( LIBMTP_Get_Friendlyname( m_device ) );
    m_name = modelname;
    if(! ownername.isEmpty() )
        m_name += " (" + ownername + ')';

    m_default_parent_folder = m_device->default_music_folder;
    debug() << "setting default parent : " << m_default_parent_folder;

    
    m_folders = LIBMTP_Get_Folder_List( m_device );
    uint16_t *filetypes;
    uint16_t filetypes_len;
    int ret = LIBMTP_Get_Supported_Filetypes( m_device, &filetypes, &filetypes_len );
    if( ret == 0 )
    {
        uint16_t i;
        for( i = 0; i < filetypes_len; i++ )
            m_supportedFiles << mtpFileTypes[ filetypes[ i ] ];
    }
    // find supported image types (for album art).
    if( m_supportedFiles.indexOf( "jpg" ) )
        m_format = "JPEG";
    else if( m_supportedFiles.indexOf( "png" ) )
        m_format = "PNG";
    else if( m_supportedFiles.indexOf( "gif" ) )
        m_format = "GIF";
    free( filetypes );

    m_success = true;
    break;
        }

        default:
            debug() << "Unhandled mtp error";
    }
    
    free( rawdevices );
    

}

MtpHandler::~MtpHandler()
{
    // TODO: free used memory
    LIBMTP_Release_Device( m_device );
    
}

void
MtpHandler::copyTrackToDevice( const Meta::TrackPtr &track )
{
    Q_UNUSED( track );
}

bool
MtpHandler::deleteTrackFromDevice( const Meta::MtpTrackPtr &track )
{
    // TODO: NYI
        Q_UNUSED( track );
    return false;
}

int
MtpHandler::readMtpMusic()
{
    DEBUG_BLOCK



    return 0;
}

void
MtpHandler::getBasicMtpTrackInfo( LIBMTP_track_t *mtptrack, Meta::MtpTrackPtr track )
{
    track->setTitle( QString::fromUtf8( mtptrack->title ) );
    if( mtptrack->duration > 0 )
        track->setLength( ( mtptrack->duration ) / 1000 );
    else
        track->setLength( 0 );
    track->setTrackNumber( mtptrack->tracknumber );
    track->setComment( QString() ); // defaulting, since not provided
    track->setDiscNumber( 1 ); // defaulting, since not provided
    track->setBitrate( mtptrack->bitrate );
    track->setFileSize( mtptrack->filesize );

    // NOTE: libmtp has no access to the filesystem, no url for playing, must find way around
//    track->setPlayableUrl( path );
    track->setPlayableUrl( "" ); // defaulting, since not provided
}

void
MtpHandler::setupArtistMap( LIBMTP_track_t *mtptrack, Meta::MtpTrackPtr track, ArtistMap &artistMap )
{
    QString artist ( QString::fromUtf8( mtptrack->artist ) );
    MtpArtistPtr artistPtr;

    if (  artistMap.contains(  artist ) )
    {
        artistPtr = MtpArtistPtr::staticCast(  artistMap.value(  artist ) );
    }
    else
    {
        artistPtr = MtpArtistPtr(  new MtpArtist(  artist ) );
        artistMap.insert(  artist,  ArtistPtr::staticCast(  artistPtr ) );
    }

    artistPtr->addTrack(  track );
    track->setArtist(  artistPtr );
}

void
MtpHandler::setupAlbumMap( LIBMTP_track_t *mtptrack, Meta::MtpTrackPtr track, AlbumMap &albumMap )
{
    QString album( QString::fromUtf8( mtptrack->album ) );
    MtpAlbumPtr albumPtr;

    if ( albumMap.contains( album ) )
        albumPtr = MtpAlbumPtr::staticCast(  albumMap.value(  album ) );

    else
    {
        albumPtr = MtpAlbumPtr(  new MtpAlbum(  album ) );
        albumMap.insert(  album,  AlbumPtr::staticCast(  albumPtr ) );
    }

    albumPtr->addTrack(  track );
    track->setAlbum(  albumPtr );
}

void
MtpHandler::setupGenreMap( LIBMTP_track_t *mtptrack, Meta::MtpTrackPtr track, GenreMap &genreMap )
{
    QString genre = mtptrack->genre;
    MtpGenrePtr genrePtr;

    if (  genreMap.contains(  genre ) )
        genrePtr = MtpGenrePtr::staticCast(  genreMap.value(  genre ) );

    else
    {
        genrePtr = MtpGenrePtr(  new MtpGenre(  genre ) );
        genreMap.insert(  genre,  GenrePtr::staticCast(  genrePtr ) );
    }

    genrePtr->addTrack( track );
    track->setGenre( genrePtr );
}

void
MtpHandler::setupComposerMap( LIBMTP_track_t *mtptrack, Meta::MtpTrackPtr track, ComposerMap &composerMap )
{
    QString composer ( QString::fromUtf8( mtptrack->composer ) );
    MtpComposerPtr composerPtr;

    if ( composerMap.contains( composer ) )
    {
        composerPtr = MtpComposerPtr::staticCast( composerMap.value( composer ) );
    }
    else
    {
        composerPtr = MtpComposerPtr( new MtpComposer( composer ) );
        composerMap.insert( composer, ComposerPtr::staticCast( composerPtr ) );
    }

    composerPtr->addTrack( track );
    track->setComposer( composerPtr );
}

void
MtpHandler::setupYearMap( LIBMTP_track_t *mtptrack, Meta::MtpTrackPtr track, YearMap &yearMap )
{
    QString year;
    year = year.setNum( ( QString::fromUtf8( mtptrack->date ) ).mid( 0, 4 ).toUInt() );
    MtpYearPtr yearPtr;
    if (  yearMap.contains(  year ) )
        yearPtr = MtpYearPtr::staticCast(  yearMap.value(  year ) );
    else
    {
        yearPtr = MtpYearPtr(  new MtpYear(  year ) );
        yearMap.insert(  year,  YearPtr::staticCast(  yearPtr ) );
    }
    yearPtr->addTrack(  track );
    track->setYear(  yearPtr );
}

bool
MtpHandler::kioCopyTrack( const KUrl &src, const KUrl &dst )
{
        DEBUG_BLOCK

    KIO::FileCopyJob *job = KIO::file_copy( src, dst,
                                            -1 /* permissions */,
                                            KIO::HideProgressInfo );

    connect( job, SIGNAL( result( KJob * ) ),
             this,  SLOT( fileTransferred( KJob * ) ) );

    The::statusBar()->newProgressOperation( job ).setDescription(  i18n(  "Transferring Tracks to MTP Device" )  );
    job->start();

    return true;
}
void
MtpHandler::deleteFile( const KUrl &url )
{
    Q_UNUSED( url );
}


void
MtpHandler::parseTracks()
{
    // NOTE: look at ReadMtpMusic function in MTPMediaDevice

        DEBUG_BLOCK

    TrackMap trackMap;
    ArtistMap artistMap;
    AlbumMap albumMap;
    GenreMap genreMap;
    ComposerMap composerMap;
    YearMap yearMap;

    LIBMTP_track_t *tracks = LIBMTP_Get_Tracklisting( m_device );

    debug() << "Got tracks from device";

    if( tracks == 0 )
    {
        debug() << "0 tracks returned. Empty device...";
        return;
    }

    QMap<LIBMTP_track_t*, MtpTrackPtr> mtpTrackMap;

/* iterate through tracklist and add to appropriate map */
    for ( ; tracks != 0; tracks = tracks->next )
    {
        /* mtptrack - provides libmtp info */
        /* track - the new track whose data is being set up */
        LIBMTP_track_t *mtptrack = tracks;
        QString format;

        if( mtptrack->filetype == LIBMTP_FILETYPE_MP3 )
            format = "mp3";
        else if( mtptrack->filetype == LIBMTP_FILETYPE_WMA )
            format = "wma";
        else if( mtptrack->filetype == LIBMTP_FILETYPE_OGG )
            format = "ogg";
        else
            format = "other";

        MtpTrackPtr track( new MtpTrack( m_memColl, format ) );

        getBasicMtpTrackInfo( mtptrack, track );

        /* map-related info retrieval */

        setupArtistMap( mtptrack, track, artistMap );
        setupAlbumMap( mtptrack, track, albumMap );
        setupGenreMap( mtptrack, track, genreMap );
        setupComposerMap( mtptrack, track, composerMap );
        setupYearMap( mtptrack, track, yearMap );

        /* TrackMap stuff to be subordinated later */

        trackMap.insert( track->url(), TrackPtr::staticCast( track ) );

        track->setMtpTrack( mtptrack ); // convenience pointer
        mtpTrackMap.insert( mtptrack, track ); // map for playlist formation
    }

    // TODO: Iterate through mtp's playlists to set track's playlists

    // Finally, assign the created maps to the collection

    m_memColl->acquireWriteLock();
    m_memColl->setTrackMap(  trackMap );
    m_memColl->setArtistMap(  artistMap );
    m_memColl->setAlbumMap(  albumMap );
    m_memColl->setGenreMap(  genreMap );
    m_memColl->setComposerMap(  composerMap );
    m_memColl->setYearMap(  yearMap );
    m_memColl->releaseLock();
}
void
MtpHandler::addMtpTrackToCollection( LIBMTP_track_t *mtptrack )
{
    Q_UNUSED( mtptrack );
}

void
MtpHandler::fileTransferred( KJob *job )  //SLOT
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
MtpHandler::fileDeleted( KJob *job )  //SLOT
{
    if(job->error())
    {
        debug() << "file deletion failed: " << job->errorText();
    }
}

QString
MtpHandler::prettyName() const
{
    return m_name;
}

