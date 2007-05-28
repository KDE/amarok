/* 
   Copyright (C) 2006 Ian Monroe <ian@monroe.nu>
   Copyright (C) 2006 Seb Ruiz <me@sebruiz.net>
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

#include "daapcollection.h"

#include "amarokconfig.h"
#include "daapmeta.h"
#include "debug.h"
#include "memoryquerymaker.h"
#include "reader.h"

#include <QStringList>
#include <QTimer>

#include <dnssd/remoteservice.h>
#include <dnssd/servicebase.h>
#include <dnssd/servicebrowser.h>
#include <k3resolver.h>

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
    foreach( QString server, sl )
    {
        debug() << "Adding server " << server << endl;
        QStringList current = server.split( ':', QString::KeepEmptyParts );
        QString host = current.first();
        quint16 port = current.last().toUShort();
        QString ip = resolve( host );
        if( ip != "0" )
        {
            //adding manual servers to the collectionMap doesn't make sense
            DaapCollection *coll = new DaapCollection( host, ip, port );
            emit newCollection( coll );
        }
    }
}

QString
DaapCollectionFactory::resolve( const QString &hostname )
{
    KNetwork::KResolver resolver( hostname );
    resolver.setFamily( KNetwork::KResolver::KnownFamily ); //A druidic incantation from Thiago. Works around a KResolver bug #132851
    resolver.start();
    if( resolver.wait( 5000 ) )
    {
        KNetwork::KResolverResults results = resolver.results();
        if( results.error() )
            debug() << "Error resolving "  << hostname << ": ("
                    << resolver.errorString( results.error() ) << ")" << endl;
        if( !results.empty() )
        {
            QString ip = results[0].address().asInet().ipAddress().toString();
            debug() << "ip found is " << ip << endl;
            return ip;
        }
    }
    return "0"; //error condition
}

void
DaapCollectionFactory::serverOffline( DNSSD::RemoteService::Ptr service )
{
    DEBUG_BLOCK
    QString key =  serverKey( service.data() );
    if( m_collectionMap.contains( key ) )
    {
        DaapCollection* coll = m_collectionMap[ key ];
        if( coll )
            coll->serverOffline();  //collection will be deleted by collectionmanager
        else
            warning() << "collection already null" << endl;
    }
    else
        warning() << "removing non-existant service" << endl;
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
    debug() << service->serviceName() << ' ' << service->hostName() << ' ' << service->domain() << ' ' << service->type() << endl;

    QString ip = resolve( service->hostName() );
    if( ip == "0" || m_collectionMap.contains(serverKey( service )) ) //same server from multiple interfaces
        return;

    DaapCollection *coll = new DaapCollection( service->hostName(), ip, service->port() );
    m_collectionMap.insert( serverKey( service ), coll );
    emit newCollection( coll );
}

QString
DaapCollectionFactory::serverKey( const DNSSD::RemoteService* service ) const
{
    return service->hostName() + ':' + QString::number( service->port() );
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
    debug() << "Host: " << host << " port: " << port << endl;
    m_reader = new Reader( this, host, port, QString(), this, "DaapReader" );
    connect( m_reader, SIGNAL( passwordRequired() ), SLOT( passwordRequired() ) );
    connect( m_reader, SIGNAL( httpError( QString ) ), SLOT( httpError( QString ) ) );
    m_reader->loginRequest();
}

DaapCollection::~DaapCollection()
{
    delete m_reader;
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
    m_reader = new Reader( this, m_host, m_port, password, this, "DaapReader" );
    connect( m_reader, SIGNAL( passwordRequired() ), SLOT( passwordRequired() ) );
    connect( m_reader, SIGNAL( httpError( QString ) ), SLOT( httpError( QString ) ) );
    m_reader->loginRequest();
}

void
DaapCollection::httpError( const QString &error )
{
    debug() << "Http error in DaapReader: " << error << endl;
}

void
DaapCollection::serverOffline()
{
    emit remove();
}

#include "daapcollection.moc"

