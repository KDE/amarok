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

#include "UserLabelsRequest.h"
#include "WebService.h"
#include "WebService/XmlRpc.h"
#include "UnicornCommon.h"

#include <QDebug>

#include <time.h>

UserLabelsRequest::UserLabelsRequest()
    : Request( TypeUserLabels, "UserLabels" )
{
}


void UserLabelsRequest::start()
{
    XmlRpc xml_rpc;
    QString const challenge = The::webService()->challengeString();
    time_t now;
    time( &now );
    QString const time = QString::number( now );

    //qDebug() << "challenge: " << challenge;
    //qDebug() << "us: " << Settings::instance().currentUsername();
    //qDebug() << "username_web " << m_username;

    xml_rpc << m_username
            << time
            << UnicornUtils::md5Digest( (m_passwordMd5 + time).toUtf8() );

    setHost( "wsdev.audioscrobbler.com" );

    xml_rpc.setMethod( "getUserLabels" ); // always on wsdev, set in Request.cpp

    qDebug() << "GetLabelsRequest: " << xml_rpc.toString();

    request( xml_rpc );
}


void UserLabelsRequest::success( QByteArray data )
{
    QList<QVariant> retVals;
    QString error;

    if ( !XmlRpc::parse( data, retVals, error ) )
    {
        setFailed( WebRequestResult_Custom, error );
        return;
    }

    if ( retVals.at( 0 ).type() != QVariant::List )
    {
        setFailed( WebRequestResult_Custom, "Result wasn't a QMap, no labels?" );
        qDebug() << retVals.at ( 0 ).typeName();
        return;
    }

    QVariantList list = retVals.at( 0 ).toList();
    qDebug() << list;

    if ( list.count() == 0 )
    {
        qDebug() << "No labels got.";
        return;
    }

    for ( QVariantList::iterator i = list.begin(); i != list.end(); i++ )
    {
        QMap<QString, QVariant> map = (*i).toMap();
        if ( map.count() != 2 )
                qDebug() << "Internal map has strange count.";

        QMap<QString, QVariant>::iterator j = map.begin();
        m_labels.insert( j.value().toInt(), ( j+1 ).value().toString() );
    }
    qDebug() << m_labels;

    /*if (map.contains( "faultCode" ))
    {
            QString faultString = map.value( "faultString" ).toString();
            setFailed( WebRequestResult_Custom, faultString );
            return;
    }*/

    //m_labels = map;
}

