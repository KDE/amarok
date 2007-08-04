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
    kDebug() << "Creating MTPBackend";

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
    kDebug() << endl << "In MTPBackend destructor, releasing device" << endl;
    if( m_device )
        LIBMTP_Release_Device( m_device );
}

void
MTPBackend::initialize()
{
    kDebug() << "Initializing MTPBackend for device " << m_solidDevice.udi();
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
    kDebug() << endl << endl << "Case-insensitively looking for serial number starting with: " << serial << endl;
    LIBMTP_mtpdevice_t *currdevice;
    for( currdevice = m_deviceList; currdevice != NULL; currdevice = currdevice->next )
    {
        kDebug() << "currdevice serial number = " << LIBMTP_Get_Serialnumber( currdevice );
        //WARNING: a startsWith is done below, as the value reported by HAL for the serial number seems to be about half
        //the length of the value reported by libmtp...is this always true?  Could this cause two devices to
        //be recognized as the same one?
        if( QString( LIBMTP_Get_Serialnumber( currdevice ) ).startsWith( serial, Qt::CaseInsensitive ) )
        {
            kDebug() << endl << endl << "Found a matching serial!" << endl;
            m_device = currdevice;
        }
        else
            LIBMTP_Release_Device( currdevice );
    }

    if( m_device )
        kDebug() << "FOUND THE MTP DEVICE WE WERE LOOKING FOR!";

    return;
}

QString
MTPBackend::getFriendlyName() const
{
    kDebug() << "Getting MTPBackend friendly name";
    QString friendlyName = QString::fromUtf8( LIBMTP_Get_Friendlyname( m_device ) );
    kDebug() << "Found MTPBackend friendly name = " << friendlyName;
    return friendlyName;
}

void
MTPBackend::setFriendlyName( const QString &name )
{
    kDebug() << "Setting MTPBackend friendly name";
    if( LIBMTP_Set_Friendlyname( m_device, name.toUtf8() ) != 0 )
        m_slave->warning( i18n( "Failed to set friendly name on the device!" ) );
}

QString
MTPBackend::getModelName() const
{
    kDebug() << "Getting MTPBackend model name";
    QString modelName = QString::fromUtf8( LIBMTP_Get_Modelname( m_device ) );
    kDebug() << "Found MTPBackend model name = " << modelName;
    return modelName;
}

void
MTPBackend::del( const KUrl &url, bool isfile )
{
    QString path = getFilePath( url );
    if( !m_gotMusicListing )
        buildMusicListing();
    int nextSlash = path.indexOf( '/' );
    QString nextLevel;
    if( nextSlash == -1 )
        nextLevel = path;
    else
    {
        nextLevel = path.left( path.indexOf( '/' ) );
    }
    if( nextLevel == m_defaultMusicLocation )
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
        m_pathToTrackIdHash.remove( path, trackid );
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
    QString path = getFilePath( url );
    kDebug() << "in MTPBackend::get, path is: " << path;
}

void
MTPBackend::listDir( const KUrl &url )
{
    QString path = getFilePath( url );
    kDebug() << "in MTPBackend::listDir, path is: " << path;
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
    int nextSlash = path.indexOf( '/' );
    QString nextLevel;
    if( nextSlash == -1 )
        nextLevel = path;
    else
    {
        nextLevel = path.left( path.indexOf( '/' ) );
    }
    kDebug() << "nextLevel = " << nextLevel << ", default = " << m_defaultMusicLocation;
    if( nextLevel == m_defaultMusicLocation )
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
    kDebug() << "(listMusic) pathOffset = " << pathOffset;
    foreach( LIBMTP_folder_t* folder, m_folderParentToPtrHash.values( pathOffset ) )
    {
        KIO::UDSEntry entry;
        entry.insert( KIO::UDSEntry::UDS_NAME, QString::fromUtf8( folder->name ) );
        KUrl url( getUrlPrefix() );
        url.addPath( pathOffset );
        QString extraPath = QString::number( folder->folder_id ) + "_###_" + QString::fromUtf8( folder->name );
        url.addPath( extraPath ); 
        kDebug() << "(listMusic): folder url.url = " << url.url();
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
    kDebug() << "in MTPBackend::stat, path is: " << path;
    KIO::UDSEntry entry;
    entry.insert( KIO::UDSEntry::UDS_NAME,m_solidDevice.product());
    entry.insert( KIO::UDSEntry::UDS_FILE_TYPE,S_IFDIR );
    entry.insert( KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IRGRP | S_IROTH );
    m_slave->statEntry( entry );
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
    kDebug() << "defaultMusicLocation set to: " << defaultMusicLocation;
    buildFolderList( folderList, QString() );
    QString folderPath;
    while( trackList != 0 )
    {
        if( trackList->parent_id == 0 )
        {
            kDebug() << "Found track " << QString::fromUtf8( trackList->filename ) << " in base folder.";
            m_trackParentToPtrHash.insert( QString::null, trackList );
            m_pathToTrackIdHash.insert( QString::number( trackList->item_id ) + QString( "_###_" ) + QString::fromUtf8( trackList->filename ),  trackList->item_id );
            m_idToPtrHash.insert( trackList->item_id, trackList );
        }
        else
        {
            folderPath = m_folderIdToPathHash.value( trackList->parent_id );
            kDebug() << "Found track " << QString::fromUtf8( trackList->filename ) << " in folder " << folderPath;
            m_trackParentToPtrHash.insert( folderPath, trackList );
            m_pathToTrackIdHash.insert( folderPath + "/" + QString::number( trackList->item_id ) + "_###_" + QString::fromUtf8( trackList->filename ), trackList->item_id );
            m_idToPtrHash.insert( trackList->item_id, trackList );
        }
        trackList = trackList->next;
    }
    kDebug() << "Printing folder keys...";
    foreach( QString key, m_pathToFolderIdHash.keys() )
        kDebug() << "Folder key: " << key;
    kDebug() << "Printing track keys...";
    foreach( QString key, m_pathToTrackIdHash.keys() )
        kDebug() << "Track key: " << key;
    m_gotMusicListing = true;
}

void
MTPBackend::buildFolderList( LIBMTP_folder_t *folderList, const QString &parentPath )
{
    if( folderList == 0 )
        return;

    kDebug() << "buildFolderList: Found folder " << QString::fromUtf8( folderList->name ) << " in " << parentPath;

    QString nextPath;
    if( parentPath.isEmpty() )
        nextPath = QString::number( folderList->folder_id ) + "_###_" + QString::fromUtf8( folderList->name );
    else
        nextPath = parentPath + "/" + QString::number( folderList->folder_id ) + "_###_" + QString::fromUtf8( folderList->name );

    kDebug() << "nextPath is: " << nextPath;

    m_folderParentToPtrHash.insert( parentPath, folderList );
    kDebug() << "inserted " << parentPath << " into m_folderParentToPtrHash";
    m_folderIdToPathHash.insert( folderList->folder_id, nextPath );
    m_pathToFolderIdHash.insert( nextPath, folderList->folder_id );
    m_idToPtrHash.insert( folderList->folder_id, folderList );

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
    kDebug() << "libmtp progress callback called, with " << sent/total << " done.";
    return 0;
}

int
MTPBackend::getObjectType( const QString &path )
{
    bool ok;
    quint32 val = QString( path.left( path.indexOf( "_###_" ) ) ).toULong( &ok );
    if( !ok )
        return MTPBackend::MTP_UNKNOWN;
    else
        return m_objectTypeHash.value( val );
}

#include "pmpkioslave_mtpbackend.moc"

