/****************************************************************************************
 * Copyright (c) 2006 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2006 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "DaapCollection"

#include "DaapCollection.h"

#include "amarokconfig.h"
#include "DaapMeta.h"
#include "core/support/Debug.h"
#include "MemoryQueryMaker.h"
#include "Reader.h"
#include "statusbar/StatusBar.h"

#include <QStringList>
#include <QTimer>

#include <KLocale>

#include <dnssd/remoteservice.h>
#include <dnssd/servicebase.h>
#include <dnssd/servicebrowser.h>

AMAROK_EXPORT_COLLECTION( DaapCollectionFactory, daapcollection )

DaapCollectionFactory::DaapCollectionFactory( QObject *parent, const QVariantList &args )
    : Amarok::CollectionFactory()
    , m_browser( 0 )
{
    setParent( parent );
    Q_UNUSED( args );
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
        //handle invalid urls gracefully
        if( current.count() < 2 )
            continue;
            
        QString host = current.first();
        quint16 port = current.last().toUShort();
        The::statusBar()->longMessage( i18n( "Loading remote collection from host %1", host), StatusBar::Information );

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
        DaapCollection *coll = m_collectionMap[ key ];
        if( coll )
            coll->serverOffline();  //collection will be deleted by collectionmanager
        else
            warning() << "collection already null";
        
        m_collectionMap.remove( key );

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
    foreach( QPointer< DaapCollection > it, m_collectionMap )
    {
        if( it == collection )
        {
            m_collectionMap.remove( m_collectionMap.key( it ) );
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
    DEBUG_BLOCK
   // debug() << "got address:" << hostInfo.addresses() << "and lookup hash contains id" << hostInfo.lookupId() << "?" << m_lookupHash.contains(hostInfo.lookupId());
    if ( !m_lookupHash.contains(hostInfo.lookupId()) )
        return;

    if ( hostInfo.addresses().isEmpty() )
        return;

    QString host = hostInfo.hostName();
    QString ip = hostInfo.addresses().at(0).toString();
    quint16 port = m_lookupHash.value( hostInfo.lookupId() );

   // debug() << "already added server?" << m_collectionMap.contains(serverKey( host, port ));
    if( m_collectionMap.contains(serverKey( host, port )) ) //same server from multiple interfaces
        return;

   // debug() << "creating daap collection with" << host << ip << port;
    QPointer<DaapCollection> coll( new DaapCollection( host, ip, port ) );
    connect( coll, SIGNAL( collectionReady() ), SLOT( slotCollectionReady() ) );
    connect( coll, SIGNAL( remove() ), SLOT( slotCollectionDownloadFailed() ) );
    m_collectionMap.insert( serverKey( host, port ), coll );
}

//DaapCollection

DaapCollection::DaapCollection( const QString &host, const QString &ip, quint16 port )
    : Collection()
    , m_host( host )
    , m_port( port )
    , m_ip( ip )
    , m_reader( 0 )
    , m_mc( new MemoryCollection() )
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
    return new MemoryQueryMaker( m_mc.toWeakRef(), collectionId() );
}

QString
DaapCollection::collectionId() const
{
    return "daap://" + m_ip + ':' + m_port;
}

QString
DaapCollection::prettyName() const
{
    QString host = m_host;
    // No need to be overly verbose
    if( host.endsWith( ".local" ) )
        host = host.remove( QRegExp(".local$") );
    return i18n("Music share at %1", host);
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
    DEBUG_BLOCK
    emit remove();
}

#include "DaapCollection.moc"

