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


SimilarTagsRequest::SimilarTagsRequest( QString tag )
        : TagsRequest( TypeSimilarTags, "SimilarTags" )
        , m_tag( tag )
{}
    
void
SimilarTagsRequest::start()
{
    XmlRpc xmlrpc;
    xmlrpc.setMethod( "getSimilarTags" );
    xmlrpc << m_tag;
    request( xmlrpc );
}

void
SimilarTagsRequest::success( QByteArray data )
{
    QList<QVariant> retVals;
    QString error;
    
    bool parsed = XmlRpc::parse( data, retVals, error );

    if (!parsed || retVals.isEmpty()) {
        LOGL( 1, error );
        return;
    }

    if (retVals[0].type() != QVariant::Map)
        //TODO mxcl setError
        return;

    QMap<QString, QVariant> topMap = retVals.at( 0 ).toMap();

    if (topMap.contains( "faultCode" )) {
        QString faultString = topMap.value( "faultString" ).toString();
        LOGL( 2, faultString );
        return;
    }

    int wmax = 0;
    foreach(QVariant v, topMap.value( "tags" ).toList()) {
        QMap<QString, QVariant> map = v.toMap();

        int w = map.value( "weight" ).toInt();
        if (w > wmax)
            wmax = w;

        m_tags += WeightedString::weighted( map.value( "name" ).toString().toLower(), w );
    }

    QString searchTerm = topMap.value( "search" ).toString();
    // make the first item the search term with the max weighting
    m_tags.prepend( WeightedString::weighted( searchTerm , wmax ) );
}
