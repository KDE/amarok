/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
 *      Erik Jaelevik, Last.fm Ltd <erik@last.fm>                          *
 *      Jono Cole, Last.fm Ltd <jono@last.fm>                              *
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

#include "CachedHttp.h"
#include "Settings.h"
#include "WebService.h"

#include <QDir>
#include <QTimer>
#include <QCoreApplication>

#ifdef WIN32
    #include <windows.h>
#endif

#undef LOG
#undef LOGL
#define LOG(x, y)
#define LOGL(x, y)

QString CachedHttp::s_customUserAgent; // whole name
QString CachedHttp::s_customCachePath; // whole path


CachedHttp::CachedHttp( QObject* parent ) :
    RedirectHttp( parent ),
    m_dataID( -1 ),
    m_statuscode( 0 ),
    m_proxyOverride( NONE ),
    m_nextId( 0 ),
    m_inProgress( false )
{
    init();
}


CachedHttp::CachedHttp( const QString& hostName, int port, QObject* parent, ProxyOverrideState proxyOverride ) :
    RedirectHttp( parent ),
    m_dataID( -1 ),
    m_hostname( hostName ),
    m_statuscode( 0 ),
    m_proxyOverride( proxyOverride ),
    m_nextId( 0 ),
    m_inProgress( false )
{
    init();
    setHost( hostName, port );
}


CachedHttp::~CachedHttp()
{
    // EJ: Added this warning just to prevent stupidity like forgetting to
    // create the Http object on the stack instead of the heap.
    if ( m_inProgress )
    {
        qDebug() << "CachedHttp object destroyed while in progress:\n" <<
            m_hostname + currentRequest().path();
    }
}


void
CachedHttp::init()
{
    QDir( cachePath() ).mkdir( cachePath() ); //rofl@Qt.com

    applyProxy();

    connect( this, SIGNAL(requestFinished( int, bool )), this, SLOT(dataFinished( int, bool )) );
    connect( this, SIGNAL(responseHeaderReceived (const QHttpResponseHeader&)), this, SLOT(headerReceived (const QHttpResponseHeader&)) );
    connect( this, SIGNAL(done( bool )), this, SLOT(requestDone( bool )) );
}


void
CachedHttp::applyProxy()
{
    //TODO really this should be determined by an settingsservice key like autoDetectProxy()

    SharedSettings* settings = SharedSettings::instance();

    if ( settings->isUseProxy() ) {
        setProxy( settings->getProxyHost(),
                  settings->getProxyPort(),
                  settings->getProxyUser(),
                  settings->getProxyPassword() );
    }
    else if ( ( The::webService()->isAutoDetectedProxy() && m_proxyOverride != PROXYOFF ) ||
                m_proxyOverride == PROXYON ) {
        setProxy( The::webService()->proxyHost(),
                  The::webService()->proxyPort(),
                  QString(),
                  QString() );
    }
    else
    {
        setProxy( "", 0 );
    }
}


void
CachedHttp::applyUserAgent( QHttpRequestHeader& header )
{
    // NEVER CHANGE THIS STRING!
    // martin says we can append stuff if we like, just never change the first bit

    QString s = userAgent(); //"Last.fm Client " + The::settings().version();
  #ifdef WIN32
    s += " (Windows)";
  #elif defined (Q_WS_MAC)
    s += " (OS X)";
  #elif defined (Q_WS_X11)
    s += " (X11)";
  #endif

    header.setValue( "User-Agent", s );
}


int
CachedHttp::get( const QString& path, bool useCache )
{
    applyProxy();
    m_buffer.clear();
    QString url = m_hostname + path;

    if ( useCache && haveCachedCopy( url ) )
    {
        // Using a singleshot so that we can return an ID and have
        // the operation proceed asynchronously.
        m_cacheStack.push( CachedRequestData( ++m_nextId, url ) );
        QTimer::singleShot( 0, this, SLOT( getFromCache() ) );
        return m_nextId;
    }

    QHttpRequestHeader header( "GET", path );
    header.setValue( "Host", m_hostname );
    applyUserAgent( header );

    m_dataID = request( header );
    if ( useCache )
        m_requestStack.insert( m_dataID, CachedRequestData( ++m_nextId, url ) );

    m_inProgress = true;

    return m_dataID;
}


int
CachedHttp::get( const QString& path, QIODevice* to )
{
    applyProxy();
    m_inProgress = true;
    return RedirectHttp::get( path, to );
}


int
CachedHttp::post( const QString& path, QIODevice* data )
{
    applyProxy();
    m_buffer.clear();

    m_dataID = RedirectHttp::post( path, data );

    m_inProgress = true;

    return m_dataID;
}


int
CachedHttp::post( const QString& path, const QByteArray& data )
{
    applyProxy();
    m_buffer.clear();

    m_dataID = RedirectHttp::post( path, data );

    m_inProgress = true;

    return m_dataID;
}


int
CachedHttp::request( const QHttpRequestHeader& header, QIODevice* data, QIODevice* to )
{
    QHttpRequestHeader h( header );
    applyProxy();
    applyUserAgent( h );

    m_buffer.clear();
    m_dataID = RedirectHttp::request( h, data, to );

    m_inProgress = true;

    return m_dataID;
}


int
CachedHttp::request( const QHttpRequestHeader& header, const QByteArray& data, QIODevice* to, bool useCache )
{
    QHttpRequestHeader h( header );
    applyProxy();
    applyUserAgent( h );

    m_buffer.clear();
    QString key( data );
    if ( useCache && haveCachedCopy( key ) )
    {
        // Using a singleshot so that we can return an ID and have
        // the operation proceed asynchronously.
        m_cacheStack.push( CachedRequestData( ++m_nextId, key ) );
        QTimer::singleShot( 0, this, SLOT( getFromCache() ) );
        return m_nextId;
    }

    m_dataID = RedirectHttp::request( h, data, to );

    m_inProgress = true;

    if ( useCache )
        m_requestStack.insert( m_dataID, CachedRequestData( ++m_nextId, key ) );

    return m_dataID;
}


void
CachedHttp::headerReceived( const QHttpResponseHeader& resp )
{
    m_statuscode = resp.statusCode();

    m_expireDate = 0;
    if ( !resp.value( "expires" ).isEmpty() )
    {
        QString expire = resp.value( "expires" );
        QStringList datelist = expire.split( " " ); //Split the datestring

        if ( datelist.count() == 6 ) // 6 items in a regular expire-date
        {
            datelist.removeLast(); //Pop the timezone, always GMT
            datelist.removeFirst(); //Pop the weekday (Mon, Tue...), we rely on the date
            QString expdate = datelist.join( " " );

            m_expireDate = QDateTime::fromString( expdate, "dd MMM yyyy hh:mm:ss" ).toTime_t();
        }

        if ( m_expireDate == -1 ) //If it's -1 we need to change it to 0 so we can compare it to a time_t (uint)
        {
            m_expireDate = 0;
        }
    }

    if ( (unsigned int)m_expireDate < QDateTime::currentDateTime().toTime_t() ) //If the expiredate is not set, or if the expiredate is in the past.
    {
        LOG( Severity::Warning, "The webservice " + objectName() + " does not set expiredate or the expiredate is in the past.\n" );
        m_expireDate = QDateTime::currentDateTime().addDays( 7 ).toTime_t();
    }
}


void
CachedHttp::dataFinished( int id, bool error )
{
    if ( error )
        emit errorOccured( RedirectHttp::error(), RedirectHttp::errorString() );
    else
        if ( id == m_dataID )
        {
            checkBuffer();

            CachedRequestData req = m_requestStack.take( id );
            if ( !req.m_cacheKey.isEmpty() && m_statuscode == 200)
            {
                putCachedCopy( req.m_cacheKey, m_buffer );
            }

            emit dataAvailable( m_buffer );
        }
}


QString
CachedHttp::userAgent()
{
    if ( s_customUserAgent.isEmpty() )
    {
        return QCoreApplication::organizationName() + " " +
               QCoreApplication::applicationName();
    }
    else
    {
        return s_customUserAgent;
    }
}


QString
CachedHttp::cachePath()
{
    if ( s_customCachePath.isEmpty() )
    {
        return UnicornUtils::appDataPath() + "/" + QCoreApplication::organizationName() + "/" +
               QCoreApplication::applicationName() + "/cache";
    }
    else
    {
        return s_customCachePath;
    }
}


QString
CachedHttp::pathToCachedCopy( QString cacheKey )
{
    QString keyMd5 = UnicornUtils::md5Digest( qPrintable( cacheKey ) );

    // The c in the front is because we use a different cacheformat now.
    // (The 10 first bytes are the expire timestamp);
    return cachePath() + "c" + keyMd5; 
}


bool
CachedHttp::haveCachedCopy( QString url )
{
    if (!QFile::exists( pathToCachedCopy( url ) )) return false;
    if (!QFileInfo( pathToCachedCopy( url ) ).isReadable()) return false;

    QFile f( pathToCachedCopy( url ) );
    if ( !f.open( QIODevice::ReadOnly ) )
    {
        return false;
    }

    QByteArray expdate = f.read(10);
    f.close();
    if (expdate.toUInt() < QDateTime::currentDateTime().toTime_t())
    {
        return false;
    }
    else
    {
        return true;
    }
}


void
CachedHttp::putCachedCopy( QString url, const QByteArray& data )
{
    #ifdef WIN32
        if ( url.size() > MAX_PATH )
        {
            url.chop( url.size() - MAX_PATH );
        }
    #endif

    QFile f( pathToCachedCopy( url ) );

    QByteArray date = QByteArray::number ( m_expireDate );
    date = date.rightJustified(10,'0');

    if ( !f.open( QIODevice::WriteOnly ) )
    {
        //LOGL( 1, "Failed to open file " << url << " for writing to cache" );
        return;
    }

    f.write(date);
    f.write(data);
}


void CachedHttp::getFromCache()
{
    Q_ASSERT( !m_cacheStack.isEmpty() );

    // Pick next request off cache stack
    CachedRequestData req = m_cacheStack.pop();

    QFile f( pathToCachedCopy( req.m_cacheKey ) );
    if ( !f.open( QIODevice::ReadOnly ) )
    {
        //LOGL( 1, "Failed to open cached file, returning error" );

        // TODO: emit error
        emit done( true );
        return;
    }

    // Keeping it as UTF-8, conversion will the done in parse function
    QByteArray date = f.read(10);
    QByteArray result = f.readAll();
    emit dataAvailable( result );
    emit done( false );
}


void CachedHttp::abort()
{
    m_inProgress = false;
    RedirectHttp::abort();
}


void CachedHttp::requestDone( bool /*error*/ )
{
    m_inProgress = false;
}
