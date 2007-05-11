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

#include <QBuffer>
#include <QDataStream>
#include <QHttp>
#include <QByteArray>

#include <kfilterdev.h>
#include <kcodecs.h>

using namespace Daap;
int ContentFetcher::s_requestId = 10;

ContentFetcher::ContentFetcher( const QString & hostname, quint16 port, const QString& password, QObject * parent, const char * name )
 : QHttp(hostname, port, parent)
 , m_hostname( hostname )
 , m_port( port )
 , m_selfDestruct( false )
{
    setObjectName( name );
    connect( this, SIGNAL( stateChanged( int ) ), this , SLOT( checkForErrors( int ) ) );
    QByteArray pass = password.toUtf8();
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
    //QByteArray read = readAll();
    //QBuffer* bytes = new QBuffer( &read );
    //QIODevice* stream =  KFilterDev::device( bytes, "application/x-gzip", false );
    //stream->open( QIODevice::ReadOnly );
    //QDataStream* ds = new QDataStream( stream );
    QDataStream* ds = new QDataStream( readAll() );
    return *ds;
}

void
ContentFetcher::getDaap( const QString & command, QIODevice* musicFile /*= 0*/ )
{
    QHttpRequestHeader header( "GET", command );
    char hash[33] = {0};
    const char *cmd = command.toAscii();
    GenerateHash(3, reinterpret_cast<const unsigned char*>( cmd ), 2, reinterpret_cast<unsigned char*>(hash), 0 /*s_requestId*/);

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
    //header.setValue( "Accept-Encoding", "gzip" );

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

