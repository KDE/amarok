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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "FingerprintQueryRequest.h"

#include "CachedHttp.h"
#include "UnicornCommon.h"
#include "Settings.h"

#include "logger.h"

#include <QDebug>
#include <QFileInfo>

#include <ctime>


FingerprintQueryRequest::FingerprintQueryRequest() :
    Request(TypeFingerprintQuery,"FingerprintQuery"),
    m_fullFpRequested( false )
{
}


FingerprintQueryRequest::FingerprintQueryRequest( TrackInfo track, QByteArray& data ) :
    Request(TypeFingerprintQuery,"FingerprintQuery"),
    m_fullFpRequested( false )
{
    m_track = track;

    setData( data );
}


void
FingerprintQueryRequest::start()
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

    QString baseUrl = "http://www.last.fm/fingerprint/query/";
    QString url = baseUrl +
        "?artist=" + QUrl::toPercentEncoding( m_track.artist() ) +
        "&album=" + QUrl::toPercentEncoding( m_track.album() ) +
        "&track=" + QUrl::toPercentEncoding( m_track.track() ) +
        "&duration=" + QString::number( m_track.duration() ) +
        "&mbid=" + QUrl::toPercentEncoding( m_track.mbId() ) +
        "&filename=" + QUrl::toPercentEncoding( QFileInfo( m_track.path() ).completeBaseName() ) +
        "&tracknum=" + QString::number( m_track.trackNr() ) +
        "&username=" + username() +
        "&sha256=" + sha256() +
        "&time=" + time +
        "&auth=" + authMd5 +
        "&authlower=" + authMd5Lower +
        "&fpversion=" + QUrl::toPercentEncoding( fpVersion() ) +
        "&fulldump=false";

    #ifndef QT_NO_DEBUG
        // This causes the fp to not be written to the database. Use
        // during debugging only.
        //url += "&noupdate=true";
    #endif

    // TODO: add
    //genre
    //samplerate

    qDebug() << url;

    QHttpRequestHeader header( "POST", url, 1, 1 );
    setHost( "www.last.fm", 80 );
    header.setValue( "Host", "www.last.fm" );
    header.setValue( "Content-Type", "multipart/form-data; boundary=----------------------------8e61d618ca16" );
    header.setValue( "Accept", "*/*" );

    QByteArray bytes;
    bytes.append( "------------------------------8e61d618ca16\r\n" );
    bytes.append( "Content-Disposition: " );
    bytes.append( "form-data; name=\"fpdata\"" );
    bytes.append( "\r\n\r\n" );

    bytes.append( data() );

    bytes.append( "\r\n");
    bytes.append( "------------------------------8e61d618ca16--\r\n");

    int contentLength = bytes.length();
    header.setContentLength(contentLength);

    post( header, bytes );
}


void
FingerprintQueryRequest::success( QByteArray data )
{
    // Data will consist of a number and a string.
    // The number is the fpid and the string is either FOUND or NEW
    // (or NOT FOUND when noupdate was used). NEW means we should
    // schedule a full fingerprint.
    //
    // In the case of an error, there will be no initial number, just
    // an error string.

    qDebug() << data;

    QString response( data );
    QStringList list = response.split( ' ' );

    if ( list.isEmpty() )
    {
        setFailed( Fingerprint_QueryError, "No data returned" );
        return;
    }

    QString fpid = list.at( 0 );
    bool isANumber;
    fpid.toUInt( &isANumber );
    if ( isANumber )
    {
        setFpId( fpid );
    }
    else
    {
        // It's an error
        setFailed( Fingerprint_QueryError, response );
        return;
    }

    QString status = list.at( 1 );
    setFullFpRequested( status == "NEW" );
}
