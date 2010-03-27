/****************************************************************************************
 * Copyright (c) 2006 Ian Monroe <ian@monroe.nu>                                        *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "contentfetcher.h"
#include "core/support/Debug.h"
#include "hasher.h"

#include <QBuffer>
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

QByteArray
ContentFetcher::results()
{
    QByteArray read = readAll();
    QHttpResponseHeader header = lastResponse();
    if( header.value( "Content-Encoding" ) == "gzip" )
    {
        QBuffer* bytes = new QBuffer( &read );
        QIODevice* stream = KFilterDev::device( bytes, "application/x-gzip", false );
        if ( !stream->open( QIODevice::ReadOnly ) )
            return read;

        //do not assign directly to read, see the documentation of the QBuffer ctor
        QByteArray filteredRead = stream->readAll();
        delete stream;
        delete bytes;
        read = filteredRead;
    }
    return read;
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
        debug() << "there is an error? " << error() << " " << errorString();
        m_selfDestruct = true;
        emit httpError( errorString() );
    }
}


#include "contentfetcher.moc"

