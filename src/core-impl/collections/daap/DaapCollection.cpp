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
#include "core/logger/Logger.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "DaapMeta.h"
#include "MemoryQueryMaker.h"
#include "daapreader/Reader.h"

#include <QStringList>
#include <QTimer>

#include <KLocalizedString>

#include <KDNSSD/RemoteService>
#include <KDNSSD/ServiceBase>
#include <KDNSSD/ServiceBrowser>

using namespace Collections;

DaapCollectionFactory::DaapCollectionFactory()
    : Collections::CollectionFactory()
    , m_browser( nullptr )
{
}

DaapCollectionFactory::~DaapCollectionFactory()
{
    delete m_browser;
}

void
DaapCollectionFactory::init()
{
    DEBUG_BLOCK
    switch( KDNSSD::ServiceBrowser::isAvailable() )
    {
    case KDNSSD::ServiceBrowser::Working:
        //don't block Amarok's startup by connecting to DAAP servers
        QTimer::singleShot( 1000, this, &DaapCollectionFactory::connectToManualServers );
        m_browser = new KDNSSD::ServiceBrowser(QStringLiteral("_daap._tcp"));
        m_browser->setObjectName(QStringLiteral("daapServiceBrowser"));
        connect( m_browser, &KDNSSD::ServiceBrowser::serviceAdded,
                 this, &DaapCollectionFactory::foundDaap );
        connect( m_browser, &KDNSSD::ServiceBrowser::serviceRemoved,
                 this, &DaapCollectionFactory::serverOffline );
        m_browser->startBrowse();
        break;

    case KDNSSD::ServiceBrowser::Stopped:
        debug() << "The Zeroconf daemon is not running";
        break;

    case KDNSSD::ServiceBrowser::Unsupported:
        debug() << "Zeroconf support is not available";
        break;

    default:
        debug() << "Unknown error with Zeroconf";
    }
    m_initialized = true;
}

void
DaapCollectionFactory::connectToManualServers()
{
    DEBUG_BLOCK
    QStringList sl = AmarokConfig::manuallyAddedServers();
    for( const QString &server : sl )
    {
        debug() << "Adding server " << server;
        QStringList current = server.split( QLatin1Char(':'), Qt::KeepEmptyParts );
        //handle invalid urls gracefully
        if( current.count() < 2 )
            continue;
            
        QString host = current.first();
        quint16 port = current.last().toUShort();
        Amarok::Logger::longMessage(
                    i18n( "Loading remote collection from host %1", host),
                    Amarok::Logger::Information );

        int lookup_id = QHostInfo::lookupHost( host, this, &DaapCollectionFactory::resolvedManualServerIp );
        m_lookupHash.insert( lookup_id, port );
    }
}

void
DaapCollectionFactory::serverOffline( KDNSSD::RemoteService::Ptr service )
{
    DEBUG_BLOCK
    QString key =  serverKey( service->hostName(), service->port() );
    if( m_collectionMap.contains( key ) )
    {
        auto coll = m_collectionMap[ key ];
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
DaapCollectionFactory::foundDaap( KDNSSD::RemoteService::Ptr service )
{
    DEBUG_BLOCK

    connect( service.data(), &KDNSSD::RemoteService::resolved, this, &DaapCollectionFactory::resolvedDaap );
    service->resolveAsync();
}

void
DaapCollectionFactory::resolvedDaap( bool success )
{
    const KDNSSD::RemoteService* service =  dynamic_cast<const KDNSSD::RemoteService*>(sender());
    if( !success || !service ) return;
    debug() << service->serviceName() << ' ' << service->hostName() << ' ' << service->domain() << ' ' << service->type();

    int lookup_id = QHostInfo::lookupHost( service->hostName(), this, &DaapCollectionFactory::resolvedServiceIp );
    m_lookupHash.insert( lookup_id, service->port() );
}

QString
DaapCollectionFactory::serverKey( const QString& host, quint16 port) const
{
    return host + QLatin1Char(':') + QString::number( port );
}

void
DaapCollectionFactory::slotCollectionReady()
{
    DEBUG_BLOCK
    DaapCollection *collection = dynamic_cast<DaapCollection*>( sender() );
    if( collection )
    {
        disconnect( collection, &DaapCollection::remove, this, &DaapCollectionFactory::slotCollectionDownloadFailed );
        Q_EMIT newCollection( collection );
    }
}

void
DaapCollectionFactory::slotCollectionDownloadFailed()
{
    DEBUG_BLOCK
    DaapCollection *collection = qobject_cast<DaapCollection*>( sender() );
    if( !collection )
        return;
    disconnect( collection, &DaapCollection::collectionReady, this, &DaapCollectionFactory::slotCollectionReady );
    for( const auto &it : m_collectionMap )
    {
        if( it.data() == collection )
        {
            m_collectionMap.remove( m_collectionMap.key( it ) );
            break;
        }
    }
    collection->deleteLater();
}

void
DaapCollectionFactory::resolvedManualServerIp( const QHostInfo &hostInfo )
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
    connect( coll, &DaapCollection::collectionReady, this, &DaapCollectionFactory::slotCollectionReady );
    connect( coll, &DaapCollection::remove, this, &DaapCollectionFactory::slotCollectionDownloadFailed );
}

void
DaapCollectionFactory::resolvedServiceIp( const QHostInfo &hostInfo )
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
    connect( coll, &DaapCollection::collectionReady, this, &DaapCollectionFactory::slotCollectionReady );
    connect( coll, &DaapCollection::remove, this, &DaapCollectionFactory::slotCollectionDownloadFailed );
    m_collectionMap.insert( serverKey( host, port ), coll.data() );
}

//DaapCollection

DaapCollection::DaapCollection( const QString &host, const QString &ip, quint16 port )
    : Collection()
    , m_host( host )
    , m_port( port )
    , m_ip( ip )
    , m_reader( nullptr )
    , m_mc( new MemoryCollection() )
{
    debug() << "Host: " << host << " port: " << port;
    m_reader = new Daap::Reader( this, host, port, QString(), this, "DaapReader" );
    connect( m_reader, &Daap::Reader::passwordRequired,this, &DaapCollection::passwordRequired );
    connect( m_reader, &Daap::Reader::httpError, this, &DaapCollection::httpError );
    m_reader->loginRequest();
}

DaapCollection::~DaapCollection()
{
}

QueryMaker*
DaapCollection::queryMaker()
{
    return new MemoryQueryMaker( m_mc.toWeakRef(), collectionId() );
}

QString
DaapCollection::collectionId() const
{
    return QString( QStringLiteral("daap://") + m_ip + QLatin1Char(':') ) + QString::number( m_port );
}

QString
DaapCollection::prettyName() const
{
    QString host = m_host;
    // No need to be overly verbose
    if( host.endsWith( QStringLiteral(".local") ) )
        host = host.remove( QRegularExpression(QStringLiteral(".local$")) );
    return i18n("Music share at %1", host);
}

void
DaapCollection::passwordRequired()
{
    //get password
    QString password;
    delete m_reader;
    m_reader = new Daap::Reader( this, m_host, m_port, password, this, "DaapReader" );
    connect( m_reader, &Daap::Reader::passwordRequired, this, &DaapCollection::passwordRequired );
    connect( m_reader, &Daap::Reader::httpError, this, &DaapCollection::httpError );
    m_reader->loginRequest();
}

void
DaapCollection::httpError( const QString &error )
{
    DEBUG_BLOCK
    debug() << "Http error in DaapReader: " << error;
    Q_EMIT remove();
}

void
DaapCollection::serverOffline()
{
    Q_EMIT remove();
}

void
DaapCollection::loadedDataFromServer()
{
    DEBUG_BLOCK
    Q_EMIT collectionReady();
}

void
DaapCollection::parsingFailed()
{
    DEBUG_BLOCK
    Q_EMIT remove();
}
