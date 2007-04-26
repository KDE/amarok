/* 
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

#include <k3resolver.h>

AMAROK_EXPORT_PLUGIN( DaapCollectionFactory )

DaapCollectionFactory::DaapCollectionFactory()
    : CollectionFactory()
{
    //nothing to do
}

DaapCollectionFactory::~DaapCollectionFactory()
{
    //nothing to do
}

void
DaapCollectionFactory::init()
{
    DEBUG_BLOCK
    QTimer::singleShot( 20000, this, SLOT( connectToManualServers() ) );
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

//DaapCollection

DaapCollection::DaapCollection( const QString &host, const QString &ip, quint16 port )
    : QObject()
    , Collection()
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
DaapCollection::queryBuilder()
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

#include "daapcollection.moc"

