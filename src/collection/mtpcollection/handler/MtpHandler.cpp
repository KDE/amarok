/****************************************************************************************
 * Copyright (c) 2006 Andy Kelk <andy@mopoke.co.uk>                                     *
 * Copyright (c) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "MtpHandler"

#include "MtpHandler.h"

#include "MtpCollection.h"
#include "Debug.h"

#include "File.h" // for KIO file handling

#include <KIO/Job>
#include <KIO/DeleteJob>
#include "kjob.h"
#include <threadweaver/ThreadWeaver.h>
#include <KUrl>

#include <QFileInfo>
#include <QString>
#include <QStringList>
#include <QTextStream>

using namespace Meta;

MtpHandler::MtpHandler( MtpCollection *mc )
        : MediaDeviceHandler( mc )
        , m_device( 0 )
        , m_default_parent_folder( 0 )
        , m_folders( 0 )
        , m_folderStructure()
        , m_format()
        , m_name()
        , m_supportedFiles()
        , m_isCanceled( false )
        , m_wait( false )
        , m_dbChanged( false )
        , m_tempdir( new KTempDir() )
{
    DEBUG_BLOCK
    m_copyingthreadsafe = true;
    m_tempdir->setAutoRemove( true );
//    init();
}

MtpHandler::~MtpHandler()
{
    // TODO: free used memory
    DEBUG_BLOCK
    // clear folder structure

    if ( m_folders != 0 )
    {

        LIBMTP_destroy_folder_t( m_folders );

        m_folders = 0;
        debug() << "Folders destroyed";
    }

    // Delete temporary files

    //delete m_tempdir;

    // release device

    if ( m_device != 0 )
    {

        LIBMTP_Release_Device( m_device );
        /* possible race condition with statusbar destructor,
        will uncomment when fixed */
        //The::statusBar()->longMessage(
        //                       i18n( "The MTP device %1 has been disconnected", prettyName() ), StatusBar::Information );
        debug() << "Device released";
    }

}

bool
MtpHandler::isWritable() const
{
    // TODO: check if read-only
    return true;
}

void
MtpHandler::init()
{

    mtpFileTypes[LIBMTP_FILETYPE_WAV] = "wav";
    mtpFileTypes[LIBMTP_FILETYPE_MP3] = "mp3";
    mtpFileTypes[LIBMTP_FILETYPE_WMA] = "wma";
    mtpFileTypes[LIBMTP_FILETYPE_OGG] = "ogg";
    mtpFileTypes[LIBMTP_FILETYPE_AUDIBLE] = "aa";
    mtpFileTypes[LIBMTP_FILETYPE_MP4] = "mp4";
    mtpFileTypes[LIBMTP_FILETYPE_UNDEF_AUDIO] = "undef-audio";
    mtpFileTypes[LIBMTP_FILETYPE_WMV] = "wmv";
    mtpFileTypes[LIBMTP_FILETYPE_AVI] = "avi";
    mtpFileTypes[LIBMTP_FILETYPE_MPEG] = "mpg";
    mtpFileTypes[LIBMTP_FILETYPE_ASF] = "asf";
    mtpFileTypes[LIBMTP_FILETYPE_QT] = "mov";
    mtpFileTypes[LIBMTP_FILETYPE_UNDEF_VIDEO] = "undef-video";
    mtpFileTypes[LIBMTP_FILETYPE_JPEG] = "jpg";
    mtpFileTypes[LIBMTP_FILETYPE_JFIF] = "jfif";
    mtpFileTypes[LIBMTP_FILETYPE_TIFF] = "tiff";
    mtpFileTypes[LIBMTP_FILETYPE_BMP] = "bmp";
    mtpFileTypes[LIBMTP_FILETYPE_GIF] = "gif";
    mtpFileTypes[LIBMTP_FILETYPE_PICT] = "pict";
    mtpFileTypes[LIBMTP_FILETYPE_PNG] = "png";
    mtpFileTypes[LIBMTP_FILETYPE_VCALENDAR1] = "vcs";
    mtpFileTypes[LIBMTP_FILETYPE_VCALENDAR2] = "vcs";
    mtpFileTypes[LIBMTP_FILETYPE_VCARD2] = "vcf";
    mtpFileTypes[LIBMTP_FILETYPE_VCARD3] = "vcf";
    mtpFileTypes[LIBMTP_FILETYPE_WINDOWSIMAGEFORMAT] = "wim";
    mtpFileTypes[LIBMTP_FILETYPE_WINEXEC] = "exe";
    mtpFileTypes[LIBMTP_FILETYPE_TEXT] = "txt";
    mtpFileTypes[LIBMTP_FILETYPE_HTML] = "html";
    mtpFileTypes[LIBMTP_FILETYPE_AAC] = "aac";
    mtpFileTypes[LIBMTP_FILETYPE_FLAC] = "flac";
    mtpFileTypes[LIBMTP_FILETYPE_MP2] = "mp3";
    mtpFileTypes[LIBMTP_FILETYPE_M4A] = "m4a";
    mtpFileTypes[LIBMTP_FILETYPE_DOC] = "doc";
    mtpFileTypes[LIBMTP_FILETYPE_XML] = "xml";
    mtpFileTypes[LIBMTP_FILETYPE_XLS] = "xls";
    mtpFileTypes[LIBMTP_FILETYPE_PPT] = "ppt";
    mtpFileTypes[LIBMTP_FILETYPE_MHT] = "mht";
    mtpFileTypes[LIBMTP_FILETYPE_JP2] = "jpg";
    mtpFileTypes[LIBMTP_FILETYPE_JPX] = "jpx";
    mtpFileTypes[LIBMTP_FILETYPE_UNKNOWN] = "unknown";

    QString genericError = i18n( "Could not connect to MTP Device" );

    m_success = false;

    // begin checking connected devices

    LIBMTP_raw_device_t * rawdevices;
    int numrawdevices;
    LIBMTP_error_number_t err;

    debug() << "Initializing MTP stuff";
    LIBMTP_Init();

    // get list of raw devices
    debug() << "Getting list of raw devices";
    err = LIBMTP_Detect_Raw_Devices( &rawdevices, &numrawdevices );

    debug() << "Error is: " << err;

    switch ( err )
    {
    case LIBMTP_ERROR_NO_DEVICE_ATTACHED:
        fprintf( stdout, "   No raw devices found.\n" );
        m_success = false;
        break;

    case LIBMTP_ERROR_CONNECTING:
        fprintf( stderr, "Detect: There has been an error connecting. Exiting\n" );
        m_success = false;
        break;

    case LIBMTP_ERROR_MEMORY_ALLOCATION:
        fprintf( stderr, "Detect: Encountered a Memory Allocation Error. Exiting\n" );
        m_success = false;
        break;

    case LIBMTP_ERROR_NONE:
    {
        m_success = true;
        break;
    }

    default:
        debug() << "Unhandled mtp error";
        m_success = false;
        break;
    }



    if ( m_success )
    {
        debug() << "Got mtp list, connecting to device using thread";
        ThreadWeaver::Weaver::instance()->enqueue( new WorkerThread( numrawdevices, rawdevices, this ) );
    }
    else
    {
        free( rawdevices );
//        emit failed();
    }

}


// this function is threaded
bool
MtpHandler::iterateRawDevices( int numrawdevices, LIBMTP_raw_device_t* rawdevices )
{
    DEBUG_BLOCK

    bool success = false;

    LIBMTP_mtpdevice_t *device = 0;
    // test raw device for connectability
    for ( int i = 0; i < numrawdevices; i++ )
    {

        debug() << "Opening raw device number: " << ( i + 1 );
        device = LIBMTP_Open_Raw_Device( &rawdevices[i] );
        if ( device == NULL )
        {
            debug() << "Unable to open raw device: " << ( i + 1 );
            success = false;
            continue;
        }

//        debug() << "Testing serial number";

        // HACK: not checking serial to confirm the right device is in place
        // this is not incorrect, and long-term goal is to remove serial number from use altogether

        /*
        QString mtpSerial = QString::fromUtf8( LIBMTP_Get_Serialnumber( device ) );
        if( !mtpSerial.contains(serial) )
        {
            debug() << "Wrong device, going to next";
            debug() << "Expected: " << serial << " but got: " << mtpSerial;
            success = false;
            LIBMTP_Release_Device( device );
            continue;
        }
        */

        debug() << "Correct device found";
        success = true;
        break;
    }

    m_device = device;

    if ( m_device == 0 )
    {
        // TODO: error protection
        success = false;
        free( rawdevices );

    }

    //QString serial = QString::fromUtf8( LIBMTP_Get_Serialnumber( m_device ) );

//    debug() << "Serial is: " << serial;

    debug() << "Success is: " << ( success ? "true" : "false" );

    return success;
}

void
MtpHandler::getDeviceInfo()
{
    // Get information for device

    // Get Battery level and print to debug

    unsigned char max;
    unsigned char curr;
    int failed;

    failed = LIBMTP_Get_Batterylevel( m_device, &max, &curr );

    if ( !failed )
        debug() << "Battery at: " << curr << "/" << max;
    else
        debug() << "Unknown battery level";

    if( LIBMTP_Get_Storage( m_device, LIBMTP_STORAGE_SORTBY_NOTSORTED ) != 0 )
    {
        debug() << "Failed to get storage properties, cannot get capacity";
        m_capacity = 0.0;
    }

    else
    {
        m_capacity =  m_device->storage->FreeSpaceInBytes;
        m_capacity = m_device->storage->MaxCapacity;
    }



    QString modelname = QString( LIBMTP_Get_Modelname( m_device ) );

    // NOTE: on next libmtp bump, may reintroduce owner name
    // for now it doesn't work as expected
    /*
    QString ownername = QString( LIBMTP_Get_Friendlyname( m_device ) );
    m_name = modelname;
    if(! ownername.isEmpty() )
        if( modelname != ownername )
            m_name += " (" + ownername + ')';

    else
        m_name += " (No Owner Name)";

    */

    m_name = modelname;

    m_default_parent_folder = m_device->default_music_folder;
    debug() << "setting default parent : " << m_default_parent_folder;


    m_folders = LIBMTP_Get_Folder_List( m_device );
    uint16_t *filetypes;
    uint16_t filetypes_len;
    int ret = LIBMTP_Get_Supported_Filetypes( m_device, &filetypes, &filetypes_len );
    if ( ret == 0 )
    {
        uint16_t i;
        for ( i = 0; i < filetypes_len; ++i )
        {
            debug() << "Device supports: " << mtpFileTypes[ filetypes[ i ] ];
            m_supportedFiles << mtpFileTypes[ filetypes[ i ] ];
        }
    }
    // find supported image types (for album art).
    if ( m_supportedFiles.indexOf( "jpg" ) )
        m_format = "JPEG";
    else if ( m_supportedFiles.indexOf( "png" ) )
        m_format = "PNG";
    else if ( m_supportedFiles.indexOf( "gif" ) )
        m_format = "GIF";
    free( filetypes );
}

void
MtpHandler::terminate()
{
    DEBUG_BLOCK
    // clear folder structure
    if ( m_folders != 0 )
    {

        LIBMTP_destroy_folder_t( m_folders );

        m_folders = 0;
        debug() << "Folders destroyed";
    }

    // Delete temporary files

    //delete m_tempdir;
/*
    TrackMap trackMap = m_memColl->trackMap();

    foreach( Meta::TrackPtr track,  trackMap.values() )
    {
        Meta::MtpTrackPtr mtptrack = Meta::MtpTrackPtr::staticCast( track );
        mtptrack->deleteTempFile();
    }
*/


    // release device
    if ( m_device != 0 )
    {

        LIBMTP_Release_Device( m_device );
        /* possible race condition with statusbar destructor,
        will uncomment when fixed */
        //The::statusBar()->longMessage(
//                       i18n( "The MTP device %1 has been disconnected", prettyName() ), StatusBar::Information );
        debug() << "Device released";
    }
}

void
MtpHandler::getCopyableUrls( const Meta::TrackList &tracks )
{
        DEBUG_BLOCK

            QMap<Meta::TrackPtr,  KUrl> urls;

        //m_tempdir.setAutoRemove(  true );

        QString genericError = i18n(  "Could not copy track from device." );

        foreach(  Meta::TrackPtr trackptr,  tracks )
            {
                Meta::MediaDeviceTrackPtr track = Meta::MediaDeviceTrackPtr::staticCast(  trackptr );
                if (  !track )
                    break;


                QString trackFileName = QString::fromUtf8( m_mtptrackhash[ track ]->filename );

                QString filename = m_tempdir->name() + trackFileName;

                debug() << "Temp Filename: " << filename;

                int ret = getTrackToFile( m_mtptrackhash[ track ]->item_id,  filename );
                if (  ret != 0 )
                {
                    debug() << "Get Track failed: " << ret;
                    /*The::statusBar()->shortLongMessage(
                      genericError,
                      i18n( "Could not copy track from device." ),
                      StatusBar::Error
                      );*/
                }
                else
                {
                    urls.insert(  trackptr,  filename );
                }

            }

        emit gotCopyableUrls( urls );
}

/**
 * Check (and optionally create) the folder structure to put a
 * track into. Return the (possibly new) parent folder ID
 */
uint32_t
MtpHandler::checkFolderStructure( const Meta::TrackPtr track, bool create )
{
    QString artistName;
    Meta::ArtistPtr artist = track->artist();
    if ( !artist || artist->prettyName().isEmpty() )
        artistName = i18n( "Unknown Artist" );
    else
        artistName = artist->prettyName();
    //FIXME: Port
//     if( bundle.compilation() == MetaBundle::CompilationYes )
//         artist = i18n( "Various Artists" );
    QString albumName;
    Meta::AlbumPtr album = track->album();
    if ( !album || album->prettyName().isEmpty() )
        albumName = i18n( "Unknown Album" );
    else
        albumName = album->prettyName();
    QString genreName;
    Meta::GenrePtr genre = track->genre();
    if ( !genre || genre->prettyName().isEmpty() )
        genreName = i18n( "Unknown Genre" );
    else
        genreName = genre->prettyName();

    uint32_t parent_id = getDefaultParentId();
    QStringList folders = m_folderStructure.split( '/' ); // use slash as a dir separator
    QString completePath;
    for ( QStringList::Iterator it = folders.begin(); it != folders.end(); ++it )
    {
        if (( *it ).isEmpty() )
            continue;
        // substitute %a , %b , %g
        ( *it ).replace( QRegExp( "%a" ), artistName )
        .replace( QRegExp( "%b" ), albumName )
        .replace( QRegExp( "%g" ), genreName );
        // check if it exists
        uint32_t check_folder = subfolderNameToID(( *it ).toUtf8(), m_folders, parent_id );
        // create if not exists (if requested)
        if ( check_folder == 0 )
        {
            if ( create )
            {
                check_folder = createFolder(( *it ).toUtf8() , parent_id );
                if ( check_folder == 0 )
                {
                    return 0;
                }
            }
            else
            {
                return 0;
            }
        }
        completePath += ( *it ).toUtf8() + '/';
        // set new parent
        parent_id = check_folder;
    }
    debug() << "Folder path : " << completePath;
    // return parent
    return parent_id;
}

uint32_t
MtpHandler::getDefaultParentId( void )
{
    // Decide which folder to send it to:
    // If the device gave us a parent_folder setting, we use it
    uint32_t parent_id = 0;
    if ( m_default_parent_folder )
    {
        parent_id = m_default_parent_folder;
    }
    // Otherwise look for a folder called "Music"
    else if ( m_folders != 0 )
    {
        parent_id = folderNameToID( qstrdup( QString( "Music" ).toUtf8() ), m_folders );
        if ( !parent_id )
        {
            debug() << "Parent folder could not be found. Going to use top level.";
        }
    }
    // Give up and don't set a parent folder, let the device deal with it
    else
    {
        debug() << "No folders found. Going to use top level.";
    }
    return parent_id;
}

/**
 * Recursively search the folder list for a matching one
 * and return its ID
 */
uint32_t
MtpHandler::folderNameToID( char *name, LIBMTP_folder_t *folderlist )
{
    uint32_t i;

    if ( folderlist == 0 )
        return 0;

    if ( !strcasecmp( name, folderlist->name ) )
        return folderlist->folder_id;

    if (( i = ( folderNameToID( name, folderlist->child ) ) ) )
        return i;
    if (( i = ( folderNameToID( name, folderlist->sibling ) ) ) )
        return i;

    return 0;
}

/**
 * Recursively search the folder list for a matching one under the specified
 * parent ID and return the child's ID
 */
uint32_t
MtpHandler::subfolderNameToID( const char *name, LIBMTP_folder_t *folderlist, uint32_t parent_id )
{
    uint32_t i;

    if ( folderlist == 0 )
        return 0;

    if ( !strcasecmp( name, folderlist->name ) && folderlist->parent_id == parent_id )
        return folderlist->folder_id;

    if (( i = ( subfolderNameToID( name, folderlist->child, parent_id ) ) ) )
        return i;
    if (( i = ( subfolderNameToID( name, folderlist->sibling, parent_id ) ) ) )
        return i;

    return 0;
}

/**
 * Create a new mtp folder
 */
uint32_t
MtpHandler::createFolder( const char *name, uint32_t parent_id )
{
    debug() << "Creating new folder '" << name << "' as a child of " << parent_id;
    char *name_copy = qstrdup( name );
    // NOTE: api change, 0 refers to default storage_id
    uint32_t new_folder_id = LIBMTP_Create_Folder( m_device, name_copy, parent_id, 0 );
    delete( name_copy );
    debug() << "New folder ID: " << new_folder_id;
    if ( new_folder_id == 0 )
    {
        debug() << "Attempt to create folder '" << name << "' failed.";
        return 0;
    }
    updateFolders();

    return new_folder_id;
}

/**
 * Update local cache of mtp folders
 */
void
MtpHandler::updateFolders( void )
{
    LIBMTP_destroy_folder_t( m_folders );
    m_folders = 0;
    m_folders = LIBMTP_Get_Folder_List( m_device );
}
#if 0

void
MtpHandler::privateDeleteTrackFromDevice( const Meta::MtpTrackPtr &track )
{
    DEBUG_BLOCK

    //If nothing is left in a folder, delete the folder
    u_int32_t object_id = track->id();

    QString genericError = i18n( "Could not delete item" );

    debug() << "delete this id : " << object_id;


    int status = LIBMTP_Delete_Object( m_device, object_id );

    if ( status != 0 )
    {
        debug() << "delete object failed";
        The::statusBar()->longMessage(
            i18n( "Delete failed" ),
            StatusBar::Error
        );
//       return false;
    }
    debug() << "object deleted";

//   return true;

    m_titlemap.remove( track->name(), Meta::TrackPtr::staticCast( track ) );

}
#endif
int
MtpHandler::getTrackToFile( const uint32_t id, const QString & filename )
{
    DEBUG_BLOCK

//    The::statusBar()->shortMessage( i18n( "Loading Track" ) );
    /*
        connect( this, SIGNAL( setProgress( int ) ),
                 The::statusBar(), SLOT( setProgress( int ) ) );

        connect( this, SIGNAL( endProgressOperation( const QObject*) ),
                 The::statusBar(), SLOT( endProgressOperation( const QObject* ) ) );
                */

    return LIBMTP_Get_Track_To_File( m_device, id, filename.toUtf8(), 0, 0 );
}

int
MtpHandler::progressCallback( uint64_t const sent, uint64_t const total, void const * const data )
{
    DEBUG_BLOCK
    Q_UNUSED( sent );
    MtpHandler *handler = ( MtpHandler* )( data );

    // NOTE: setting max many times wastes cycles,
    // but how else to get total outside of callback?

    debug() << "Setting max to: " << (( int ) total );

    debug() << "Device: " << handler->prettyName();
/*
    handler->setBarMaximum(( int ) total );
    handler->setBarProgress(( int ) sent );

    if ( sent == total )
        handler->endBarProgressOperation();
    */

    return 0;
}


QString
MtpHandler::prettyName() const
{
    return m_name;
}

/// Begin MediaDeviceHandler overrides

void
MtpHandler::findPathToCopy( const Meta::TrackPtr &srcTrack, const Meta::MediaDeviceTrackPtr &destTrack )
{
    Q_UNUSED( destTrack );
    uint32_t parent_id = 0;
    if ( !m_folderStructure.isEmpty() )
    {
        parent_id = checkFolderStructure( srcTrack, true ); // true means create
        if ( parent_id == 0 )
        {
            debug() << "Could not create new parent (" << m_folderStructure << ")";
            /*The::statusBar()->shortLongMessage(
            genericError,
            i18n( "Cannot create parent folder. Check your structure." ),
            StatusBar::Error
                                              );*/
            return;
        }
    }
    else
    {
        parent_id = getDefaultParentId();
    }
    debug() << "Parent id : " << parent_id;

    m_copyparentid = parent_id;

}

bool
MtpHandler::libCopyTrack( const Meta::TrackPtr &srcTrack, Meta::MediaDeviceTrackPtr &destTrack )
{
    DEBUG_BLOCK

    findPathToCopy( srcTrack, destTrack );
    debug() << "sending...";

    debug() << "Playable Url is: " << srcTrack->playableUrl();
    debug() << "Sending file with path: " << srcTrack->playableUrl().path().toUtf8();



    int ret = LIBMTP_Send_Track_From_File( m_device, qstrdup( srcTrack->playableUrl().path().toUtf8() ), m_mtptrackhash[ destTrack ],
                                           0, 0 );

    debug() << "sent";
//    emit canCopyMoreTracks();

//    emit libCopyTrackDone( srcTrack );

    return ( ret == 0 );

}


// TODO: nyi

void
MtpHandler::writeDatabase()
{
    slotDatabaseWritten( true );
}


void
MtpHandler::libDeleteTrack( const Meta::MediaDeviceTrackPtr &track )
{
    DEBUG_BLOCK
    LIBMTP_track_struct *mtptrack = m_mtptrackhash[ track ];

    m_mtptrackhash.remove( track );

    u_int32_t object_id = mtptrack->item_id;

    QString genericError = i18n( "Could not delete item" );

    debug() << "delete this id : " << object_id;


    int status = LIBMTP_Delete_Object( m_device, object_id );

    removeNextTrackFromDevice();

    if ( status != 0 )
    {
        debug() << "delete object failed";
        /*
        The::statusBar()->longMessage(
            i18n( "Delete failed" ),
            StatusBar::Error
            */
        //);
//       return false;
    }
    else
        debug() << "object deleted";
}

void
MtpHandler::databaseChanged()
{
    m_dbChanged = true;
}


void
MtpHandler::prepareToParseTracks()
{
    m_currtracklist = LIBMTP_Get_Tracklisting_With_Callback( m_device, 0, this );
}

bool
MtpHandler::isEndOfParseTracksList()
{
    return (m_currtracklist ? false : true);
}

void
MtpHandler::prepareToParseNextTrack()
{
    m_currtracklist = m_currtracklist->next;
}

void
MtpHandler::nextTrackToParse()
{
    m_currtrack = m_currtracklist;
}

/// Playlist Parsing

void
MtpHandler::prepareToParsePlaylists()
{
    m_currplaylistlist = LIBMTP_Get_Playlist_List( m_device );
}


bool
MtpHandler::isEndOfParsePlaylistsList()
{
    return (m_currplaylistlist == 0);
}


void
MtpHandler::prepareToParseNextPlaylist()
{
    m_currplaylistlist = m_currplaylistlist->next;
}


void
MtpHandler::nextPlaylistToParse()
{
    m_currplaylist = m_currplaylistlist;
}

bool
MtpHandler::shouldNotParseNextPlaylist()
{
    // NOTE: parse all
    return false;
}


void
MtpHandler::prepareToParsePlaylistTracks()
{
    m_trackcounter = 0;
}


bool
MtpHandler::isEndOfParsePlaylist()
{
    return (m_trackcounter >= m_currplaylist->no_tracks);
}


void
MtpHandler::prepareToParseNextPlaylistTrack()
{
    m_trackcounter++;
}


void
MtpHandler::nextPlaylistTrackToParse()
{
    m_currtrack = m_idtrackhash[ m_currplaylist->tracks[ m_trackcounter ] ];
}


Meta::MediaDeviceTrackPtr
MtpHandler::libGetTrackPtrForTrackStruct()
{
    return m_mtptrackhash.key( m_currtrack );
}

QString
MtpHandler::libGetPlaylistName()
{
    return QString::fromUtf8( m_currplaylist->name );
}

void
MtpHandler::setAssociateTrack( const Meta::MediaDeviceTrackPtr track )
{
    m_mtptrackhash[ track ] = m_currtrack;
    m_idtrackhash[ m_currtrack->item_id ] = m_currtrack;
}

QStringList
MtpHandler::supportedFormats()
{
    return m_supportedFiles;
}


QString
MtpHandler::libGetTitle( const Meta::MediaDeviceTrackPtr &track )
{
    return QString::fromUtf8( m_mtptrackhash[ track ]->title );
}

QString
MtpHandler::libGetAlbum( const Meta::MediaDeviceTrackPtr &track )
{
    return QString::fromUtf8( m_mtptrackhash[ track ]->album );
}

QString
MtpHandler::libGetArtist( const Meta::MediaDeviceTrackPtr &track )
{
    return QString::fromUtf8( m_mtptrackhash[ track ]->artist );
}

QString
MtpHandler::libGetComposer( const Meta::MediaDeviceTrackPtr &track )
{
    return QString::fromUtf8( m_mtptrackhash[ track ]->composer );
}

QString
MtpHandler::libGetGenre( const Meta::MediaDeviceTrackPtr &track )
{
    return QString::fromUtf8( m_mtptrackhash[ track ]->genre );
}

int
MtpHandler::libGetYear( const Meta::MediaDeviceTrackPtr &track )
{
    return QString::fromUtf8( m_mtptrackhash[ track ]->date ).mid( 0, 4 ).toUInt();
}

int
MtpHandler::libGetLength( const Meta::MediaDeviceTrackPtr &track )
{
    if ( m_mtptrackhash[ track ]->duration > 0 )
        return ( ( m_mtptrackhash[ track ]->duration ) / 1000 );
    else
        return 0;
}

int
MtpHandler::libGetTrackNumber( const Meta::MediaDeviceTrackPtr &track )
{
    return m_mtptrackhash[ track ]->tracknumber;
}

QString
MtpHandler::libGetComment( const Meta::MediaDeviceTrackPtr &track )
{
    // NOTE: defaulting, since not provided
    Q_UNUSED( track );
    return QString();
}

int
MtpHandler::libGetDiscNumber( const Meta::MediaDeviceTrackPtr &track )
{
    Q_UNUSED( track );
    // NOTE: defaulting, since not provided
    return 1;
}

int
MtpHandler::libGetBitrate( const Meta::MediaDeviceTrackPtr &track )
{
    return m_mtptrackhash[ track ]->bitrate;
}

int
MtpHandler::libGetSamplerate( const Meta::MediaDeviceTrackPtr &track )
{
    return m_mtptrackhash[ track ]->samplerate;
}

float
MtpHandler::libGetBpm( const Meta::MediaDeviceTrackPtr &track )
{
    Q_UNUSED( track );
    // NOTE: defaulting, since not provided
    return 0.0;
}
int
MtpHandler::libGetFileSize( const Meta::MediaDeviceTrackPtr &track )
{
    return m_mtptrackhash[ track ]->filesize;
}
int
MtpHandler::libGetPlayCount( const Meta::MediaDeviceTrackPtr &track )
{
    return m_mtptrackhash[ track ]->usecount;
}
uint
MtpHandler::libGetLastPlayed( const Meta::MediaDeviceTrackPtr &track )
{
    Q_UNUSED( track );
    // NOTE: defaulting, since not provided
    return 0;
}

// TODO: implement rating
int
MtpHandler::libGetRating( const Meta::MediaDeviceTrackPtr &track )
{
    return ( m_mtptrackhash[ track ]->rating / 10 );
}
QString
MtpHandler::libGetType( const Meta::MediaDeviceTrackPtr &track )
{
    return mtpFileTypes[ m_mtptrackhash[ track ]->filetype ];
}

KUrl
MtpHandler::libGetPlayableUrl( const Meta::MediaDeviceTrackPtr &track )
{
    Q_UNUSED( track )
    // NOTE: not a real url, using for unique key for qm
        return KUrl( QString::number(  m_mtptrackhash[ track ]->item_id,  10 ) );
}

float
MtpHandler::usedCapacity() const
{
    DEBUG_BLOCK
    return m_capacity;
}

float
MtpHandler::totalCapacity() const
{
    DEBUG_BLOCK
    if( LIBMTP_Get_Storage( m_device, LIBMTP_STORAGE_SORTBY_NOTSORTED ) != 0 )
    {
        debug() << "Failed to get storage properties, cannot get capacity";
        return 0.0;
    }

    else
    {
        return m_device->storage->FreeSpaceInBytes;
    }

}

/// Sets

void
MtpHandler::libSetTitle( Meta::MediaDeviceTrackPtr& track, const QString& title )
{
//    m_mtptrackhash[ track ]->title = ( title.isEmpty() ? qstrdup( i18n( "Unknown title" ).toUtf8() ) : qstrdup( title.toUtf8() ) );
    m_mtptrackhash[ track ]->title = ( title.isEmpty() ? qstrdup( "" ) : qstrdup( title.toUtf8() ) );
    debug() << "Set to: " << m_mtptrackhash[ track ]->title;
}
void
MtpHandler::libSetAlbum( Meta::MediaDeviceTrackPtr &track, const QString& album )
{
//    m_mtptrackhash[ track ]->album = ( album.isEmpty() ? qstrdup( i18n( "Unknown album" ).toUtf8() ) : qstrdup( album.toUtf8() ) );
    m_mtptrackhash[ track ]->album = ( album.isEmpty() ? qstrdup( "" ) : qstrdup( album.toUtf8() ) );
    debug() << "Set to: " << m_mtptrackhash[ track ]->album;
}
void
MtpHandler::libSetArtist( Meta::MediaDeviceTrackPtr &track, const QString& artist )
{
//    m_mtptrackhash[ track ]->artist = ( artist.isEmpty() ? qstrdup( i18n( "Unknown artist" ).toUtf8() ) : qstrdup( artist.toUtf8() ) );
    m_mtptrackhash[ track ]->artist = ( artist.isEmpty() ? qstrdup( "" ) : qstrdup( artist.toUtf8() ) );
    debug() << "Set to: " << m_mtptrackhash[ track ]->artist;
}
void
MtpHandler::libSetComposer( Meta::MediaDeviceTrackPtr &track, const QString& composer )
{
//    m_mtptrackhash[ track ]->composer = ( composer.isEmpty() ? qstrdup( i18n( "Unknown composer" ).toUtf8() ) : qstrdup( composer.toUtf8() ) );
    m_mtptrackhash[ track ]->composer = ( composer.isEmpty() ? qstrdup( "" ) : qstrdup( composer.toUtf8() ) );
    debug() << "Set to: " << m_mtptrackhash[ track ]->composer;
}
void
MtpHandler::libSetGenre( Meta::MediaDeviceTrackPtr &track, const QString& genre )
{
//    m_mtptrackhash[ track ]->genre = ( genre.isEmpty() ? qstrdup( i18n( "Unknown genre" ).toUtf8() ) : qstrdup( genre.toUtf8() ) );
    m_mtptrackhash[ track ]->genre = ( genre.isEmpty() ? qstrdup( "" ) : qstrdup( genre.toUtf8() ) );
    debug() << "Set to: " << m_mtptrackhash[ track ]->genre;
}
void
MtpHandler::libSetYear( Meta::MediaDeviceTrackPtr &track, const QString& year )
{
    if( year.toInt() > 0 )
    {
        QString date;
        QTextStream( &date ) << year.toInt() << "0101T0000.0";
        m_mtptrackhash[ track ]->date = qstrdup( date.toUtf8() );
    }
    else
        m_mtptrackhash[ track ]->date = qstrdup( "00010101T0000.0" );
}
void
MtpHandler::libSetLength( Meta::MediaDeviceTrackPtr &track, int length )
{
    m_mtptrackhash[ track ]->duration = ( length > 0 ? length*1000 : 0 );
}
void
MtpHandler::libSetTrackNumber( Meta::MediaDeviceTrackPtr &track, int tracknum )
{
    m_mtptrackhash[ track ]->tracknumber = tracknum;
}
void
MtpHandler::libSetComment( Meta::MediaDeviceTrackPtr &track, const QString& comment )
{
    // NOTE: defaulting, since not provided
    Q_UNUSED( track )
    Q_UNUSED( comment )
}
void
MtpHandler::libSetDiscNumber( Meta::MediaDeviceTrackPtr &track, int discnum )
{
    // NOTE: defaulting, since not provided
    Q_UNUSED( track )
    Q_UNUSED( discnum )
}
void
MtpHandler::libSetBitrate( Meta::MediaDeviceTrackPtr &track, int bitrate )
{
    m_mtptrackhash[ track ]->bitrate = bitrate;
}
void
MtpHandler::libSetSamplerate( Meta::MediaDeviceTrackPtr &track, int samplerate )
{
    m_mtptrackhash[ track ]->samplerate = samplerate;
}
void
MtpHandler::libSetBpm( Meta::MediaDeviceTrackPtr &track, float bpm )
{
    // NOTE: defaulting, since not provided
    Q_UNUSED( track )
    Q_UNUSED( bpm )
}
void
MtpHandler::libSetFileSize( Meta::MediaDeviceTrackPtr &track, int filesize )
{
    m_mtptrackhash[ track ]->filesize = filesize;
}
void
MtpHandler::libSetPlayCount( Meta::MediaDeviceTrackPtr &track, int playcount )
{
    m_mtptrackhash[ track ]->usecount = playcount;
}
void
MtpHandler::libSetLastPlayed( Meta::MediaDeviceTrackPtr &track, uint lastplayed)
{
    Q_UNUSED( track )
    Q_UNUSED( lastplayed )
}
void
MtpHandler::libSetRating( Meta::MediaDeviceTrackPtr &track, int rating )
{
    m_mtptrackhash[ track ]->rating = ( rating * 10 );
}
void
MtpHandler::libSetType( Meta::MediaDeviceTrackPtr &track, const QString& type )
{
    debug() << "filetype : " << type;
    if ( type == "mp3" )
    {
        m_mtptrackhash[ track ]->filetype = LIBMTP_FILETYPE_MP3;
    }
    else if ( type == "ogg" )
    {
        m_mtptrackhash[ track ]->filetype = LIBMTP_FILETYPE_OGG;
    }
    else if ( type == "wma" )
    {
        m_mtptrackhash[ track ]->filetype = LIBMTP_FILETYPE_WMA;
    }
    else if ( type == "mp4" )
    {
        m_mtptrackhash[ track ]->filetype = LIBMTP_FILETYPE_MP4;
    }
    else
    {
        // Couldn't recognise an Amarok filetype.
        // fallback to checking the extension (e.g. .wma, .ogg, etc)
        debug() << "No filetype found by Amarok filetype";

        const QString extension = type.toLower();

        int libmtp_type = m_supportedFiles.indexOf( extension );
        if ( libmtp_type >= 0 )
        {
            int keyIndex = mtpFileTypes.values().indexOf( extension );
            libmtp_type = mtpFileTypes.keys()[keyIndex];
            m_mtptrackhash[ track ]->filetype = ( LIBMTP_filetype_t ) libmtp_type;
            debug() << "set filetype to " << libmtp_type << " based on extension of ." << extension;
        }
        else
        {
            debug() << "We do not support the extension ." << extension;
            /*   The::statusBar()->shortLongMessage(
               genericError,
               i18n( "Cannot determine a valid file type" ),
               StatusBar::Error
                                                 );*/
        }
    }

    debug() << "Filetype set to: " << mtpFileTypes[ m_mtptrackhash[ track ]->filetype ];
}

void
MtpHandler::libSetPlayableUrl( Meta::MediaDeviceTrackPtr &destTrack, const Meta::TrackPtr &srcTrack )
{
    if( !srcTrack->playableUrl().fileName().isEmpty() )
        m_mtptrackhash[ destTrack ]->filename = qstrdup( srcTrack->playableUrl().fileName().toUtf8() );
}

void
MtpHandler::libCreateTrack( const Meta::MediaDeviceTrackPtr& track )
{
    m_mtptrackhash[ track ] = LIBMTP_new_track_t();
    m_mtptrackhash[ track ]->item_id = 0;
    m_mtptrackhash[ track ]->parent_id = m_copyparentid;
    m_mtptrackhash[ track ]->storage_id = 0; // default storage id
}

void
MtpHandler::prepareToPlay( Meta::MediaDeviceTrackPtr &track )
{
    DEBUG_BLOCK
    KUrl url;
    if( m_cachedtracks.contains( track ) )
    {
        debug() << "File is already copied, simply return";
        //m_playableUrl = KUrl::fromPath( m_playableUrl );
    }
    else
    {
        QString tempPath = setTempFile( track, libGetType( track ) );
        track->setPlayableUrl( tempPath );

        debug() << "Beginning temporary file copy";
//        m_tempfile.open();
        bool success = !(getTrackToFile( m_mtptrackhash[ track ]->item_id , track->playableUrl().path() ) );
        debug() << "File transfer complete";
        if( success )
        {
            debug() << "File transfer successful!";
            //m_playableUrl = KUrl::fromPath( m_playableUrl );
        }
        else
        {
            debug() << "File transfer failed!";
            //m_playableUrl = KUrl::fromPath( "" );
            m_cachedtracks.remove( track );
        }
    }
}

QString
MtpHandler::setTempFile( Meta::MediaDeviceTrackPtr &track, const QString &format )
{
    m_cachedtracks[ track ] = new KTemporaryFile();
    m_cachedtracks[ track ]->setSuffix( ('.' + format) ); // set suffix based on info from libmtp
    if (!m_cachedtracks[ track ]->open())
        return QString();

    QFileInfo tempFileInfo( *(m_cachedtracks[ track ]) ); // get info for path
    QString tempPath = tempFileInfo.absoluteFilePath(); // path

    m_cachedtracks[ track ]->setAutoRemove( true );

    return tempPath;
}


void
MtpHandler::slotDeviceMatchSucceeded( ThreadWeaver::Job* job )
{
    DEBUG_BLOCK

    if ( job->success() )
    {
        getDeviceInfo();
//        debug() << "Device matches serial, emitting succeeded()";
        m_memColl->slotAttemptConnectionDone( true );
    }
    else
        m_memColl->slotAttemptConnectionDone( false );
}

void
MtpHandler::slotDeviceMatchFailed( ThreadWeaver::Job* job )
{
    DEBUG_BLOCK
    debug() << "Running slot device match failed";
    disconnect( job, SIGNAL( done( ThreadWeaver::Job* ) ), this, SLOT( slotDeviceMatchSucceeded() ) );
    m_memColl->slotAttemptConnectionDone( false );
}

void
MtpHandler::updateTrack( Meta::MediaDeviceTrackPtr &track )
{
    DEBUG_BLOCK

    // pull out track struct to prepare for update

    LIBMTP_track_t *mtptrack = m_mtptrackhash[ track ];

    // commence update on device

    int failed = LIBMTP_Update_Track_Metadata( m_device, mtptrack );

    if ( !failed )
        debug() << "Metadata update succeeded!";

    else
        debug() << "Failed to update metadata";
}

/// Capability-related functions

bool
MtpHandler::hasCapabilityInterface( Handler::Capability::Type type ) const
{
    switch( type )
    {
        case Handler::Capability::Readable:
            return true;
        case Handler::Capability::Playlist:
            return true;
        case Handler::Capability::Writable:
            return true;

        default:
            return false;
    }
}

Handler::Capability*
MtpHandler::createCapabilityInterface( Handler::Capability::Type type )
{
    switch( type )
    {
        case Handler::Capability::Readable:
            return new Handler::MtpReadCapability( this );
        case Handler::Capability::Playlist:
            return new Handler::MtpPlaylistCapability( this );
        case Handler::Capability::Writable:
            return new Handler::MtpWriteCapability( this );

        default:
            return 0;
    }
}

WorkerThread::WorkerThread( int numrawdevices, LIBMTP_raw_device_t* rawdevices,  MtpHandler* handler )
        : ThreadWeaver::Job()
        , m_success( false )
        , m_numrawdevices( numrawdevices )
        , m_rawdevices( rawdevices )
        , m_handler( handler )
{
    connect( this, SIGNAL( failed( ThreadWeaver::Job* ) ), m_handler, SLOT( slotDeviceMatchFailed( ThreadWeaver::Job* ) ) );
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), m_handler, SLOT( slotDeviceMatchSucceeded( ThreadWeaver::Job* ) ) );
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), this, SLOT( deleteLater() ) );
}

WorkerThread::~WorkerThread()
{
    //nothing to do
}

bool
WorkerThread::success() const
{
    return m_success;
}

void
WorkerThread::run()
{
    m_success = m_handler->iterateRawDevices( m_numrawdevices, m_rawdevices );
}
/*
void
MtpHandler::slotCopyNextTrackFailed( ThreadWeaver::Job* job )
{
    Q_UNUSED( job );
    m_copyFailed = true;
    QString error = "Job Failed";
    m_tracksFailed.insert( m_lastTrackCopied, error );

    copyNextTrackToDevice();
}

void
MtpHandler::slotCopyNextTrackToDevice( ThreadWeaver::Job* job )
{
    if ( job->success() )
    {
        emit incrementProgress();
    }
    else
    {
        m_copyFailed = true;
        QString error = "MTP copy error";
        m_tracksFailed.insert( m_lastTrackCopied, error );
    }

    copyNextTrackToDevice();
}

CopyWorkerThread::CopyWorkerThread( const Meta::TrackPtr &track, MtpHandler* handler )
        : ThreadWeaver::Job()
        , m_success( false )
        , m_track( track )
        , m_handler( handler )
{
    connect( this, SIGNAL( failed( ThreadWeaver::Job* ) ), m_handler, SLOT( slotCopyNextTrackFailed( ThreadWeaver::Job* ) ) );
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), m_handler, SLOT( slotCopyNextTrackToDevice( ThreadWeaver::Job* ) ) );
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), this, SLOT( deleteLater() ) );
}

CopyWorkerThread::~CopyWorkerThread()
{
    //nothing to do
}

bool
CopyWorkerThread::success() const
{
    return m_success;
}

void
CopyWorkerThread::run()
{
    m_success = m_handler->privateCopyTrackToDevice( m_track );
}
*/
