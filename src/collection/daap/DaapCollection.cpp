/* 
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

#define DEBUG_PREFIX "DaapCollection"

#include "DaapCollection.h"

#include "amarokconfig.h"
#include "DaapMeta.h"
#include "Debug.h"
#include "MemoryQueryMaker.h"
#include "Reader.h"

#include <QStringList>
#include <QTimer>

#include <dnssd/remoteservice.h>
#include <dnssd/servicebase.h>
#include <dnssd/servicebrowser.h>

AMAROK_EXPORT_PLUGIN( DaapCollectionFactory )

DaapCollectionFactory::DaapCollectionFactory()
    : CollectionFactory()
{
    //nothing to do
}

DaapCollectionFactory::~DaapCollectionFactory()
{
    delete m_browser;
}

void
DaapCollectionFactory::init()
{
    DEBUG_BLOCK
    //don't block Amarok's startup by connecting to DAAP servers
    QTimer::singleShot( 1000, this, SLOT( connectToManualServers() ) );
    m_browser = new DNSSD::ServiceBrowser("_daap._tcp");
    m_browser->setObjectName("daapServiceBrowser");
    connect( m_browser, SIGNAL( serviceAdded( DNSSD::RemoteService::Ptr ) ),
                  this,   SLOT( foundDaap   ( DNSSD::RemoteService::Ptr ) ) );
    connect( m_browser, SIGNAL( serviceRemoved( DNSSD::RemoteService::Ptr ) ),
                  this,   SLOT( serverOffline ( DNSSD::RemoteService::Ptr ) ) );
    m_browser->startBrowse();
}

void
DaapCollectionFactory::connectToManualServers()
{
    DEBUG_BLOCK
    QStringList sl = AmarokConfig::manuallyAddedServers();
    foreach( const QString &server, sl )
    {
        debug() << "Adding server " << server;
        QStringList current = server.split( ':', QString::KeepEmptyParts );
        QString host = current.first();
        quint16 port = current.last().toUShort();

        int lookup_id = QHostInfo::lookupHost( host, this, SLOT( resolvedManualServerIp(QHostInfo)));
        m_lookupHash.insert( lookup_id, port );
    }
}

void
DaapCollectionFactory::serverOffline( DNSSD::RemoteService::Ptr service )
{
    DEBUG_BLOCK
    QString key =  serverKey( service.data()->hostName(), service.data()->port() );
    if( m_collectionMap.contains( key ) )
    {
        DaapCollection* coll = m_collectionMap[ key ];
        if( coll )
            coll->serverOffline();  //collection will be deleted by collectionmanager
        else
            warning() << "collection already null";
    }
    else
        warning() << "removing non-existent service";
}

void
DaapCollectionFactory::foundDaap( DNSSD::RemoteService::Ptr service )
{
    DEBUG_BLOCK

    connect( service.data(), SIGNAL( resolved( bool ) ), this, SLOT( resolvedDaap( bool ) ) );
    service->resolveAsync();
}

void
DaapCollectionFactory::resolvedDaap( bool success )
{
    DEBUG_BLOCK
    const DNSSD::RemoteService* service =  dynamic_cast<const DNSSD::RemoteService*>(sender());
    if( !success || !service ) return;
    debug() << service->serviceName() << ' ' << service->hostName() << ' ' << service->domain() << ' ' << service->type();

    int lookup_id = QHostInfo::lookupHost( service->hostName(), this, SLOT( resolvedServiceIp(QHostInfo)));
    m_lookupHash.insert( lookup_id, service->port() );
}

QString
DaapCollectionFactory::serverKey( const QString& host, quint16 port) const
{
    return host + ':' + QString::number( port );
}

void
DaapCollectionFactory::slotCollectionReady()
{
    DEBUG_BLOCK
    DaapCollection *collection = dynamic_cast<DaapCollection*>( sender() );
    if( collection )
    {
        disconnect( collection, SIGNAL( remove() ), this, SLOT( slotCollectionDownloadFailed() ) );
        emit newCollection( collection );
    }
}

void
DaapCollectionFactory::slotCollectionDownloadFailed()
{
    DEBUG_BLOCK
    DaapCollection *collection = qobject_cast<DaapCollection*>( sender() );
    if( !collection )
        return;
    disconnect( collection, SIGNAL( collectionReady() ), this, SLOT( slotCollectionReady() ) );
    foreach( const QString &key, m_collectionMap.keys() )
    {
        if( m_collectionMap.value( key ) == collection )
        {
            m_collectionMap.remove( key );
            break;
        }
    }
    collection->deleteLater();
}

void
DaapCollectionFactory::resolvedManualServerIp( QHostInfo hostInfo )
{
    if ( !m_lookupHash.contains(hostInfo.lookupId()) )
        return;

    if ( hostInfo.addresses().isEmpty() )
        return;

    QString host = hostInfo.hostName();
    QString ip = hostInfo.addresses().at(0).toString();
    quint16 port = m_lookupHash.value( hostInfo.lookupId() );

    //adding manual servers to the collectionMap doesn't make sense
    DaapCollection *coll = new DaapCollection( host, ip, port );
    connect( coll, SIGNAL( collectionReady() ), SLOT( slotCollectionReady() ) );
    connect( coll, SIGNAL( remove() ), SLOT( slotCollectionDownloadFailed() ) );
}

void
DaapCollectionFactory::resolvedServiceIp( QHostInfo hostInfo )
{
    if ( !m_lookupHash.contains(hostInfo.lookupId()) )
        return;

    if ( hostInfo.addresses().isEmpty() )
        return;

    QString host = hostInfo.hostName();
    QString ip = hostInfo.addresses().at(0).toString();
    quint16 port = m_lookupHash.value( hostInfo.lookupId() );

    if( m_collectionMap.contains(serverKey( host, port )) ) //same server from multiple interfaces
        return;

    DaapCollection *coll = new DaapCollection( host, ip, port );
    connect( coll, SIGNAL( collectionReady() ), SLOT( slotCollectionReady() ) );
    connect( coll, SIGNAL( remove() ), SLOT( slotCollectionDownloadFailed() ) );
    m_collectionMap.insert( serverKey( host, port ), coll );
}

//DaapCollection

DaapCollection::DaapCollection( const QString &host, const QString &ip, quint16 port )
    : Collection()
    , MemoryCollection()
    , m_host( host )
    , m_port( port )
    , m_ip( ip )
    , m_reader( 0 )
{
    debug() << "Host: " << host << " port: " << port;
    m_reader = new Daap::Reader( this, host, port, QString(), this, "DaapReader" );
    connect( m_reader, SIGNAL( passwordRequired() ), SLOT( passwordRequired() ) );
    connect( m_reader, SIGNAL( httpError( QString ) ), SLOT( httpError( QString ) ) );
    m_reader->loginRequest();
}

DaapCollection::~DaapCollection()
{
}

void
DaapCollection::startFullScan()
{
    //ignore
}

QueryMaker*
DaapCollection::queryMaker()
{
    return new MemoryQueryMaker( this, collectionId() );
}

QString
DaapCollection::collectionId() const
{
    return "daap://" + m_ip + ':' + m_port;
}

QString
DaapCollection::prettyName() const
{
    return "daap://" + m_host;
}

void
DaapCollection::passwordRequired()
{
    //get password
    QString password;
    delete m_reader;
    m_reader = new Daap::Reader( this, m_host, m_port, password, this, "DaapReader" );
    connect( m_reader, SIGNAL( passwordRequired() ), SLOT( passwordRequired() ) );
    connect( m_reader, SIGNAL( httpError( QString ) ), SLOT( httpError( QString ) ) );
    m_reader->loginRequest();
}

void
DaapCollection::httpError( const QString &error )
{
    DEBUG_BLOCK
    debug() << "Http error in DaapReader: " << error;
    emit remove();
}

void
DaapCollection::serverOffline()
{
    emit remove();
}

void
DaapCollection::loadedDataFromServer()
{
    DEBUG_BLOCK
    emit collectionReady();
}

void
DaapCollection::parsingFailed()
{
    emit remove();
}

#include "DaapCollection.moc"

