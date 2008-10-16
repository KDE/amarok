/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Max Howell, Last.fm Ltd <max@last.fm>                              *
 *      Jono Cole, Last.fm Ltd <jono@last.fm>                              *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "logger.h"
#include "Request.h"
#include "WebService.h"
#include "XmlRpc.h"
#include "UnicornCommon.h"
#include "Settings.h"

#include <QApplication>
#include <QHttpRequestHeader>

QString Request::m_baseHost;
QString Request::s_language;


Request::Request( RequestType type, const char *name, CachedHttp::ProxyOverrideState proxyOverride )
        : QObject( The::webService() )
        , m_http( 0 )
        , m_result( Request_Undefined )
        , m_responseHeaderCode( 0 )
        , m_auto_delete( true )
        , m_type( type )
        , m_override_cursor( false )
{
    setObjectName( name );

    QString host = baseHost();
    if (host.isEmpty())
        host = qApp->arguments().contains( "--debug" )
                ? "wsdev.audioscrobbler.com"
                : "ws.audioscrobbler.com";

    m_http = new CachedHttp( host, 80, this, proxyOverride );

    connect( m_http, SIGNAL(dataAvailable( QByteArray )), SLOT(onSuccessPrivate( QByteArray )) );
    connect( m_http, SIGNAL(errorOccured( int, QString )), SLOT(onFailurePrivate( int, QString )) );        
    connect( m_http, SIGNAL(responseHeaderReceived( QHttpResponseHeader )), SLOT(onHeaderReceivedPrivate( QHttpResponseHeader )) );

    connect( this, SIGNAL(result( Request* )), The::webService(), SLOT(requestResult( Request* )), Qt::QueuedConnection );

//////
    // a 10 second timeout seems ages to the user -- mxcl
    // also keep in mind timeout is time without a response header! so this is ages for our servers
    m_timeout_timer.setInterval( 10 * 1000 );

    // this increases each tryAgain()
    m_retry_timer.setInterval( 0 );

    m_timeout_timer.setSingleShot( true );
    m_retry_timer.setSingleShot( true );

    connect( &m_timeout_timer, SIGNAL(timeout()), SLOT(tryAgain()) );
    connect( &m_retry_timer, SIGNAL(timeout()), SLOT(start()) );
}


Request::~Request()
{
    #ifndef Q_WS_MAC
    if (qApp && m_override_cursor)
        // we may be deleted by QApplication instance
        // but this function crashes if we are being deleted in that way
        QApplication::restoreOverrideCursor();
    #endif
}


void
Request::abort()
{
    m_http->abort();
}


void
Request::onHeaderReceivedPrivate( const QHttpResponseHeader& header )
{
    m_timeout_timer.stop();

    m_responseHeaderCode = header.statusCode();

    // Let subclass have a stab
    bool handled = headerReceived( header );

    if ( !handled )
    {
        if ( m_responseHeaderCode != 200 )
        {
            switch( m_responseHeaderCode )
            {
                case 403:
                {
                    setFailed( Request_WrongUserNameOrPassword );
                }
                break;

                default:
                    // TODO eg 503 suggests we should try again shortly
                    m_result = Request_BadResponseCode;
            }

        }
    }
}


void
Request::onFailurePrivate( int error_code, const QString &error_string )
{
    m_timeout_timer.stop();

    QDebugBlock block( QString(__PRETTY_FUNCTION__) + " - " + objectName() );

    LOG( 1, objectName() << "\n" <<
        "  Http response: " << m_http->lastResponse().statusCode() << "\n" <<
        "  QHttp error code: " << error_code << "\n" <<
        "  QHttp error text: " << error_string << "\n" <<
        "  Request: " << m_http->host() + m_http->currentRequest().path() << "\n" <<
        "  Bytes returned: " << m_http->bytesAvailable() << "\n"
        );

    qDebug() << error_code << error_string;

    switch (error_code)
    {
        case QHttp::NoError: // can't happen
            Q_ASSERT( !"can't happen" );
            break;

        case QHttp::Aborted: // aborted with abort();
            m_result = Request_Aborted;
            break;

        case QHttp::HostNotFound:
            m_result = Request_HostNotFound;
            break;

        #if QT_VERSION >= 0x040300
        case QHttp::ProxyAuthenticationRequiredError:
            m_result = Request_ProxyAuthenticationRequired;
            break;
        #endif

        case QHttp::ConnectionRefused:
        case QHttp::UnexpectedClose:
        case QHttp::InvalidResponseHeader:
        case QHttp::UnknownError:
        case QHttp::WrongContentLength:
            tryAgain();
            return; //don't emit result
    }

    emit result( this );
}


void
Request::onSuccessPrivate( QByteArray data )
{
//     LOG( 3, objectName() << " request returned:\n" <<
//         "  " << data.data() << "\n" );

    if ( m_result == Request_Undefined )
    {
        m_data = data;
        m_result = Request_Success;

        // This is optionally implemented by subclasses
        success( data );
    }

    if ( !m_retry_timer.isActive() )
        emit result( this );

}


void
Request::setHost( QString path, int port )
{
    m_http->setHost( path, port );
}


void
Request::get( QString path )
{
    QHttpRequestHeader header( "GET", path );
    header.setValue( "Host", m_http->host() );
    header.setValue( "Accept-Language",
        UnicornUtils::lfmLangCodeToIso639( language() ) + ", en" );
    m_http->request( header );

    qDebug() << objectName() << "initiated:" << ( m_http->host() + path );

    m_timeout_timer.start();
}


void
Request::post( QString path, QByteArray& data )
{
    QHttpRequestHeader header( "POST", path );
    header.setValue( "Host", m_http->host() );
    header.setValue( "Accept-Language",
                     UnicornUtils::lfmLangCodeToIso639( language() ) + ", en" );
    post( header, data );

    qDebug() << objectName() << "initiated multipart post:" << ( m_http->host() + path );

    m_timeout_timer.start();
}


void
Request::post( QHttpRequestHeader& header, QByteArray& data )
{
    m_http->request( header, data );

    qDebug() << objectName() << "initiated multipart post:" << ( m_http->host() + header.value("host") );

    m_timeout_timer.start();
}


void
Request::request( const XmlRpc &xmlrpc )
{
    QHttpRequestHeader header( "POST", "/1.0/rw/xmlrpc.php" );
    header.setValue( "Host", m_http->host() );
    header.setValue( "Accept-Language",
        UnicornUtils::lfmLangCodeToIso639( language() ) + ", en" );
    header.setContentType( "text/xml" );

    QString const xml = xmlrpc.toString();

    m_http->request( header, xml.toUtf8(), 0 /*to QIoDevice */,  xmlrpc.useCache() );

    qDebug() << objectName() << "initiated:" << ( m_http->host() + header.path() );
/*    LOG( 3, objectName() << " request xmlrpc:\n" <<
        "  " << xml << "\n" );*/

    m_timeout_timer.start();
}


void
Request::tryAgain()
{
    int const ms = m_retry_timer.interval();

    m_retry_timer.setInterval( ms + 500 );

    if (ms == 0)
        // special case, don't use timer method as at very least that
        // causes when event loop iteration, which is relatively slow
        // when user is pissed at slow last.fm software
        start();

    else if (ms <= 500)
        // stop trying at 500 = 3 attempts in total
        m_retry_timer.start();

    else {
        m_result = Request_NoResponse;

        // We disconnect the signal to prevent onFailurePrivate from emitting 
        // result as it would then happen twice.
        disconnect( m_http, SIGNAL(errorOccured( int, QString )), this, SLOT(onFailurePrivate( int, QString )) );
        abort();
        connect( m_http, SIGNAL(errorOccured( int, QString )), SLOT(onFailurePrivate( int, QString )) );

        emit result( this );
    }
}


QString
Request::parameter( QString keyName, QString data )
{
    QStringList list = data.split( '\n' );

    for ( int i = 0; i < list.size(); i++ )
    {
        QStringList values = list[ i ].split( '=' );
        if ( values[0] == keyName )
        {
            values.removeAt( 0 );
            return QString::fromUtf8( values.join( "=" ).toAscii() );
        }
    }

    return QString();
}


QString
Request::errorMessage() const
{
    switch ( m_result )
    {
        case Request_NoResponse:
        case Request_BadResponseCode:
            return tr( "The Last.fm servers are temporarily overloaded, please try again in a moment." );

        case Request_HostNotFound:
            return tr( "Cannot contact the Last.fm server. Is your Internet connection configured correctly?" );

        case Request_Aborted:
            return tr( "The web request was cancelled." );

        case Request_WrongUserNameOrPassword:
            return tr( "Could not connect to server. Wrong username or password." );

        default:
            return m_error;
    }
}


void
Request::setOverrideCursor()
{
    #ifndef Q_WS_MAC
    // Mac busy cursor is basically unsupported and thus ugly
    m_override_cursor = true;
    QApplication::setOverrideCursor( Qt::BusyCursor );
    #endif
}
