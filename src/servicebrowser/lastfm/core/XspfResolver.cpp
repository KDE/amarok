/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
 *      Erik Jalevik, Last.fm Ltd <erik@last.fm>                           *
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

#include "XspfResolver.h"
#include "MooseCommon.h"

#include "WebService/XmlRpc.h"
#include "logger.h"

#include <QDomDocument>

// This doesn't currently do any resolving. It simply copies info from
// the XSPF playlist into TrackInfo objects. When we get local playback
// integrated, this should prefer locally available tracks.
QList<TrackInfo>
XspfResolver::resolveTracks( const QByteArray& xspf )
{
    Q_DEBUG_BLOCK;

    QList<TrackInfo> tracks;

    // Try and parse the XML
    QDomDocument doc;
    QString errorMsg;
    int errorLine;
    if ( !doc.setContent( xspf, &errorMsg, &errorLine ) )
    {
        QString error = QString( "Couldn't parse XSPF.\nError: %1\nLine: %2" ).
            arg( errorMsg ).
            arg( errorLine );
        
        LOGL( 1, error );
        throw ParseException( error );
    }

    QDomElement docElem = doc.documentElement();

    // Get station name
    QDomNode title = docElem.namedItem( "title" );
    if ( !title.isNull() )
    {
        QDomElement e = title.toElement();
        if ( !e.isNull() )
        {
            m_station = e.text();

            // This is currently URL encoded, but that will probably change
            // NOTE I think this is fixed
            m_station = QUrl::fromEncoded( m_station.toUtf8() ).toString();
            
            // The conversion still retains the pluses for some reason
            m_station.replace( '+', ' ' );

            // Decode HTML entities like &amp;
            m_station = XmlRpc::unescape( m_station );
                        
            m_station = m_station.trimmed();
            if ( !m_station.isEmpty() )
                m_station[0] = m_station[0].toUpper();
        }
    }

    // Get skip count
    m_skipLimit = -1;
    QDomElement link = docElem.firstChildElement( "link" );
    while ( !link.isNull() )
    {
        if ( link.hasAttribute( "rel" ) &&
             link.attribute( "rel" ) == "http://www.last.fm/skipsLeft" )
        {
            bool conversionOk = false;
            m_skipLimit = link.text().toInt( &conversionOk );
            if ( !conversionOk )
            {
                LOGL( 2, "Failed to read skip limit" );
                m_skipLimit = -1;
            }
            break;
        }
        else
        {
            link = link.nextSiblingElement( "link" );
        }
    }

    // God I hate writing XML parsing code, it is truly the most tedious thing on earth.

    // Now, look for tracklist
    QDomNode trackList = docElem.namedItem( "trackList" );

    if ( trackList.isNull() )
    {
        QString error = QString( "Required XSPF node trackList missing." );
        
        LOGL( 1, error );
        throw ParseException( error );
    }

    QDomNode trackNode = trackList.firstChild();

    while ( !trackNode.isNull() )
    {
        if ( trackNode.nodeName() != "track" )
        {
            continue;
        }
        
        TrackInfo track;
        track.setSource( TrackInfo::Radio );
        
        track.setAuthCode( childText( trackNode, "lastfm:trackauth" ) );
        track.setTrack( childText( trackNode, "title" ) );
        track.setArtist( childText( trackNode, "creator" ) );
        track.setAlbum( childText( trackNode, "album" ) );
        track.setDuration( childText( trackNode, "duration" ).toInt() / 1000 ); // comes in ms

        track.setPowerPlayLabel( childText( trackNode, "lastfm:sponsored" ) );

        // Hacky workaround for tracks wrongly labelled as having duration 0 on the site
        if ( track.duration() == 0 )
        {
            // Make em a minute
            track.setDuration( 60 );
        }

        // There can be more than one location
        QStringList paths;
        QDomElement location = trackNode.firstChildElement( "location" );
        while ( !location.isNull() )
        {
            QDomElement e = location.toElement();
            if ( !e.isNull() )
            {
                paths << e.text();
            }
            location = location.nextSiblingElement( "location" );
        }
        track.setPaths( paths );

        // If parsing of any of these vital fields failed, just don't add
        // the track to the playlist.
        if ( !track.artist().isEmpty() &&
             !track.track().isEmpty() &&
             track.duration() != 0 &&
             !track.path().isEmpty() )
        {
            tracks << track;
        }

        trackNode = trackNode.nextSibling();
    }

    return tracks;
}

QString
XspfResolver::childText( QDomNode parent, QString tagName )
{
    // This is safe because if the node is not an element, the cast will
    // cast it to a null element whose text() function will return an
    // empty string.
    return parent.namedItem( tagName ).toElement().text();
}
