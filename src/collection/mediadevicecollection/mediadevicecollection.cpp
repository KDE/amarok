/* 
   Mostly taken from Daap code:
   Copyright (C) 2006 Ian Monroe <ian@monroe.nu>
   Copyright (C) 2006 Seb Ruiz <ruiz@kde.org>  
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#define DEBUG_PREFIX "MediaDeviceCollection"

#include "mediadevicecollection.h"

#include "amarokconfig.h"
#include "mediadevicemeta.h"
#include "Debug.h"
#include "MemoryQueryMaker.h"
#include "reader.h"

#include <QStringList>
#include <QTimer>

#include <dnssd/remoteservice.h>
#include <dnssd/servicebase.h>
#include <dnssd/servicebrowser.h>
#include <k3resolver.h>

AMAROK_EXPORT_PLUGIN( MediaDeviceCollectionFactory )

MediaDeviceCollectionFactory::MediaDeviceCollectionFactory()
    : CollectionFactory()
{
    //nothing to do
}

MediaDeviceCollectionFactory::~MediaDeviceCollectionFactory()
{
    delete m_browser;
}

void
MediaDeviceCollectionFactory::init()
{
    DEBUG_BLOCK
    //don't block Amarok's startup by connecting to MEDIADEVICE servers
    QTimer::singleShot( 1000, this, SLOT( connectToManualServers() ) );
    m_browser = new DNSSD::ServiceBrowser("_mediadevice._tcp");
    m_browser->setObjectName("mediadeviceServiceBrowser");
    connect( m_browser, SIGNAL( serviceAdded( DNSSD::RemoteService::Ptr ) ),
                  this,   SLOT( foundMediaDevice   ( DNSSD::RemoteService::Ptr ) ) );
    connect( m_browser, SIGNAL( serviceRemoved( DNSSD::RemoteService::Ptr ) ),
                  this,   SLOT( serverOffline ( DNSSD::RemoteService::Ptr ) ) );
    m_browser->startBrowse();
}

void
MediaDeviceCollectionFactory::connectToManualServers()
{
    DEBUG_BLOCK
    QStringList sl = AmarokConfig::manuallyAddedServers();
    foreach( const QString &server, sl )
    {
        debug() << "Adding server " << server;
        QStringList current = server.split( ':', QString::KeepEmptyParts );
        QString host = current.first();
        quint16 port = current.last().toUShort();
        QString ip = resolve( host );
        if( ip != "0" )
        {
            //adding manual servers to the collectionMap doesn't make sense
            MediaDeviceCollection *coll = new MediaDeviceCollection( host, ip, port );
            emit newCollection( coll );
        }
    }
}

QString
MediaDeviceCollectionFactory::resolve( const QString &hostname )
{
    KNetwork::KResolver resolver( hostname );
    resolver.setFamily( KNetwork::KResolver::KnownFamily ); //A druidic incantation from Thiago. Works around a KResolver bug #132851
    resolver.start();
    if( resolver.wait( 5000 ) )
    {
        KNetwork::KResolverResults results = resolver.results();
/*        if( results.error() )
            debug() << "Error resolving "  << hostname << ": ("
                    << resolver.errorString( results.error() ) << ")";*/
        if( !results.empty() )
        {
            QString ip = results[0].address().asInet().ipAddress().toString();
            debug() << "ip found is " << ip;
            return ip;
        }
    }
    return "0"; //error condition
}

void
MediaDeviceCollectionFactory::serverOffline( DNSSD::RemoteService::Ptr service )
{
    DEBUG_BLOCK
    QString key =  serverKey( service.data() );
    if( m_collectionMap.contains( key ) )
    {
        MediaDeviceCollection* coll = m_collectionMap[ key ];
        if( coll )
            coll->serverOffline();  //collection will be deleted by collectionmanager
        else
            warning() << "collection already null";
    }
    else
        warning() << "removing non-existant service";
}

void
MediaDeviceCollectionFactory::foundMediaDevice( DNSSD::RemoteService::Ptr service )
{
    DEBUG_BLOCK

    connect( service.data(), SIGNAL( resolved( bool ) ), this, SLOT( resolvedMediaDevice( bool ) ) );
    service->resolveAsync();
}

void
MediaDeviceCollectionFactory::resolvedMediaDevice( bool success )
{
    DEBUG_BLOCK
    const DNSSD::RemoteService* service =  dynamic_cast<const DNSSD::RemoteService*>(sender());
    if( !success || !service ) return;
    debug() << service->serviceName() << ' ' << service->hostName() << ' ' << service->domain() << ' ' << service->type();

    QString ip = resolve( service->hostName() );
    if( ip == "0" || m_collectionMap.contains(serverKey( service )) ) //same server from multiple interfaces
        return;

    MediaDeviceCollection *coll = new MediaDeviceCollection( service->hostName(), ip, service->port() );
    connect( coll, SIGNAL( collectionReady() ), SLOT( slotCollectionReady() ) );
    connect( coll, SIGNAL( remove() ), SLOT( slotCollectionDownloadFailed() ) );
    m_collectionMap.insert( serverKey( service ), coll );
}

QString
MediaDeviceCollectionFactory::serverKey( const DNSSD::RemoteService* service ) const
{
    return service->hostName() + ':' + QString::number( service->port() );
}

void
MediaDeviceCollectionFactory::slotCollectionReady()
{
    DEBUG_BLOCK
    MediaDeviceCollection *collection = dynamic_cast<MediaDeviceCollection*>( sender() );
    if( collection )
    {
        disconnect( collection, SIGNAL( remove() ), this, SLOT( slotCollectionDownloadFailed() ) );
        emit newCollection( collection );
    }
}

void
MediaDeviceCollectionFactory::slotCollectionDownloadFailed()
{
    MediaDeviceCollection *collection = dynamic_cast<MediaDeviceCollection*>( sender() );
    if( !collection )
        return;
    foreach( const QString &key, m_collectionMap.keys() )
    {
        if( m_collectionMap.value( key ) == collection )
        {
            m_collectionMap.remove( key );
            collection->deleteLater();
            break;
        }
    }
}

//MediaDeviceCollection

MediaDeviceCollection::MediaDeviceCollection( const QString &host, const QString &ip, quint16 port )
    : Collection()
    , MemoryCollection()
    , m_host( host )
    , m_port( port )
    , m_ip( ip )
    , m_reader( 0 )
{
    debug() << "Host: " << host << " port: " << port;
    m_reader = new MediaDevice::Reader( this, host, port, QString(), this, "MediaDeviceReader" );
    connect( m_reader, SIGNAL( passwordRequired() ), SLOT( passwordRequired() ) );
    connect( m_reader, SIGNAL( httpError( QString ) ), SLOT( httpError( QString ) ) );
    m_reader->loginRequest();
}

MediaDeviceCollection::~MediaDeviceCollection()
{
    delete m_reader;
}

void
MediaDeviceCollection::startFullScan()
{
    //ignore
}

QueryMaker*
MediaDeviceCollection::queryMaker()
{
    return new MemoryQueryMaker( this, collectionId() );
}

QString
MediaDeviceCollection::collectionId() const
{
    return "mediadevice://" + m_ip + ':' + m_port;
}

QString
MediaDeviceCollection::prettyName() const
{
    return "mediadevice://" + m_host;
}

void
MediaDeviceCollection::passwordRequired()
{
    //get password
    QString password;
    delete m_reader;
    m_reader = new MediaDevice::Reader( this, m_host, m_port, password, this, "MediaDeviceReader" );
    connect( m_reader, SIGNAL( passwordRequired() ), SLOT( passwordRequired() ) );
    connect( m_reader, SIGNAL( httpError( QString ) ), SLOT( httpError( QString ) ) );
    m_reader->loginRequest();
}

void
MediaDeviceCollection::httpError( const QString &error )
{
    DEBUG_BLOCK
    debug() << "Http error in MediaDeviceReader: " << error;
    deleteLater();
}

void
MediaDeviceCollection::serverOffline()
{
    emit remove();
}

void
MediaDeviceCollection::loadedDataFromServer()
{
    DEBUG_BLOCK
    emit collectionReady();
}

void
MediaDeviceCollection::parsingFailed()
{
    emit remove();
}

#include "mediadevicecollection.moc"

