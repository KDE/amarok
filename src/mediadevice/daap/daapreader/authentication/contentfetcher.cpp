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

using namespace Daap;
int ContentFetcher::s_requestId = 10;

ContentFetcher::ContentFetcher( const QString & hostname, Q_UINT16 port, QObject * parent, const char * name )
 : QHttp(hostname, port, parent, name)
 , m_hostname( hostname )
{
}

ContentFetcher::~ContentFetcher()
{ }

/*QDataStream*
ContentFetcher::dataStream() const
{
    return m_dataStream;
}

void
ContentFetcher::clearStream()
{
    if( m_dataStream )
    {
        delete m_dataStream->device();
        m_dataStream->unsetDevice();
        delete m_dataStream;
        m_dataStream = 0;
    }
}*/

void
ContentFetcher::getDaap( const QString & command )
{
//    DEBUG_BLOCK
    QHttpRequestHeader header( "GET", command );
    char hash[33] = {0};
    GenerateHash(3, reinterpret_cast<const unsigned char*>(command.ascii()), 2, reinterpret_cast<unsigned char*>(hash), 0 /*s_requestId*/);
    header.setValue( "Host", m_hostname + ":3689" );
    header.setValue( "Client-DAAP-Request-ID", "0"/*QString::number( s_requestId )*/ );
    header.setValue( "Client-DAAP-Access-Index", "2" );
    header.setValue( "Client-DAAP-Validation", hash );
    header.setValue( "Client-DAAP-Version", "3.0" );
    header.setValue( "User-Agent", "iTunes/4.6 (Windows; N)" );
    header.setValue( "Accept", "*/*" );
 //   header.setValue( "Accept-Encoding", "gzip" );

    request( header );
//    debug() << hash << ' ' << command.ascii() << ' ' << reinterpret_cast<const unsigned char*>(command.ascii()) << endl;
}

#include "contentfetcher.moc"

