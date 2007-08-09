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

#include "pmpbackend.h"
#include "pmpdevice.h"
#include "pmpkioslave.h"

#include <QCoreApplication>
#include <QString>
#include <QTimer>

#include <kapplication.h>
#include <kcomponentdata.h>
#include <kdebug.h>
#include <kdirnotify.h>
#include <kio/ioslave_defaults.h>
#include <klocale.h>
#include <solid/device.h>
#include <solid/deviceinterface.h>
#include <solid/genericinterface.h>
#include <solid/portablemediaplayer.h>

using namespace KIO;

extern "C" int KDE_EXPORT kdemain( int argc, char **argv )
{
    QCoreApplication app( argc, argv );
    KComponentData( "kio_pmp", "kdelibs4" );
    (void) KGlobal::locale();

    fprintf(stderr, "ENTERED kdemain of pmp slave\n" );

    if( argc != 4 )
    {
        fprintf(stderr, "Usage: kio_pmp protocol domain-socket1 domain-socket2\n" );
        exit( -1 );
    }

    PMPProtocol slave( argv[1], argv[2], argv[3] );
    slave.dispatchLoop();
    return 0;
}

PMPProtocol::PMPProtocol( const QByteArray &protocol, const QByteArray &pool,
                          const QByteArray &app )
                        : SlaveBase( protocol, pool, app )
                        , m_devices()
                        , m_mtpInitialized( false )
{
}

PMPProtocol::~PMPProtocol()
{
    foreach( QString udi, m_devices.keys() )
        delete m_devices[udi];
}

void
PMPProtocol::setHost( const QString &host, quint16 port,
                      const QString &user, const QString &pass )
{
    Q_UNUSED( port );
    Q_UNUSED( user );
    Q_UNUSED( pass );
    if( !host.isEmpty() )
    {
        error( KIO::ERR_CANNOT_OPEN_FOR_READING,
                i18n("portable media player : Setting a hostname is not supported. \
                      Use a URL of the form pmp:/<udi> or pmp:///<udi>" ) );
    }
}

void
PMPProtocol::del( const KUrl &url, bool isfile )
{
    kDebug() << endl << endl << "Entering del with url = " << url << endl;
    if( getBackendForUrl( url ) )
        getBackendForUrl( url )->del( url, isfile );

    emit finished();
    kDebug() << endl << endl << "Leaving del with url = " << url << endl;
}

void
PMPProtocol::get( const KUrl &url )
{
    kDebug() << endl << endl << "Entering get with url = " << url << endl;
    if( getBackendForUrl( url ) )
        getBackendForUrl( url )->get( url );

    emit finished();
    kDebug() << endl << endl << "Leaving get with url = " << url << endl;
}

void
PMPProtocol::listDir( const KUrl &url )
{
    kDebug() << endl << endl << "Entering listDir with url = " << url;
    kDebug() << "path = " << url.path() << ", path.isEmpty = " << (url.path().isEmpty() ? "true" : "false" );
    if( url.isEmpty() )
    {
        listEntry( UDSEntry(), true );
        emit finished();
        return;
    }
    if( url.path().isEmpty() || url.path() == "/" )
    {
        QList<Solid::Device> deviceList = Solid::Device::listFromQuery( "is PortableMediaPlayer" );
        foreach( Solid::Device device, deviceList )
        {
            UDSEntry entry;
            if( !m_devices.contains( transUdi( device.udi() ) ) )
            {
                m_devices[transUdi( device.udi() )] = new PMPDevice( this, device );
                m_devices[transUdi( device.udi() )]->initialize();
            }
            QString name = m_devices[transUdi( device.udi() )]->backend()->getFriendlyName();
            kDebug() << "Friendly name returned from device is: " << name;
            if( name.isEmpty() )
            {
                name = m_devices[transUdi( device.udi() )]->backend()->getModelName();
                kDebug() << "Model name returned from device is: " << name;
            }
            if( name.isEmpty() )
            {
                name = getSolidFriendlyName( device );
                kDebug() << "Friendly name returned from Solid is: " << name;
            }
            entry.insert( KIO::UDSEntry::UDS_NAME, name.isEmpty() ? QString( "Portable Media Player at " + device.udi() ) : name );
            entry.insert( KIO::UDSEntry::UDS_URL,"pmp:///" + transUdi( device.udi() ) );
            entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
            entry.insert( KIO::UDSEntry::UDS_HIDDEN, 0 );
            listEntry( entry, false );
        }
        listEntry( UDSEntry(), true );
        emit finished();
        return;
    }
    else
    {
        if( getBackendForUrl( url ) )
            getBackendForUrl( url )->listDir( url );
    }

    emit finished();
    kDebug() << endl << endl << "Leaving listDir with url = " << url << endl;
}

void
PMPProtocol::rename( const KUrl &src, const KUrl &dest, bool overwrite )
{
    kDebug() << "srcUrl = " << src.url() << ", destUrl = " << dest.url();
    QString srcPath = src.path( KUrl::RemoveTrailingSlash );
    QString destPath = dest.path( KUrl::RemoveTrailingSlash );
    while( srcPath[0] == '/' )
        srcPath.remove( 0, 1 );
    while( destPath[0] == '/' )
        destPath.remove( 0, 1 );
    //Check to see if they're trying to set a friendly name
    //i.e. only top-level paths
    kDebug() << endl << "srcPath = " << srcPath << ", destPath = " << destPath;
    kDebug() << endl << "src.directory() = " << src.directory() << ", dest.directory() = " << dest.directory();
    if( srcPath.indexOf( '/' ) == -1 && destPath.indexOf( '/' ) == -1 )
    {
        QString srcName = udiFromUrl( src );
        QString dstName = udiFromUrl( dest );
        kDebug() << "srcName = " << srcName << ", dstName (friendly name) = " << dstName;
        kDebug() << "m_devices keys: ";
        foreach( QString key, m_devices.keys() )
            kDebug() << "key = " << key;
        if( m_devices.contains( dstName ) )
            warning( i18n( "Destination name cannot be the same as a Solid UDI!" ) );
        else if ( getBackendForUrl( src ) )
        {
            getBackendForUrl( src )->setFriendlyName( dstName );
        }
        emit finished();
        org::kde::KDirNotify::emitFilesAdded( "pmp:///" );
        return;
    }
    if( udiFromUrl( src ) != udiFromUrl( dest ) )
    {
        //this shouldn't ever happen...
        warning( i18n( "Could not rename file, files cannot be renamed across devices!" ) );
        emit finished();
        return;
    }
    if( getBackendForUrl( src ) )
        getBackendForUrl( src )->rename( src, dest, overwrite ); 

    emit finished();
}

void
PMPProtocol::stat( const KUrl &url )
{
    kDebug() << endl << endl << "Entering stat with url = " << url << endl;
    if( url.path().isEmpty() || url.path() == "/" )
    {
        KIO::UDSEntry entry;
        entry.insert( KIO::UDSEntry::UDS_NAME, "Available Devices" );
        entry.insert( KIO::UDSEntry::UDS_URL, "pmp:///" );
        entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
        entry.insert( KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IRGRP | S_IROTH );
        statEntry( entry );
        emit finished();
        return;
    }
    else if( getBackendForUrl( url ) )
        getBackendForUrl( url )->stat( url );

    emit finished();
    kDebug() << endl << endl << "Leaving stat with url = " << url << endl;
}

QString
PMPProtocol::getSolidFriendlyName( const Solid::Device & device ) const
{
    kDebug() << "Looking for a friendly name for " << device.udi() << "...";
    //NOTE/TODO: The following is hal-dependent.
    const Solid::GenericInterface *gi = device.as<Solid::GenericInterface>();
    if( !gi )
    {
        kDebug() << "Couldn't get the device as a generic interface.";
        return QString();
    }
    if( !gi->propertyExists( "portable_audio_player.access_method.drivers" ) )
    {
        kDebug() << "No access_method.drivers property detected.";
        return QString();
    }
    QVariant possibleLibraries = gi->property( "portable_audio_player.access_method.drivers" );
    if( !possibleLibraries.isValid() || possibleLibraries.isNull() || !possibleLibraries.canConvert( QVariant::StringList ) )
        return QString();
    QStringList libraries = possibleLibraries.toStringList();
    foreach( QString library, libraries )
    {
        QVariant possibleName = gi->property( "portable_audio_player." + library + ".name" );
        if( possibleName.isValid() && !possibleName.isNull() && possibleName.canConvert( QVariant::String ) )
        {
            QString name = possibleName.toString();
            if( !name.isEmpty() )
                return name;
        }
    }
    return QString();
}

QString
PMPProtocol::udiFromUrl( const KUrl &url )
{
    if( url.isEmpty() )
    {
        error( KIO::ERR_CANNOT_OPEN_FOR_READING, i18n( "portable media player : Empty UDI passed in" ) );
        return QString::null;
    }
    
    QString path = url.path( KUrl::RemoveTrailingSlash );
    while( path[0] == '/' )
        path.remove( 0, 1 );

    int index = path.indexOf( '/' );
    //if not found, use the path as is; if it is truncate so we only get the udi
    QString udi = ( index == -1 ? path : path.left( index ) );
    //translate the udi to its required format
    udi = transUdi( udi );

    return udi;
}

PMPBackend*
PMPProtocol::getBackendForUrl( const KUrl &url )
{
    if( !m_devices.contains( udiFromUrl( url ) ) )
    {
        m_devices[udiFromUrl( url )] = new PMPDevice( this, Solid::Device( untransUdi( udiFromUrl( url ) ) ) );
        if( m_devices[udiFromUrl( url )]->isValid() )
            m_devices[udiFromUrl( url )]->initialize();
        else
        {
            delete m_devices[udiFromUrl( url )];
            m_devices.remove( udiFromUrl( url ) );
            return 0;
        }
    }
    return m_devices[udiFromUrl( url )]->backend();
}

#include "pmpkioslave.moc"

