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

#include "TrackUploadRequest.h"

#include "WebService/XmlRpc.h"

#include <QUrl>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

TrackUploadRequest::TrackUploadRequest ()
    : Request (TypeTrackUpload, "TrackUploadRequest")
{
}

void TrackUploadRequest::start()
{
    QFile file(QDir::tempPath() + "lastfm-uploader/" + track().album() + "/" + track().track() + ".mp3");
    
    QByteArray data = file.readAll ();
    
    QString path = QString( "http://labels.last.fm:8090/fileupload/clientupload.php?label=%1&filename=%2" ).arg( label() ).arg( QUrl::toPercentEncoding( QString ("%1 - %2").arg( track().artist() ).arg( track().track() ) ).data() );
    
    qDebug() << "posting: " << path;
    
    post ( path, data );
}

/*void TrackUploadRequest::success( QByteArray data )
{
    QList<QVariant> retVals;
    QString error;

    if (!XmlRpc::parse( data, retVals, error )) {
        setFailed( WebRequestResult_Custom, error );
        return;
    }

    // There was previously no proper fault struct being returned so we're
    // checking for error (item not found etc) by seeing whether the returned
    // param is a map (struct) or not.
    if (retVals.at( 0 ).type() != QVariant::Map) {
        setFailed( WebRequestResult_Custom, "Result wasn't a <struct>, track not found?" );
        return;
    }

    QMap<QString, QVariant> map = retVals.at( 0 ).toMap();

    if (map.contains( "faultCode" )) {
        QString faultString = map.value( "faultString" ).toString();
        setFailed( WebRequestResult_Custom, faultString );
        return;
    }
}*/

