/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
 *      Erik Jaelevik, Last.fm Ltd <erik@last.fm>                          *
 *      Max Howell, Last.fm Ltd <max@last.fm>                              *
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

#include <QMessageBox>
#include <QSettings>

#include "logger.h"
#include "Settings.h"
#include "WebService/Request.h"
#include "WebService.h"


WebService::WebService( QObject* parent ) :
        QObject( parent ),
        m_isAutoDetectedProxy( false ),
		m_proxyPort( 0 ),
        m_isSubscriber( false )
{
    LOGL( 3, "Initialising Web Service" );
    
    //TODO needs to be a setting, to manually set the proxy or automatically
    if( !SharedSettings::instance()->isUseProxy() )
        autoDetectProxy();
}

void
WebService::requestResult( Request *r )
{
    Q_ASSERT( r );
   
    switch (r->type())
    {
        case TypeHandshake:
        { 
            //TODO mxcl before I refactored, the handshake success if block was still
            // executed, but the result code was set to an error, I considered this a
            // bug and hopefully I'll remember to ask someone if that is the case or not
    
            Handshake *handshake = static_cast<Handshake*>(r);

            Q_ASSERT( m_username == handshake->username() );
            Q_ASSERT( m_password == handshake->password() );

            if (handshake->succeeded()) {
                m_streamUrl = handshake->streamUrl(); // legacy
                m_isSubscriber = handshake->isSubscriber();
            }
            
            if (handshake->isMessage())
                QMessageBox::information( 
                        qApp->activeWindow(), 
                        tr( "Last.fm Information" ),
                        handshake->message() );            
            
            emit handshakeResult( handshake );
            
            break;
        }

        // these macros makes the code more readable
        #define CASE( T ) case Type##T: { T##Request *request = static_cast<T##Request*>(r);
        #define break } break

        CASE( ChangeStation )
            StationUrl url = request->stationUrl();
            QString name = request->stationName();

            emit changeStationResult( request );
            
            if (r->succeeded())
                emit stationChanged( url, name );
            break;
       
        CASE( SetTag )
            emit setTagResult( request );
            break;

        CASE( ProxyTest )
            if ( request->succeeded() )
            {
                m_isAutoDetectedProxy = request->proxyUsed();
                emit proxyTestResult( m_isAutoDetectedProxy );
            }
            break;
        
        default:
            ;
    }
    
    /// these are emitted only for the currentUsername()
    switch (r->type()) 
    {
        CASE( Love )
            emit loved( request->track() );
            break;
        CASE( UnLove )
            emit unloved( request->track() );
            break;
        CASE( Ban )
            emit banned( request->track() );
            break;
        CASE( UnBan )
            emit unbanned( request->track() );
            break;
        CASE( UnListen )
            emit unlistened( request->track() );
            break;
        CASE( UserTags )
            if (request->username() == currentUsername())
                emit userTags( request->tags() );
            break;
        CASE( DeleteFriend )
            emit friendDeleted( request->deletedUsername() );
            break;
        CASE( Friends )
            emit friends( request->usernames() );
            break;
        CASE( Neighbours )
            emit neighbours( request->usernames() );
            break;
        CASE( RecentTracks )
            emit recentTracks( request->tracks() );
            break;
        CASE( RecentlyLovedTracks )
            emit recentLovedTracks( request->tracks() );
            break;
        CASE( RecentlyBannedTracks )
            emit recentBannedTracks( request->tracks() );
            break;
            
        default:
            ;            
    }
   
    #undef CASE
    #undef break

    if ( r->failed() && r->resultCode() != Request_ProxyAuthenticationRequired )
        emit failure( r );
    else        
        emit success( r );

    emit result( r );

    
    if (r->autoDelete())
        // do last in case one of the signals is queued
        r->deleteLater();
}

QString
WebService::challengeString()
{
    uint const unixTime = QDateTime::currentDateTime().toTime_t();
    return QString::number( unixTime );
}


#ifdef Q_WS_MAC
    #include "UnicornCommon.h"
    #include <SystemConfiguration/SystemConfiguration.h> 
#endif

void
WebService::autoDetectProxy()
{
    Q_DEBUG_BLOCK;

    #ifdef Q_WS_MAC
        CFNumberRef enableNum;
        int enable;

        // Get the dictionary.
        CFDictionaryRef proxyDict = SCDynamicStoreCopyProxies( NULL );
        bool result = (proxyDict != NULL);
    
        // Get the enable flag.  This isn't a CFBoolean, but a CFNumber.
        if (result) {
            enableNum = (CFNumberRef) CFDictionaryGetValue( proxyDict, kSCPropNetProxiesHTTPEnable );
            result = (enableNum != NULL) && (CFGetTypeID(enableNum) == CFNumberGetTypeID());
        }
        
        if (result)
            result = CFNumberGetValue( enableNum, kCFNumberIntType, &enable ) && (enable != 0);
        
        // Get the proxy host.  DNS names must be in ASCII.  If you 
        // put a non-ASCII character  in the "Secure Web Proxy"
        // field in the Network preferences panel, the CFStringGetCString
        // function will fail and this function will return false.
        
        CFStringRef hostStr;
        
        if (result) {
            hostStr = (CFStringRef) CFDictionaryGetValue( proxyDict, kSCPropNetProxiesHTTPProxy );
            result = (hostStr != NULL) && (CFGetTypeID(hostStr) == CFStringGetTypeID());
        }
        if (result)
            m_proxyHost = UnicornUtils::CFStringToQString( hostStr );


    ////// Get the proxy port.
        int portInt;
        CFNumberRef portNum;
        
        if (result) {
            portNum = (CFNumberRef) CFDictionaryGetValue( proxyDict, kSCPropNetProxiesHTTPPort );
            result = (portNum != NULL) && (CFGetTypeID(portNum) == CFNumberGetTypeID());
        }
        if (result)
            result = CFNumberGetValue( portNum, kCFNumberIntType, &portInt );
        
        if (result)
            m_proxyPort = portInt;
    
    ////// Set that we found the proxy information
        m_isAutoDetectedProxy = true;
    
    ////// Clean up.
        
        if (proxyDict != NULL)
            CFRelease( proxyDict );
    #elif defined Q_WS_WIN
        #undef QSettings
        QString inetSettingPrefix = "CurrentVersion/Internet Settings/";
        QSettings autoProxySettings( QSettings::NativeFormat, QSettings::UserScope, "Microsoft", "Windows", this );
        
        m_isAutoDetectedProxy = autoProxySettings.value( inetSettingPrefix + "ProxyEnable", "" ).toBool();
        
        if( m_isAutoDetectedProxy )
        {
            QString proxySettings = autoProxySettings.value( inetSettingPrefix + "ProxyServer", "" ).toString();
            QStringList proxySettingList = proxySettings.split( ':' );
            m_proxyHost = proxySettingList[0];
            m_proxyPort = proxySettingList[1].toInt();
        }
    #endif
    
//     qDebug() << m_proxyHost << ':' << m_proxyPort;
}
