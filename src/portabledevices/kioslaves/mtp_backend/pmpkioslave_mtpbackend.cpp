/*
 *  Copyright (c) 2007 Jeff Mitchell <kde-dev@emailgoeshere.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "pmpkioslave_mtpbackend.h"
#include "pmpkioslave.h"

#include <sys/stat.h>

#include <QCoreApplication>
#include <QString>
#include <QTemporaryFile>
#include <QVariant>

#include <kapplication.h>
#include <kcomponentdata.h>
#include <kdebug.h>
#include <klocale.h>
#include <kurl.h>
#include <kio/slavebase.h>
#include <solid/device.h>
#include <solid/genericinterface.h>

MTPBackend::MTPBackend( PMPProtocol* slave, const Solid::Device &device )
            : PMPBackend( slave, device )
            , m_device( 0 )
            , m_trackList( 0 )
            , m_folderList( 0 )
            , m_gotMusicListing( false )
            , m_defaultMusicLocation( "Music" )
{
    kDebug() << "Creating MTPBackend" << endl;

    if( !m_slave->mtpInitialized() )
        LIBMTP_Init();

    if( LIBMTP_Get_Connected_Devices( &m_deviceList ) != LIBMTP_ERROR_NONE )
    {
        m_slave->error( KIO::ERR_INTERNAL, i18n( "Could not get a connected device list from libmtp. Possibly you are trying to run two copies of this kioslave at once, which is not supported." ) );
        return;
    }
    quint32 deviceCount = LIBMTP_Number_Devices_In_List( m_deviceList );
    if( deviceCount == 0 )
    {
        m_slave->error( KIO::ERR_INTERNAL, i18n( "libmtp found no devices." ) );
        return;
    }
}

MTPBackend::~MTPBackend()
{
    kDebug() << endl << "In MTPBackend destructor, releasing device" << endl << endl;
    if( m_device )
        LIBMTP_Release_Device( m_device );
}

void
MTPBackend::initialize()
{
    kDebug() << "Initializing MTPBackend for device " << m_solidDevice.udi() << endl;
    Solid::GenericInterface *gi = m_solidDevice.as<Solid::GenericInterface>();
    if( !gi )
    {
        m_slave->error( KIO::ERR_INTERNAL, i18n( "Error getting a GenericInterface to the device from Solid." ) );
        return;
    }
    if( !gi->propertyExists( "usb.serial" ) )
    {
        m_slave->error( KIO::ERR_INTERNAL, i18n( "Could not find serial number of MTP device in HAL. \
                                                  When filing a bug please include the full output of \"lshal -lt\"." ) );
        return;
    }
    QVariant possibleSerial = gi->property( "usb.serial" );
    if( !possibleSerial.isValid() || possibleSerial.isNull() || !possibleSerial.canConvert( QVariant::String ) )
    {
        m_slave->error( KIO::ERR_INTERNAL, i18n( "Could not get the serial number from HAL." ) );
        return;
    }
    QString serial = possibleSerial.toString();
    kDebug() << endl << endl << "Case-insensitively looking for serial number starting with: " << serial << endl << endl;
    LIBMTP_mtpdevice_t *currdevice;
    for( currdevice = m_deviceList; currdevice != NULL; currdevice = currdevice->next )
    {
        kDebug() << "currdevice serial number = " << LIBMTP_Get_Serialnumber( currdevice ) << endl;
        //WARNING: a startsWith is done below, as the value reported by HAL for the serial number seems to be about half
        //the length of the value reported by libmtp...is this always true?  Could this cause two devices to
        //be recognized as the same one?
        if( QString( LIBMTP_Get_Serialnumber( currdevice ) ).startsWith( serial, Qt::CaseInsensitive ) )
        {
            kDebug() << endl << endl << "Found a matching serial!" << endl << endl;
            m_device = currdevice;
        }
        else
            LIBMTP_Release_Device( currdevice );
    }

    if( m_device )
        kDebug() << "FOUND THE MTP DEVICE WE WERE LOOKING FOR!" << endl;

    return;
}

QString
MTPBackend::getFriendlyName() const
{
    kDebug() << "Getting MTPBackend friendly name" << endl;
    QString friendlyName = QString::fromUtf8( LIBMTP_Get_Friendlyname( m_device ) );
    kDebug() << "Found MTPBackend friendly name = " << friendlyName << endl;
    return friendlyName;
}

void
MTPBackend::setFriendlyName( const QString &name )
{
    kDebug() << "Setting MTPBackend friendly name" << endl;
    if( LIBMTP_Set_Friendlyname( m_device, name.toUtf8() ) != 0 )
        m_slave->warning( i18n( "Failed to set friendly name on the device!" ) );
}

QString
MTPBackend::getModelName() const
{
    kDebug() << "Getting MTPBackend model name" << endl;
    QString modelName = QString::fromUtf8( LIBMTP_Get_Modelname( m_device ) );
    kDebug() << "Found MTPBackend model name = " << modelName << endl;
    return modelName;
}

void
MTPBackend::del( const KUrl &url, bool isfile )
{
    QString path = getFilePath( url );
    if( getObjectType( url ) == MTPBackend::TRACK )
    {
        delMusic( path, isfile );
        m_slave->listEntry( KIO::UDSEntry(), true );
    }
    else
        m_slave->error( KIO::ERR_INTERNAL, i18n( "Invalid path requested!" ) );

}

void
MTPBackend::delMusic( const QString &path, bool isfile )
{
    if( !isfile && m_pathToFolderIdHash.contains( path ) )
    {
        //TODO:recursively delete children
    }
    else if( isfile && m_pathToTrackIdHash.contains( path ) )
    {
        if( !LIBMTP_Delete_Object( m_device, m_pathToTrackIdHash.value( path ) ) )
        {
            m_slave->error( KIO::ERR_INTERNAL, i18n( "libmtp reported failure deleting %1", path ) );
            return;
        }
        quint32 trackid = m_pathToTrackIdHash.value( path );
        m_pathToTrackIdHash.remove( path );
        m_trackParentToPtrHash.remove( path.left( path.lastIndexOf( '/' ) ), (LIBMTP_track_t*)(m_idToPtrHash.value( trackid )) );
        m_idToPtrHash.remove( trackid );
        LIBMTP_track_t *behind, *curr = m_trackList;
        while( curr && curr->item_id != trackid )
        {
            behind = curr;
            curr = curr->next;
        }
        if( curr->item_id == trackid )
        {
            behind->next = curr->next;
            LIBMTP_destroy_track_t( curr );
        }
    }
    else
        m_slave->error( KIO::ERR_INTERNAL, i18n( "Could not find the proper path to delete!" ) );
}

void
MTPBackend::get( const KUrl &url )
{
    kDebug() << "in MTPBackend::get, url is: " << url << endl;
    int type = getObjectType( url );
    if( type == MTPBackend::TRACK )
    {
        QTemporaryFile tf;
        if( !tf.open() )
        {
            kDebug() << "in MTPBackend::get, failed to open a temporary file!";
            return;
        }
        quint32 id = getUIDFromFilename( url );
        if( id == 0 )
        {
            kDebug() << "in MTPBackend::get, failed to get UID from filename!";
            return;
        }
        int retval = LIBMTP_Get_Track_To_File_Descriptor( m_device, id, tf.handle(), 0, 0 );
        if( retval != 0 )
        {
            kDebug() << "in MTPBackend::get, value returned from LIBMTP_Get_Track_To_File_Descriptor is not zero, it is " << retval;
            return;
        }
        qint64 curr = 0;
        const qint64 readsize = 4194304; //4MB at a time
        for( ; curr + readsize < tf.size(); curr += readsize )
        {
            QByteArray qb = tf.read( readsize );
            if( qb.size() != readsize )
            {
                kDebug() << "in MTPBackend::get, bytearray size is not readsize, it is " << qb.size();
                return;
            }
            emit m_slave->data( qb );
        }
        qint64 left = tf.size() - curr;
        if( left )
        {
            QByteArray qb = tf.read( left );
            if( qb.size() != left )
            {
                kDebug() << "in MTPBackend::get, bytearray size is not left, it is " << qb.size();
                return;
            }
            emit m_slave->data( qb );
        }
        emit m_slave->data( QByteArray() );
    }
    if( type == MTPBackend::UNKNOWN )
        kDebug() << "in MTPBackend::get, type returned is UNKNOWN";
}

void
MTPBackend::listDir( const KUrl &url )
{
    QString path = getFilePath( url );
    kDebug() << "in MTPBackend::listDir, path is: " << path << endl;
    //first case: no specific folder chosen, display a list of available actions as folders
    if( path.isEmpty() )
    {
        LIBMTP_folder_t *folderList = LIBMTP_Get_Folder_List( m_device );
        if( folderList == 0 )
        {
            m_slave->warning( i18n( "Could not find any folders on device!" ) );
            return;
        }
        while( folderList )
        {
            KIO::UDSEntry entry;
            entry.insert( KIO::UDSEntry::UDS_NAME, QString::fromUtf8( folderList->name ) );
            KUrl url( getUrlPrefix() );
            QString extraPath = QString::number( folderList->folder_id ) + "_###_" + QString::fromUtf8( folderList->name );
            url.addPath( extraPath );
            entry.insert( KIO::UDSEntry::UDS_URL, url.url() );
            entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
            entry.insert( KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IRGRP | S_IROTH );
            m_slave->listEntry( entry, false );
            folderList = folderList->sibling;
        }
        m_slave->listEntry( KIO::UDSEntry(), true );
        return;
    }
    if( !m_gotMusicListing )
        buildMusicListing();
    //next case: User requests something specific...first, find out what they requested
    //and error if not appropriate
    if( getObjectType( url ) == MTPBackend::TRACK )
    {
        listMusic( path );
        m_slave->listEntry( KIO::UDSEntry(), true );
    }
    else
        m_slave->error( KIO::ERR_INTERNAL, i18n( "Invalid path requested!" ) );
}

void
MTPBackend::listMusic( const QString &pathOffset )
{
    kDebug() << "(listMusic) pathOffset = " << pathOffset << endl;
    foreach( LIBMTP_folder_t* folder, m_folderParentToPtrHash.values( pathOffset ) )
    {
        KIO::UDSEntry entry;
        entry.insert( KIO::UDSEntry::UDS_NAME, QString::fromUtf8( folder->name ) );
        KUrl url( getUrlPrefix() );
        url.addPath( pathOffset );
        QString extraPath = QString::number( folder->folder_id ) + "_###_" + QString::fromUtf8( folder->name );
        url.addPath( extraPath ); 
        kDebug() << "(listMusic): folder url.url = " << url.url() << endl;
        entry.insert( KIO::UDSEntry::UDS_URL, url.url() );
        entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
        entry.insert( KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IRGRP | S_IROTH );
        m_slave->listEntry( entry, false );
    }
    foreach( LIBMTP_track_t* track, m_trackParentToPtrHash.values( pathOffset ) )
    {
        KIO::UDSEntry entry;
        entry.insert( KIO::UDSEntry::UDS_NAME, QString::fromUtf8( track->filename ) );
        KUrl url( getUrlPrefix() );
        url.addPath( pathOffset );
        QString extraPath = QString::number( track->item_id ) + "_###_" + QString::fromUtf8( track->filename );
        url.addPath( extraPath ); 
        entry.insert( KIO::UDSEntry::UDS_URL, url.url() );
        entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
        entry.insert( KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IRGRP | S_IROTH );
        m_slave->listEntry( entry, false );
    }
}

void
MTPBackend::rename( const KUrl &src, const KUrl &dest, bool overwrite )
{
    Q_UNUSED( src );
    Q_UNUSED( dest );
    Q_UNUSED( overwrite );
    //make sure they're renaming playlists to playlists, tracks to tracks, etc...
}

void
MTPBackend::stat( const KUrl &url )
{
    //TODO: Not finished
    QString path = getFilePath( url );
    kDebug() << "in MTPBackend::stat, url is: " << url.url() << endl; 
    kDebug() << "in MTPBackend::stat, getFilePath path is: " << path << endl;
    KIO::UDSEntry entry;
    if( path.isEmpty() )
    {
        entry.insert( KIO::UDSEntry::UDS_NAME, m_solidDevice.product());
        entry.insert( KIO::UDSEntry::UDS_URL, url.url() );
        entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
        entry.insert( KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IRGRP | S_IROTH );
        m_slave->statEntry( entry );
        return;
    }
    if( getObjectType( url ) == MTPBackend::TRACK )
        statMusic( url, entry );
    m_slave->statEntry( entry );
}

void
MTPBackend::statMusic( const KUrl &url, KIO::UDSEntry &entry )
{
    QString path = getFilePath( url );
    kDebug() << "Found an instance of path " << path << " in the hash.";
    if( m_pathToTrackIdHash.contains( path ) )
    {
        quint32 id = m_pathToTrackIdHash[path];
        LIBMTP_track_t* track = static_cast<LIBMTP_track_t*>(m_idToPtrHash[id]);
        entry.insert( KIO::UDSEntry::UDS_NAME, track->filename );
        entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG );
    }
    else //it's a folder
    {
        quint32 id = m_pathToFolderIdHash[path];
        LIBMTP_folder_t* folder = static_cast<LIBMTP_folder_t*>(m_idToPtrHash[id]);
        entry.insert( KIO::UDSEntry::UDS_NAME, folder->name );
        entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
    }
    entry.insert( KIO::UDSEntry::UDS_URL, url.url() );
    entry.insert( KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IRGRP | S_IROTH );
}

void
MTPBackend::buildMusicListing()
{
    m_trackList = LIBMTP_Get_Tracklisting_With_Callback( m_device, progressCallback, (void *)this );
    LIBMTP_track_t* trackList = m_trackList;
    m_folderList = LIBMTP_Get_Folder_List( m_device );
    LIBMTP_folder_t* folderList = m_folderList;
    LIBMTP_folder_t* defaultFolder = LIBMTP_Find_Folder( folderList, m_device->default_music_folder );
    QString defaultMusicLocation = QString::number( defaultFolder->folder_id ) + "_###_" + QString::fromUtf8( defaultFolder->name );
    m_defaultMusicLocation = defaultMusicLocation;
    kDebug() << "defaultMusicLocation set to: " << defaultMusicLocation << endl;
    buildFolderList( folderList, QString() );
    QString folderPath;
    while( trackList != 0 )
    {
        if( trackList->parent_id == 0 )
        {
            kDebug() << "Found track " << QString::fromUtf8( trackList->filename ) << " in base folder." << endl;
            m_trackParentToPtrHash.insert( QString::null, trackList );
            m_pathToTrackIdHash.insert( QString::number( trackList->item_id ) + QString( "_###_" ) + QString::fromUtf8( trackList->filename ),  trackList->item_id );
        }
        else
        {
            folderPath = m_folderIdToPathHash.value( trackList->parent_id );
            kDebug() << "Found track " << QString::fromUtf8( trackList->filename ) << " in folder " << folderPath << endl;
            m_trackParentToPtrHash.insert( folderPath, trackList );
            m_pathToTrackIdHash.insert( folderPath + "/" + QString::number( trackList->item_id ) + "_###_" + QString::fromUtf8( trackList->filename ), trackList->item_id );
        }
        m_idToPtrHash.insert( trackList->item_id, (void*)trackList );
        m_objectTypeHash.insert( trackList->item_id, MTPBackend::TRACK );
        trackList = trackList->next;
    }
    kDebug() << "Printing folder keys..." << endl;
    foreach( QString key, m_pathToFolderIdHash.keys() )
        kDebug() << "Folder key: " << key << endl;
    kDebug() << "Printing track keys..." << endl;
    foreach( QString key, m_pathToTrackIdHash.keys() )
        kDebug() << "Track key: " << key << endl;
    m_gotMusicListing = true;
}

void
MTPBackend::buildFolderList( LIBMTP_folder_t *folderList, const QString &parentPath )
{
    if( folderList == 0 )
        return;

    kDebug() << "buildFolderList: Found folder " << QString::fromUtf8( folderList->name ) << " in " << parentPath << endl;

    QString nextPath;
    if( parentPath.isEmpty() )
        nextPath = QString::number( folderList->folder_id ) + "_###_" + QString::fromUtf8( folderList->name );
    else
        nextPath = parentPath + "/" + QString::number( folderList->folder_id ) + "_###_" + QString::fromUtf8( folderList->name );

    kDebug() << "nextPath is: " << nextPath << endl;

    m_folderParentToPtrHash.insert( parentPath, folderList );
    kDebug() << "inserted " << parentPath << " into m_folderParentToPtrHash" << endl;
    m_folderIdToPathHash.insert( folderList->folder_id, nextPath );
    m_pathToFolderIdHash.insert( nextPath, folderList->folder_id );
    m_idToPtrHash.insert( folderList->folder_id, (void*)folderList );
    m_objectTypeHash.insert( folderList->folder_id, MTPBackend::FOLDER );

    buildFolderList( folderList->child, nextPath );
    buildFolderList( folderList->sibling, parentPath );
}

int
MTPBackend::progressCallback( quint64 const sent, quint64 const total, void const * const data )
{
    Q_UNUSED( data );
    kapp->processEvents();
    MTPBackend *backend = (MTPBackend *)(data);
    if( sent == total )
        backend->getSlave()->infoMessage( i18n( "Receiving tracklisting...done" ) );
    else
        backend->getSlave()->infoMessage( i18n( "Receiving tracklisting...%1\%", ( ( sent * 1.0 )/total ) * 100 ) );
    kDebug() << "libmtp progress callback called, with " << sent/total << " done." << endl;
    return 0;
}

int
MTPBackend::getObjectType( const KUrl &url )
{
    if( !m_gotMusicListing )
        buildMusicListing();
    bool ok;
    QString path = url.fileName();
    quint32 val = QString( path.left( path.indexOf( "_###_" ) ) ).toULong( &ok );
    if( !ok )
        return MTPBackend::UNKNOWN;
    else
        return m_objectTypeHash.value( val );
}

quint32
MTPBackend::getUIDFromFilename( const KUrl &url )
{
    bool ok;
    QString path = url.fileName();
    quint32 val = QString( path.left( path.indexOf( "_###_" ) ) ).toULong( &ok );
    if( !ok )
        return 0;
    else
        return val;
}

#include "pmpkioslave_mtpbackend.moc"

