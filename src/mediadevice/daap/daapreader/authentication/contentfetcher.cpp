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

#include "contentfetcher.h"
#include "debug.h"
#include "hasher.h"

#include <qbuffer.h>
#include <qsocket.h>
#include <qdatastream.h>
#include <qhttp.h>

#include <kfilterdev.h>
#include <kmdcodec.h>

using namespace Daap;
int ContentFetcher::s_requestId = 10;

ContentFetcher::ContentFetcher( const QString & hostname, Q_UINT16 port, const QString& password, QObject * parent, const char * name )
 : QHttp(hostname, port, parent, name)
 , m_hostname( hostname )
 , m_port( port )
 , m_selfDestruct( false )
{
    connect( this, SIGNAL( stateChanged( int ) ), this , SLOT( checkForErrors( int ) ) );
    QCString pass = password.utf8();
    if( !password.isNull() )
    {
        m_authorize = "Basic " + KCodecs::base64Encode( "none:" + pass );
    }
}

ContentFetcher::~ContentFetcher()
{ }

QDataStream&
ContentFetcher::results()
{
    QBuffer* bytes = new QBuffer( readAll() ); 
    QIODevice* stream =  KFilterDev::device( bytes, "application/x-gzip", false );
    stream->open( IO_ReadOnly );
    QDataStream* ds = new QDataStream( stream );
    return *ds;
}

void
ContentFetcher::getDaap( const QString & command, QIODevice* musicFile /*= 0*/ )
{
    QHttpRequestHeader header( "GET", command );
    char hash[33] = {0};
    GenerateHash(3, reinterpret_cast<const unsigned char*>(command.ascii()), 2, reinterpret_cast<unsigned char*>(hash), 0 /*s_requestId*/);

    if( !m_authorize.isEmpty() )
    {
        header.setValue( "Authorization", m_authorize );
    }

    header.setValue( "Host", m_hostname + QString::number( m_port ) );
    header.setValue( "Client-DAAP-Request-ID", "0"/*QString::number( s_requestId )*/ );
    header.setValue( "Client-DAAP-Access-Index", "2" );
    header.setValue( "Client-DAAP-Validation", hash );
    header.setValue( "Client-DAAP-Version", "3.0" );
    header.setValue( "User-Agent", "iTunes/4.6 (Windows; N)" );
    header.setValue( "Accept", "*/*" );
    header.setValue( "Accept-Encoding", "gzip" );

    request( header, 0, musicFile );
}

/**
 *  QHttp enjoys forgetting to emit a requestFinished when there's an error
 *  This gets around that.
 */
void
ContentFetcher::checkForErrors( int /*state*/ )
{
    if( !m_selfDestruct && error() != 0 )
    {
        debug() << "there is an error? " << error() << " " << errorString() << endl;
        m_selfDestruct = true;
        emit httpError( errorString() );
    }
}


#include "contentfetcher.moc"

