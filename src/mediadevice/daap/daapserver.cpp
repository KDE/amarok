/***************************************************************************
 * copyright            : (C) 2006 Ian Monroe <ian@monroe.nu>              *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "amarok.h"
#include "debug.h"
#include "daapserver.h"
#include "collectiondb.h"

#include <kstandarddirs.h>
#include <kuser.h>
#if DNSSD_SUPPORT
    #include <dnssd/publicservice.h>
#endif
DaapServer::DaapServer(QObject* parent, char* name)
  : QObject( parent, name )
  , m_service( 0 )
{
    DEBUG_BLOCK

    m_server = new KProcIO();
    m_server->setComm( KProcess::All );
    *m_server << "amarok_daapserver.rb";
    *m_server << locate( "data", "amarok/ruby_lib/" );
    *m_server << locate( "lib", "ruby_lib/" );
    *m_server << locate( "data", "amarok/scripts/ruby_debug/debug.rb" );
    if( !m_server->start( KProcIO::NotifyOnExit, true ) ) {
        error() << "Failed to start amarok_daapserver.rb" << endl;
        return;
    }

    connect( m_server, SIGNAL( readReady( KProcIO* ) ), this, SLOT( readSql() ) );
}

DaapServer::~DaapServer()
{
    #if DNSSD_SUPPORT
        delete m_service;
    #endif
    delete m_server;
}

void
DaapServer::readSql()
{
    static const QCString sqlPrefix = "SQL QUERY: ";
    static const QCString serverStartPrefix = "SERVER STARTING: ";
    QString line;
    while( m_server->readln( line ) != -1 )
    {
        if( line.startsWith( sqlPrefix ) )
        {
            line.remove( 0, sqlPrefix.length() );
            debug() << "sql run " << line << endl;
            m_server->writeStdin( CollectionDB::instance()->query( line ).join("\n") );
            m_server->writeStdin( "**** END SQL ****" );
        }
        else if( line.startsWith( serverStartPrefix ) )
        {
            line.remove( 0, serverStartPrefix.length() );
            debug() << "Server starting on port " << line << '.' << endl;
            #if DNSSD_SUPPORT
                KUser current;
                if( !m_service )
                    m_service = new DNSSD::PublicService( i18n("%1's Amarok Share").arg( current.fullName() ), "_daap._tcp", line.toInt() );
                    debug() << "port number: " << line.toInt() << endl;
                m_service->publishAsync();
            #endif
        }
        else
            debug() << "server says " << line << endl;
   }
   //m_server->ackRead();
   //m_server->enableReadSignals(true);
}

#include "daapserver.moc"

