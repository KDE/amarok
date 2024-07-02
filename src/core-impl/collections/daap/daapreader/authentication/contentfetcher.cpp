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
#include "network/NetworkAccessManagerProxy.h"

#include <QBuffer>
#include <QByteArray>
#include <QNetworkReply>

#include <KCodecs>
#include <KCompressionDevice>

using namespace Daap;
int ContentFetcher::s_requestId = 10;

ContentFetcher::ContentFetcher( const QString & hostname, quint16 port, const QString& password, QObject * parent, const char * name )
 : QObject(parent)
 , m_reply( nullptr )
 , m_hostname( hostname )
 , m_port( port )
 , m_selfDestruct( false )
{
    setObjectName( QLatin1String(name) );
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
    return m_lastResult;
}

void
ContentFetcher::getDaap( const QString &command, QIODevice* musicFile /*= 0*/ )
{
    QUrl url( command );
    url.setHost( m_hostname );
    url.setPort( m_port );
    QNetworkRequest request( url );
    char hash[33] = {0};
    const char *cmd = command.toLatin1();
    GenerateHash(3, reinterpret_cast<const unsigned char*>( cmd ), 2, reinterpret_cast<unsigned char*>(hash), 0 /*s_requestId*/);

    if( !m_authorize.isEmpty() )
    {
        request.setRawHeader( "Authorization", m_authorize );
    }

    request.setRawHeader( "Client-DAAP-Request-ID", "0"/*QString::number( s_requestId )*/ );
    request.setRawHeader( "Client-DAAP-Access-Index", "2" );
    request.setRawHeader( "Client-DAAP-Validation", hash );
    request.setRawHeader( "Client-DAAP-Version", "3.0" );
    request.setRawHeader( "User-Agent", "iTunes/4.6 (Windows; N)" );
    request.setRawHeader( "Accept", "*/*" );
    request.setRawHeader( "Accept-Encoding", "gzip" );

    m_reply = The::networkAccessManager()->sendCustomRequest( request, "GET", musicFile );

    if( m_reply->isFinished() )
        onFinished();
    else
        connect( m_reply, &QNetworkReply::finished, this, &ContentFetcher::onFinished );
}

void
ContentFetcher::onFinished()
{
    if( !m_reply )
        return;

    if( !m_selfDestruct && m_reply->error() )
    {
        if( m_reply->error() == QNetworkReply::AuthenticationRequiredError )
        {
            Q_EMIT loginRequired();
            return;
        }

        debug() << "there is an error? " << m_reply->error() << " " << m_reply->errorString();
        m_selfDestruct = true;
        Q_EMIT httpError( m_reply->errorString() );
    }

    QByteArray read = m_reply->readAll();
    if( m_reply->rawHeader( "Content-Encoding" ) == "gzip" )
    {
        QBuffer* bytes = new QBuffer( &read );
        KCompressionDevice *stream = new KCompressionDevice( bytes, true, KCompressionDevice::GZip );
        if ( stream->open( QIODevice::ReadOnly ) )
            m_lastResult = stream->readAll();
        else
            m_lastResult = read;

        delete stream;
    }
    else
        m_lastResult = read;

    Q_EMIT finished();
    m_reply->deleteLater();
    m_reply = nullptr;
}



