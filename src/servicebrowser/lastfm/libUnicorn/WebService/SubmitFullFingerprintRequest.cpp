/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Last.fm Ltd <client@last.fm>                                       *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "SubmitFullFingerprintRequest.h"

#include "CachedHttp.h"
#include "UnicornCommon.h"
#include "Settings.h"

#include <QFileInfo>

#include <ctime>


SubmitFullFingerprintRequest::SubmitFullFingerprintRequest()
    : Request(TypeSubmitFingerprint,"SubmitFingerprint")
{
}

SubmitFullFingerprintRequest::SubmitFullFingerprintRequest( TrackInfo track, QByteArray& data )
    : Request(TypeSubmitFingerprint,"SubmitFingerprint")
{
    m_track = track;
    
    setData( data );
}

void
SubmitFullFingerprintRequest::start()
{
    time_t now;
    time( &now );
    QString time = QString::number( now );
    //QString time = QString::number( 1186743888 );
    // Concatenate pw hash with time
    QString auth = passwordMd5() + time;
    QString authLower = passwordMd5Lower() + time;
    // Hash the concatenated string to create auth code
    QString authMd5 = UnicornUtils::md5Digest( auth.toUtf8() );
    QString authMd5Lower = UnicornUtils::md5Digest( authLower.toUtf8() );
    
    QString baseUrl = "http://"+http()->host()+"/fingerprint/upload.php";
    QString url = baseUrl
    +"?artist="+QUrl::toPercentEncoding( m_track.artist() )
    +"&album="+QUrl::toPercentEncoding( m_track.album() )
    +"&track="+QUrl::toPercentEncoding( m_track.track() )
    +"&duration="+QString::number( m_track.duration() )
    +"&mbid="+QUrl::toPercentEncoding( m_track.mbId() )
    +"&filename="+QUrl::toPercentEncoding( QFileInfo( m_track.path() ).completeBaseName() )
    +"&tracknum="+QString::number( m_track.trackNr() )
    +"&username="+username()
    +"&sha256="+sha256()
    +"&time="+time
    +"&auth="+authMd5
    +"&authlower="+authMd5Lower
    +"&fpversion="+QUrl::toPercentEncoding( fpVersion() );
    
    qDebug() << url;

    QHttpRequestHeader header( "POST", url, 1, 1 );
    header.setValue( "Host", http()->host() );
    header.setValue( "Content-type", "multipart/form-data, boundary=AaB03x" );
    header.setValue( "Cache-Control", "no-cache" );
    header.setValue( "Accept", "*/*" );
    
    QByteArray bytes;
    bytes.append( "--AaB03x\r\n" );
    bytes.append( "content-disposition: " );
    bytes.append( "form-data; name=\"agency\"\r\n" );
    bytes.append( "\r\n" );
    bytes.append( "0\r\n" );
    bytes.append( "--AaB03x\r\n" );
    bytes.append( "content-disposition: " );
    bytes.append( "form-data; name=\"fpdata\"; filename=\"fpdata\"\r\n" );
    bytes.append( "Content-Transfer-Encoding: binary\r\n" );
    bytes.append( "\r\n" );
    
    bytes.append( data() );
    
    bytes.append("\r\n");
    bytes.append("--AaB03x--");
    int contentLength = bytes.length();
    header.setContentLength(contentLength);
    
    post(header,bytes);
}
