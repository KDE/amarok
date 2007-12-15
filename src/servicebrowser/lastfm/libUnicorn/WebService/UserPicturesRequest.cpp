/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Max Howell, Last.fm Ltd <max@last.fm>                              *
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

#include "logger.h"
#include "Request.h"
#include "XmlRpc.h"


UserPicturesRequest::UserPicturesRequest()
        : Request( TypeUserPictures, "UserPictures" )
{}


void
UserPicturesRequest::start()
{
    QString size;
    switch( imageSize() )
    {
        case SIZE_PAGE:
            size = "page";
            break;

        case SIZE_LARGE:
            size = "large";
            break;

        case SIZE_MEDIUM:
            size = "medium";
            break;

        default:
            size = "small";
            break;
    }

    XmlRpc xmlrpc;
    xmlrpc.setMethod( "getUserAvatars" );
    xmlrpc << names();
    xmlrpc << size;
    request( xmlrpc );
}


void
UserPicturesRequest::success( QByteArray data )
{
    QList<QVariant> retVals;
    QString error;
    bool parsed = XmlRpc::parse( data, retVals, error );

    if ( !parsed )
    {
        setFailed( WebRequestResult_Custom, "Couldn't parse" ); //TODO mxcl proof XmlRpc Parsing should be handled by baseclass
        LOGL( 1, error );
        return;
    }

    if ( retVals.at( 0 ).type() != QVariant::List )
    {
        setFailed( WebRequestResult_Custom, "Result wasn't an <array>." ); //TODO mxcl setError should log
        return;
    }

    QList<QVariant> array = retVals.at( 0 ).toList();

    foreach( QVariant val, array )
    {
        QMap<QString, QVariant> map = val.toMap();

        QString user = map.value( "name" ).toString();
        QString url = map.value( "avatar" ).toString();

        m_urls.insert( user, url );
    }
}
