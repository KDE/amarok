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

#include "Request.h"


SimilarArtistsRequest::SimilarArtistsRequest( QString artist )
        : Request( TypeSimilarArtists, "SimilarArtists" )
        , m_artist( artist )
{}

void
SimilarArtistsRequest::start()
{
    get( "/1.0/get.php?resource=artist&document=similar&format=xml&artist=" + QUrl::toPercentEncoding( m_artist ) );
}
    
void
SimilarArtistsRequest::success( QByteArray data )
{
    QDomDocument xml;
    xml.setContent( data );
    
    QDomNamedNodeMap attr = xml.elementsByTagName( "similarartists" ).item( 0 ).attributes();
    
    QString artist = attr.namedItem( "artist" ).nodeValue();
    QString image = attr.namedItem( "picture" ).nodeValue();
    //bool streamable = attr.namedItem( "streamable" ).nodeValue() == "1";

    m_artists += WeightedString::weighted( artist, 100 );
    
    QDomNodeList values = xml.elementsByTagName( "artist" );
    
    for (int i = 0; i < values.count(); i++)
    {
        QDomNode n = values.item( i );
        
        QDomNode item = n.namedItem( "name" );
        QDomNode match = n.namedItem( "match" );
        QDomNode image = n.namedItem( "image_small" );
        
        m_artists += WeightedString( item.toElement().text(), match.toElement().text().toInt() );
        m_images += image.toElement().text();
    }
}
