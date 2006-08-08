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

DaapServer::DaapServer(QObject* parent, char* name)
  : QObject( parent, name )
{
    DEBUG_BLOCK
    
    
    m_server = new amaroK::ProcIO();
    m_server->setComm( static_cast<KProcess::Communication>( KProcess::All ) );
    *m_server << "amarok_daapserver.rb";
    *m_server << QString("%1").arg( locate( "data", "amarok/ruby_lib/" ) );
    if( !m_server->start( KProcIO::NotifyOnExit, true ) ) {
        error() << "Failed to start amarok_daapserver.rb" << endl;
        return;
    }

    connect( m_server, SIGNAL( readReady( KProcIO* ) ), this, SLOT( readSql() ) );
}

DaapServer::~DaapServer()
{
    delete m_server;
}

void
DaapServer::readSql()
{
    DEBUG_BLOCK
    static const QCString prefix = "SQL QUERY: ";
    QString line;
    if( m_server->readln( line ) != -1 && line.startsWith( prefix ) )
    {
        line.remove( 0, prefix.length() );
        debug() << "sql run " << line << endl;
        m_server->writeStdin( CollectionDB::instance()->query( line ).join("\n") );
        m_server->writeStdin( "**** END SQL ****" );
    }
    else
        debug() << "not sql:  " << line << endl;
}

#include "daapserver.moc"
