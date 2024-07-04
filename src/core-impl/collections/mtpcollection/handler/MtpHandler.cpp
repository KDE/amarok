/****************************************************************************************
 * Copyright (c) 2006 Andy Kelk <andy@mopoke.co.uk>                                     *
 * Copyright (c) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "MtpHandler"

#include "MtpHandler.h"

#include "MtpCollection.h"
#include "core/support/Debug.h"

#include "core-impl/meta/file/File.h" // for KIO file handling
#include "core/logger/Logger.h"

#include <KIO/Job>
#include <KIO/DeleteJob>
#include "kjob.h"
#include <ThreadWeaver/ThreadWeaver>

#include <QFileInfo>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QUrl>

using namespace Meta;

MtpHandler::MtpHandler( Collections::MtpCollection *mc )
    : MediaDeviceHandler( mc )
    , m_device( nullptr )
    , m_capacity( 0.0 )
    , m_default_parent_folder( 0 )
    , m_folders( nullptr )
    , m_folderStructure()
    , m_format()
    , m_name()
    , m_supportedFiles()
    , m_isCanceled( false )
    , m_wait( false )
    , m_dbChanged( false )
    , m_trackcounter( 0 )
    , m_copyParentId( 0 )
    , m_tempDir( new QTemporaryDir() )
{
    DEBUG_BLOCK
    m_copyingthreadsafe = true;
    m_tempDir->setAutoRemove( true );
//    init();
}

MtpHandler::~MtpHandler()
{
    // TODO: free used memory
    DEBUG_BLOCK
    // clear folder structure

    if ( m_folders != nullptr )
    {

        LIBMTP_destroy_folder_t( m_folders );

        m_folders = nullptr;
        debug() << "Folders destroyed";
    }

    // Delete temporary files

    //delete m_tempDir;

    // release device

    if ( m_device != nullptr )
    {

        LIBMTP_Release_Device( m_device );
        /* possible race condition with statusbar destructor,
        will uncomment when fixed */
        //Amarok::Logger::longMessage(
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
    mtpFileTypes[LIBMTP_FILETYPE_WAV] = QStringLiteral("wav");
    mtpFileTypes[LIBMTP_FILETYPE_MP3] = QStringLiteral("mp3");
    mtpFileTypes[LIBMTP_FILETYPE_WMA] = QStringLiteral("wma");
    mtpFileTypes[LIBMTP_FILETYPE_OGG] = QStringLiteral("ogg");
    mtpFileTypes[LIBMTP_FILETYPE_AUDIBLE] = QStringLiteral("aa");
    mtpFileTypes[LIBMTP_FILETYPE_MP4] = QStringLiteral("mp4");
    mtpFileTypes[LIBMTP_FILETYPE_UNDEF_AUDIO] = QStringLiteral("undef-audio");
    mtpFileTypes[LIBMTP_FILETYPE_WMV] = QStringLiteral("wmv");
    mtpFileTypes[LIBMTP_FILETYPE_AVI] = QStringLiteral("avi");
    mtpFileTypes[LIBMTP_FILETYPE_MPEG] = QStringLiteral("mpg");
    mtpFileTypes[LIBMTP_FILETYPE_ASF] = QStringLiteral("asf");
    mtpFileTypes[LIBMTP_FILETYPE_QT] = QStringLiteral("mov");
    mtpFileTypes[LIBMTP_FILETYPE_UNDEF_VIDEO] = QStringLiteral("undef-video");
    mtpFileTypes[LIBMTP_FILETYPE_JPEG] = QStringLiteral("jpg");
    mtpFileTypes[LIBMTP_FILETYPE_JFIF] = QStringLiteral("jfif");
    mtpFileTypes[LIBMTP_FILETYPE_TIFF] = QStringLiteral("tiff");
    mtpFileTypes[LIBMTP_FILETYPE_BMP] = QStringLiteral("bmp");
    mtpFileTypes[LIBMTP_FILETYPE_GIF] = QStringLiteral("gif");
    mtpFileTypes[LIBMTP_FILETYPE_PICT] = QStringLiteral("pict");
    mtpFileTypes[LIBMTP_FILETYPE_PNG] = QStringLiteral("png");
    mtpFileTypes[LIBMTP_FILETYPE_VCALENDAR1] = QStringLiteral("vcs");
    mtpFileTypes[LIBMTP_FILETYPE_VCALENDAR2] = QStringLiteral("vcs");
    mtpFileTypes[LIBMTP_FILETYPE_VCARD2] = QStringLiteral("vcf");
    mtpFileTypes[LIBMTP_FILETYPE_VCARD3] = QStringLiteral("vcf");
    mtpFileTypes[LIBMTP_FILETYPE_WINDOWSIMAGEFORMAT] = QStringLiteral("wim");
    mtpFileTypes[LIBMTP_FILETYPE_WINEXEC] = QStringLiteral("exe");
    mtpFileTypes[LIBMTP_FILETYPE_TEXT] = QStringLiteral("txt");
    mtpFileTypes[LIBMTP_FILETYPE_HTML] = QStringLiteral("html");
    mtpFileTypes[LIBMTP_FILETYPE_AAC] = QStringLiteral("aac");
    mtpFileTypes[LIBMTP_FILETYPE_FLAC] = QStringLiteral("flac");
    mtpFileTypes[LIBMTP_FILETYPE_MP2] = QStringLiteral("mp3");
    mtpFileTypes[LIBMTP_FILETYPE_M4A] = QStringLiteral("m4a");
    mtpFileTypes[LIBMTP_FILETYPE_DOC] = QStringLiteral("doc");
    mtpFileTypes[LIBMTP_FILETYPE_XML] = QStringLiteral("xml");
    mtpFileTypes[LIBMTP_FILETYPE_XLS] = QStringLiteral("xls");
    mtpFileTypes[LIBMTP_FILETYPE_PPT] = QStringLiteral("ppt");
    mtpFileTypes[LIBMTP_FILETYPE_MHT] = QStringLiteral("mht");
    mtpFileTypes[LIBMTP_FILETYPE_JP2] = QStringLiteral("jpg");
    mtpFileTypes[LIBMTP_FILETYPE_JPX] = QStringLiteral("jpx");
    mtpFileTypes[LIBMTP_FILETYPE_UNKNOWN] = QStringLiteral("unknown");

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
            debug() << "No raw devices found.";
            m_success = false;
            break;

        case LIBMTP_ERROR_CONNECTING:
            debug() << "Detect: There has been an error connecting.";
            m_success = false;
            break;

        case LIBMTP_ERROR_MEMORY_ALLOCATION:
            debug() << "Detect: Encountered a Memory Allocation Error. Exiting";
            m_success = false;
            break;

        case LIBMTP_ERROR_NONE:
            m_success = true;
            break;

        default:
            debug() << "Unhandled mtp error";
            m_success = false;
            break;
    }

    if ( m_success )
    {
        debug() << "Got mtp list, connecting to device using thread";
        ThreadWeaver::Queue::instance()->enqueue( QSharedPointer<ThreadWeaver::Job>(new WorkerThread( numrawdevices, rawdevices, this )) );
    }
    else
    {
        free( rawdevices );
    //        Q_EMIT failed();
    }
}


// this function is threaded
bool
MtpHandler::iterateRawDevices( int numrawdevices, LIBMTP_raw_device_t* rawdevices )
{
    DEBUG_BLOCK

    bool success = false;

    LIBMTP_mtpdevice_t *device = nullptr;
    // test raw device for connectability
    for ( int i = 0; i < numrawdevices; i++ )
    {
            debug() << "Opening raw device number: " << ( i + 1 );
            device = LIBMTP_Open_Raw_Device( &rawdevices[i] );
            if ( device == nullptr )
            {
                debug() << "Unable to open raw device: " << ( i + 1 );
                success = false;
                continue;
            }

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

    if ( m_device == nullptr )
    {
        // TODO: error protection
        success = false;
        free( rawdevices );
    }

    //QString serial = QString::fromUtf8( LIBMTP_Get_Serialnumber( m_device ) );

    //    debug() << "Serial is: " << serial;

    return success;
}

void
MtpHandler::getDeviceInfo()
{
    DEBUG_BLOCK

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
        m_capacity = m_device->storage->MaxCapacity;
    }

    QString modelname = QLatin1String( LIBMTP_Get_Modelname( m_device ) );

    // NOTE: on next libmtp bump, may reintroduce owner name
    // for now it doesn't work as expected
    /*
    QString ownername = QString( LIBMTP_Get_Friendlyname( m_device ) );
    m_name = modelname;
    if(! ownername.isEmpty() )
        if( modelname != ownername )
            m_name += " (" + ownername + QLatin1Char(')');

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
            debug() << "Device supports: " << mtpFileTypes.value( filetypes[ i ] );
            m_supportedFiles << mtpFileTypes.value( filetypes[ i ] );
        }
    }
    // find supported image types (for album art).
    if ( m_supportedFiles.indexOf( QStringLiteral("jpg") ) )
        m_format = QStringLiteral("JPEG");
    else if ( m_supportedFiles.indexOf( QStringLiteral("png") ) )
        m_format = QStringLiteral("PNG");
    else if ( m_supportedFiles.indexOf( QStringLiteral("gif") ) )
        m_format = QStringLiteral("GIF");
    free( filetypes );
}

void
MtpHandler::terminate()
{
    DEBUG_BLOCK
    // clear folder structure
    if ( m_folders != nullptr )
    {
        LIBMTP_destroy_folder_t( m_folders );

        m_folders = nullptr;
    }

    // release device
    if ( m_device != nullptr )
    {
        LIBMTP_Release_Device( m_device );
        /* possible race condition with statusbar destructor,
        will uncomment when fixed
        Amarok::Logger::longMessage(
                                    i18n( "The MTP device %1 has been disconnected", prettyName() ),
                                    Amarok::Logger::Information
                            ); */

        debug() << "Device released";
    }
}

void
MtpHandler::getCopyableUrls( const Meta::TrackList &tracks )
{
    DEBUG_BLOCK

    QMap<Meta::TrackPtr,  QUrl> urls;

    QString genericError = i18n( "Could not copy track from device." );

    for( Meta::TrackPtr trackptr :  tracks )
    {
        Meta::MediaDeviceTrackPtr track = Meta::MediaDeviceTrackPtr::dynamicCast( trackptr );
        if( !track )
            break;

        QString trackFileName = QString::fromUtf8( m_mtpTrackHash.value( track )->filename );

        QString filename = m_tempDir->path() + QLatin1Char('/') + trackFileName;

        debug() << "Temp Filename: " << filename;

        int ret = getTrackToFile( m_mtpTrackHash.value( track )->item_id, filename );
        if (  ret != 0 )
        {
            debug() << "Get Track failed: " << ret;
            /*Amarok::Components::logger()->shortLongMessage(
                genericError,
                i18n( "Could not copy track from device." ),
                StatusBar::Error
                );*/
        }
        else
        {
            urls.insert( trackptr, QUrl::fromLocalFile( filename ) );
        }
    }

    Q_EMIT gotCopyableUrls( urls );
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
    QStringList folders = m_folderStructure.split( QLatin1Char('/') ); // use slash as a dir separator
    QString completePath;
    for ( QStringList::Iterator it = folders.begin(); it != folders.end(); ++it )
    {
        if (( *it ).isEmpty() )
            continue;
        // substitute %a , %b , %g
        ( *it ).replace( QRegularExpression( QStringLiteral("%a") ), artistName )
        .replace( QRegularExpression( QStringLiteral("%b") ), albumName )
        .replace( QRegularExpression( QStringLiteral("%g") ), genreName );
        // check if it exists
        uint32_t check_folder = subfolderNameToID(( *it ).toUtf8().constData(), m_folders, parent_id );
        // create if not exists (if requested)
        if ( check_folder == 0 )
        {
            if ( create )
            {
                check_folder = createFolder(( *it ).toUtf8().constData() , parent_id );
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
        completePath += ( *it ) + QLatin1Char('/');
        // set new parent
        parent_id = check_folder;
    }
    debug() << "Folder path : " << completePath;
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
    else if ( m_folders != nullptr )
    {
        parent_id = folderNameToID( qstrdup( QStringLiteral( "Music" ).toUtf8().constData() ), m_folders );
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

    if ( folderlist == nullptr )
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

    if ( folderlist == nullptr )
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
    m_folders = nullptr;
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
        Amarok::Logger::longMessage( i18n( "Delete failed" ),
                                                   Amarok::Logger::Error
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
    return LIBMTP_Get_Track_To_File( m_device, id, filename.toUtf8().constData(), nullptr, nullptr );
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
            /*Amarok::Components::logger()->shortLongMessage(
            genericError,
            i18n( "Cannot create parent folder. Check your structure." ),
            Amarok::Logger::Error
                                              );*/
            return;
        }
    }
    else
    {
        parent_id = getDefaultParentId();
    }
    debug() << "Parent id : " << parent_id;

    m_copyParentId = parent_id;

}

bool
MtpHandler::libCopyTrack( const Meta::TrackPtr &srcTrack, Meta::MediaDeviceTrackPtr &destTrack )
{
    DEBUG_BLOCK

    findPathToCopy( srcTrack, destTrack );
    debug() << "sending...";

    debug() << "Playable Url is: " << srcTrack->playableUrl();
    debug() << "Sending file with path: " << srcTrack->playableUrl().path().toUtf8();



    int ret = LIBMTP_Send_Track_From_File( m_device, qstrdup( srcTrack->playableUrl().path().toUtf8().constData() ), m_mtpTrackHash.value( destTrack ),
                                           nullptr, nullptr );

    debug() << "sent";
//    Q_EMIT canCopyMoreTracks();

//    Q_EMIT libCopyTrackDone( srcTrack );

    return ( ret == 0 );
}

bool
MtpHandler::libDeleteTrackFile( const Meta::MediaDeviceTrackPtr &track )
{
    slotFinalizeTrackRemove( Meta::TrackPtr::staticCast( track ) );
    return true;
}

void
MtpHandler::libDeleteTrack( const Meta::MediaDeviceTrackPtr &track )
{
    DEBUG_BLOCK
    LIBMTP_track_struct *mtptrack = m_mtpTrackHash.value( track );

    m_mtpTrackHash.remove( track );

    quint32 object_id = mtptrack->item_id;

    const QString genericError = i18n( "Could not delete item" );

    int status = LIBMTP_Delete_Object( m_device, object_id );

    removeNextTrackFromDevice();

    if( status != 0 )
        debug() << "delete object failed";
    else
        debug() << "object deleted";
}

void
MtpHandler::setDatabaseChanged()
{
    m_dbChanged = true;
}


void
MtpHandler::prepareToParseTracks()
{
    DEBUG_BLOCK

    m_currentTrackList = LIBMTP_Get_Tracklisting_With_Callback( m_device, nullptr, this );
}

bool
MtpHandler::isEndOfParseTracksList()
{
    return m_currentTrackList ? false : true;
}

void
MtpHandler::prepareToParseNextTrack()
{
    m_currentTrackList = m_currentTrackList->next;
}

void
MtpHandler::nextTrackToParse()
{
    m_currentTrack = m_currentTrackList;
}

/// Playlist Parsing

void
MtpHandler::prepareToParsePlaylists()
{
    m_currentPlaylistList = LIBMTP_Get_Playlist_List( m_device );
}


bool
MtpHandler::isEndOfParsePlaylistsList()
{
    return (m_currentPlaylistList == nullptr);
}


void
MtpHandler::prepareToParseNextPlaylist()
{
    m_currentPlaylistList = m_currentPlaylistList->next;
}


void
MtpHandler::nextPlaylistToParse()
{
    m_currentPlaylist = m_currentPlaylistList;
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
    return (m_trackcounter >= m_currentPlaylist->no_tracks);
}


void
MtpHandler::prepareToParseNextPlaylistTrack()
{
    m_trackcounter++;
}


void
MtpHandler::nextPlaylistTrackToParse()
{
    m_currentTrack = m_idTrackHash.value( m_currentPlaylist->tracks[ m_trackcounter ] );
}


Meta::MediaDeviceTrackPtr
MtpHandler::libGetTrackPtrForTrackStruct()
{
    return m_mtpTrackHash.key( m_currentTrack );
}

QString
MtpHandler::libGetPlaylistName()
{
    return QString::fromUtf8( m_currentPlaylist->name );
}

void
MtpHandler::setAssociatePlaylist( const Playlists::MediaDevicePlaylistPtr &playlist )
{
    m_mtpPlaylisthash[ playlist ] = m_currentPlaylist;
}

void
MtpHandler::libSavePlaylist( const Playlists::MediaDevicePlaylistPtr &playlist, const QString& name )
{
    DEBUG_BLOCK
    Meta::TrackList tracklist = const_cast<Playlists::MediaDevicePlaylistPtr&> ( playlist )->tracks();
    // Make new playlist

    LIBMTP_playlist_t *metadata = LIBMTP_new_playlist_t();
    metadata->name = qstrdup( name.toUtf8().constData() );
    const int trackCount = tracklist.count();
    if( trackCount > 0 )
    {
        uint32_t *tracks = ( uint32_t* )malloc( sizeof( uint32_t ) * trackCount );
        uint32_t i = 0;
        for( Meta::TrackPtr trk : tracklist )
        {
            if( !trk ) // playlists might contain invalid tracks. see BUG: 297816
                continue;
            Meta::MediaDeviceTrackPtr track = Meta::MediaDeviceTrackPtr::staticCast( trk );
            tracks[i] = m_mtpTrackHash.value( track )->item_id;
        }
        metadata->tracks = tracks;
        metadata->no_tracks = trackCount;
    }
    else
    {
        debug() << "no tracks available for playlist " << metadata->name;
        metadata->no_tracks = 0;
    }

    QString genericError = i18n( "Could not save playlist." );

    debug() << "creating new playlist : " << metadata->name << Qt::endl;
    int ret = LIBMTP_Create_New_Playlist( m_device, metadata );
    if( ret == 0 )
    {
        m_mtpPlaylisthash[ playlist ] = metadata;
        debug() << "playlist saved : " << metadata->playlist_id << Qt::endl;
    }
    else
        debug () << "Could not create new playlist on device.";
}

void
MtpHandler::deletePlaylist( const Playlists::MediaDevicePlaylistPtr &playlist )
{
    DEBUG_BLOCK

    LIBMTP_playlist_t *pl = m_mtpPlaylisthash.value( playlist );

    if( pl )
    {

        m_mtpPlaylisthash.remove( playlist );

        quint32 object_id = pl->playlist_id;

        QString genericError = i18n( "Could not delete item" );

        debug() << "delete this id : " << object_id;

        int status = LIBMTP_Delete_Object( m_device, object_id );

        if ( status != 0 )
        {
            debug() << "delete object failed";
        }
        else
            debug() << "object deleted";
    }
}

void
MtpHandler::renamePlaylist( const Playlists::MediaDevicePlaylistPtr &playlist )
{
    DEBUG_BLOCK
    LIBMTP_playlist_t *pl = m_mtpPlaylisthash.value( playlist );

    if( pl )
    {
        debug() << "updating playlist : " << pl->name << Qt::endl;
        int ret = LIBMTP_Update_Playlist( m_device, pl );
        if( ret != 0 )
        {
            debug() << "Could not rename playlist";
        }
        else
            debug() << "Playlist renamed";
    }
}

void
MtpHandler::setAssociateTrack( const Meta::MediaDeviceTrackPtr track )
{
    m_mtpTrackHash[ track ] = m_currentTrack;
    m_idTrackHash[ m_currentTrack->item_id ] = m_currentTrack;
}

QStringList
MtpHandler::supportedFormats()
{
    return m_supportedFiles;
}


QString
MtpHandler::libGetTitle( const Meta::MediaDeviceTrackPtr &track )
{
    return QString::fromUtf8( m_mtpTrackHash.value( track )->title );
}

QString
MtpHandler::libGetAlbum( const Meta::MediaDeviceTrackPtr &track )
{
    return QString::fromUtf8( m_mtpTrackHash.value( track )->album );
}

QString
MtpHandler::libGetArtist( const Meta::MediaDeviceTrackPtr &track )
{
    return QString::fromUtf8( m_mtpTrackHash.value( track )->artist );
}

QString
MtpHandler::libGetAlbumArtist( const Meta::MediaDeviceTrackPtr &track )
{
    //Album artist isn't supported by libmtp ATM.
    Q_UNUSED( track )
    return QString();
}

QString
MtpHandler::libGetComposer( const Meta::MediaDeviceTrackPtr &track )
{
    return QString::fromUtf8( m_mtpTrackHash.value( track )->composer );
}

QString
MtpHandler::libGetGenre( const Meta::MediaDeviceTrackPtr &track )
{
    return QString::fromUtf8( m_mtpTrackHash.value( track )->genre );
}

int
MtpHandler::libGetYear( const Meta::MediaDeviceTrackPtr &track )
{
    return QString::fromUtf8( m_mtpTrackHash.value( track )->date ).left( 4 ).toUInt();
}

qint64
MtpHandler::libGetLength( const Meta::MediaDeviceTrackPtr &track )
{
    if ( m_mtpTrackHash.value( track )->duration > 0 )
        return ( ( m_mtpTrackHash.value( track )->duration ) );
    return 0;
}

int
MtpHandler::libGetTrackNumber( const Meta::MediaDeviceTrackPtr &track )
{
    return m_mtpTrackHash.value( track )->tracknumber;
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
    return m_mtpTrackHash.value( track )->bitrate;
}

int
MtpHandler::libGetSamplerate( const Meta::MediaDeviceTrackPtr &track )
{
    return m_mtpTrackHash.value( track )->samplerate;
}

qreal
MtpHandler::libGetBpm( const Meta::MediaDeviceTrackPtr &track )
{
    Q_UNUSED( track );
    // NOTE: defaulting, since not provided
    return 0.0;
}
int
MtpHandler::libGetFileSize( const Meta::MediaDeviceTrackPtr &track )
{
    return m_mtpTrackHash.value( track )->filesize;
}
int
MtpHandler::libGetPlayCount( const Meta::MediaDeviceTrackPtr &track )
{
    return m_mtpTrackHash.value( track )->usecount;
}

QDateTime
MtpHandler::libGetLastPlayed( const Meta::MediaDeviceTrackPtr &track )
{
    Q_UNUSED( track );
    // NOTE: defaulting, since not provided
    return QDateTime();
}

// TODO: implement rating
int
MtpHandler::libGetRating( const Meta::MediaDeviceTrackPtr &track )
{
    return ( m_mtpTrackHash.value( track )->rating / 10 );
}
QString
MtpHandler::libGetType( const Meta::MediaDeviceTrackPtr &track )
{
    return mtpFileTypes.value( m_mtpTrackHash.value( track )->filetype );
}

QUrl
MtpHandler::libGetPlayableUrl( const Meta::MediaDeviceTrackPtr &track )
{
    Q_UNUSED( track )
    // NOTE: not a real url, using for unique key for qm
    return QUrl( QString::number(  m_mtpTrackHash.value( track )->item_id,  10 ) );
}

float
MtpHandler::totalCapacity() const
{
    DEBUG_BLOCK
    return m_capacity;
}

float
MtpHandler::usedCapacity() const
{
    DEBUG_BLOCK
    if( LIBMTP_Get_Storage( m_device, LIBMTP_STORAGE_SORTBY_NOTSORTED ) != 0 )
    {
        debug() << "Failed to get storage properties, cannot get capacity";
        return 0.0;
    }
    return ( totalCapacity() - m_device->storage->FreeSpaceInBytes );
}

/// Sets

void
MtpHandler::libSetTitle( Meta::MediaDeviceTrackPtr& track, const QString& title )
{
    m_mtpTrackHash.value( track )->title = ( title.isEmpty() ? qstrdup( "" ) : qstrdup( title.toUtf8().constData() ) );
    debug() << "Set to: " << m_mtpTrackHash.value( track )->title;
}
void
MtpHandler::libSetAlbum( Meta::MediaDeviceTrackPtr &track, const QString& album )
{
    m_mtpTrackHash.value( track )->album = ( album.isEmpty() ? qstrdup( "" ) : qstrdup( album.toUtf8().constData() ) );
    debug() << "Set to: " << m_mtpTrackHash.value( track )->album;
}
void
MtpHandler::libSetArtist( Meta::MediaDeviceTrackPtr &track, const QString& artist )
{
    m_mtpTrackHash.value( track )->artist = ( artist.isEmpty() ? qstrdup( "" ) : qstrdup( artist.toUtf8().constData() ) );
    debug() << "Set to: " << m_mtpTrackHash.value( track )->artist;
}

void
MtpHandler::libSetAlbumArtist( MediaDeviceTrackPtr &track, const QString &albumArtist )
{
    //Album artist isn't supported by libmtp ATM.
    Q_UNUSED( track )
    Q_UNUSED( albumArtist )
}

void
MtpHandler::libSetComposer( Meta::MediaDeviceTrackPtr &track, const QString& composer )
{
    m_mtpTrackHash.value( track )->composer = ( composer.isEmpty() ? qstrdup( "" ) : qstrdup( composer.toUtf8().constData() ) );
    debug() << "Set to: " << m_mtpTrackHash.value( track )->composer;
}
void
MtpHandler::libSetGenre( Meta::MediaDeviceTrackPtr &track, const QString& genre )
{
    m_mtpTrackHash.value( track )->genre = ( genre.isEmpty() ? qstrdup( "" ) : qstrdup( genre.toUtf8().constData() ) );
    debug() << "Set to: " << m_mtpTrackHash.value( track )->genre;
}
void
MtpHandler::libSetYear( Meta::MediaDeviceTrackPtr &track, const QString& year )
{
    if( year.toInt() > 0 )
    {
        QString date;
        QTextStream( &date ) << year.toInt() << "0101T0000.0";
        m_mtpTrackHash.value( track )->date = qstrdup( date.toUtf8().constData() );
    }
    else
        m_mtpTrackHash.value( track )->date = qstrdup( "00010101T0000.0" );
}
void
MtpHandler::libSetLength( Meta::MediaDeviceTrackPtr &track, int length )
{
    m_mtpTrackHash.value( track )->duration = ( length > 0 ? length : 0 );
}
void
MtpHandler::libSetTrackNumber( Meta::MediaDeviceTrackPtr &track, int tracknum )
{
    m_mtpTrackHash.value( track )->tracknumber = tracknum;
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
    m_mtpTrackHash.value( track )->bitrate = bitrate;
}
void
MtpHandler::libSetSamplerate( Meta::MediaDeviceTrackPtr &track, int samplerate )
{
    m_mtpTrackHash.value( track )->samplerate = samplerate;
}
void
MtpHandler::libSetBpm( Meta::MediaDeviceTrackPtr &track, qreal bpm )
{
    // NOTE: defaulting, since not provided
    Q_UNUSED( track )
    Q_UNUSED( bpm )
}
void
MtpHandler::libSetFileSize( Meta::MediaDeviceTrackPtr &track, int filesize )
{
    m_mtpTrackHash.value( track )->filesize = filesize;
}
void
MtpHandler::libSetPlayCount( Meta::MediaDeviceTrackPtr &track, int playcount )
{
    m_mtpTrackHash.value( track )->usecount = playcount;
}

void
MtpHandler::libSetLastPlayed( Meta::MediaDeviceTrackPtr &track, const QDateTime &lastplayed)
{
    Q_UNUSED( track )
    Q_UNUSED( lastplayed )
}
void
MtpHandler::libSetRating( Meta::MediaDeviceTrackPtr &track, int rating )
{
    m_mtpTrackHash.value( track )->rating = ( rating * 10 );
}
void
MtpHandler::libSetType( Meta::MediaDeviceTrackPtr &track, const QString& type )
{
    debug() << "filetype : " << type;
    if ( type == QStringLiteral("mp3") )
    {
        m_mtpTrackHash.value( track )->filetype = LIBMTP_FILETYPE_MP3;
    }
    else if ( type == QStringLiteral("ogg") )
    {
        m_mtpTrackHash.value( track )->filetype = LIBMTP_FILETYPE_OGG;
    }
    else if ( type == QStringLiteral("wma") )
    {
        m_mtpTrackHash.value( track )->filetype = LIBMTP_FILETYPE_WMA;
    }
    else if ( type == QStringLiteral("mp4") )
    {
        m_mtpTrackHash.value( track )->filetype = LIBMTP_FILETYPE_MP4;
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
            m_mtpTrackHash.value( track )->filetype = ( LIBMTP_filetype_t ) libmtp_type;
            debug() << "set filetype to " << libmtp_type << " based on extension of ." << extension;
        }
        else
        {
            debug() << "We do not support the extension ." << extension;
            /*   Amarok::Components::logger()->shortLongMessage(
               genericError,
               i18n( "Cannot determine a valid file type" ),
               Amarok::Logger::Error
                                                 );*/
        }
    }

    debug() << "Filetype set to: " << mtpFileTypes.value( m_mtpTrackHash.value( track )->filetype );
}

void
MtpHandler::libSetPlayableUrl( Meta::MediaDeviceTrackPtr &destTrack, const Meta::TrackPtr &srcTrack )
{
    if( !srcTrack->playableUrl().fileName().isEmpty() )
        m_mtpTrackHash.value( destTrack )->filename = qstrdup( srcTrack->playableUrl().fileName().toUtf8().constData() );
}

void
MtpHandler::libCreateTrack( const Meta::MediaDeviceTrackPtr& track )
{
    m_mtpTrackHash[ track ] = LIBMTP_new_track_t();
    m_mtpTrackHash.value( track )->item_id = 0;
    m_mtpTrackHash.value( track )->parent_id = m_copyParentId;
    m_mtpTrackHash.value( track )->storage_id = 0; // default storage id
}

void
MtpHandler::prepareToPlay( Meta::MediaDeviceTrackPtr &track )
{
    DEBUG_BLOCK
    QUrl url;
    if( m_cachedTracks.contains( track ) )
    {
        debug() << "File is already copied, simply return";
        //m_playableUrl = QUrl::fromLocalFile( m_playableUrl );
    }
    else
    {
        QString tempPath = setTempFile( track, libGetType( track ) );
        track->setPlayableUrl( QUrl::fromLocalFile( tempPath ) );

        debug() << "Beginning temporary file copy";
//        m_tempfile.open();
        bool success = !(getTrackToFile( m_mtpTrackHash.value( track )->item_id , track->playableUrl().path() ) );
        debug() << "File transfer complete";
        if( success )
        {
            debug() << "File transfer successful!";
            //m_playableUrl = QUrl::fromLocalFile( m_playableUrl );
        }
        else
        {
            debug() << "File transfer failed!";
            //m_playableUrl = QUrl::fromLocalFile( "" );
            m_cachedTracks.remove( track );
        }
    }
}

QString
MtpHandler::setTempFile( Meta::MediaDeviceTrackPtr &track, const QString &format )
{
    m_cachedTracks[ track ] = new QTemporaryFile();
    m_cachedTracks.value( track )->setFileTemplate( QDir::tempPath() + QStringLiteral("/XXXXXX.") + format ); // set suffix based on info from libmtp
    if (!m_cachedTracks.value( track )->open())
        return QString();

    QFileInfo tempFileInfo( *(m_cachedTracks.value( track ) ) ); // get info for path
    QString tempPath = tempFileInfo.absoluteFilePath(); // path

    m_cachedTracks.value( track )->setAutoRemove( true );

    return tempPath;
}


void
MtpHandler::slotDeviceMatchSucceeded( ThreadWeaver::JobPointer job )
{
    DEBUG_BLOCK
    if( !m_memColl ) // try to fix BUG:279966
        return;

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
MtpHandler::slotDeviceMatchFailed( ThreadWeaver::JobPointer job )
{
    DEBUG_BLOCK
    if( !m_memColl ) // try to fix BUG:279966
        return;

    debug() << "Running slot device match failed";
    disconnect( job.dynamicCast<WorkerThread>().data(), &WorkerThread::done, this, &MtpHandler::slotDeviceMatchSucceeded );
    m_memColl->slotAttemptConnectionDone( false );
}

void
MtpHandler::updateTrack( Meta::MediaDeviceTrackPtr &track )
{
    DEBUG_BLOCK

    // pull out track struct to prepare for update

    LIBMTP_track_t *mtptrack = m_mtpTrackHash.value( track );

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
            return nullptr;
    }
}

WorkerThread::WorkerThread( int numrawdevices, LIBMTP_raw_device_t* rawdevices,  MtpHandler* handler )
        : QObject()
        , ThreadWeaver::Job()
        , m_success( false )
        , m_numrawdevices( numrawdevices )
        , m_rawdevices( rawdevices )
        , m_handler( handler )
{
    connect( this, &WorkerThread::failed, m_handler, &Meta::MtpHandler::slotDeviceMatchFailed );
    connect( this, &WorkerThread::done, m_handler, &Meta::MtpHandler::slotDeviceMatchSucceeded );
    connect( this, &WorkerThread::done, this, &WorkerThread::deleteLater );
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
WorkerThread::run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread)
{
    Q_UNUSED(self);
    Q_UNUSED(thread);
    m_success = m_handler->iterateRawDevices( m_numrawdevices, m_rawdevices );
}

void
WorkerThread::defaultBegin(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread)
{
    Q_EMIT started(self);
    ThreadWeaver::Job::defaultBegin(self, thread);
}

void
WorkerThread::defaultEnd(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread)
{
    ThreadWeaver::Job::defaultEnd(self, thread);
    if (!self->success()) {
        Q_EMIT failed(self);
    }
    Q_EMIT done(self);
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
        Q_EMIT incrementProgress();
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
    connect( this, SIGNAL(failed(ThreadWeaver::Job*)), m_handler, SLOT(slotCopyNextTrackFailed(ThreadWeaver::Job*)) );
    connect( this, SIGNAL(done(ThreadWeaver::Job*)), m_handler, SLOT(slotCopyNextTrackToDevice(ThreadWeaver::Job*)) );
    connect( this, SIGNAL(done(ThreadWeaver::Job*)), this, &QObject::deleteLater );
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
