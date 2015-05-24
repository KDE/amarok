/****************************************************************************************
 * Copyright (c) 2010 Rick W. Chen <stuffcorpse@archlinux.us>                           *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "LastFmEventXmlParser.h"


LastFmEventXmlParser::LastFmEventXmlParser( QXmlStreamReader &reader )
    : m_xml( reader )
{
}

LastFmEventXmlParser::~LastFmEventXmlParser()
{}

bool
LastFmEventXmlParser::read()
{
    while( !m_xml.atEnd() && !m_xml.hasError() )
    {
        m_xml.readNext();
        if( m_xml.isStartElement() && m_xml.name() == "event" )
        {
            QHash<QString, QString> artists;
            LastFmEventPtr event( new LastFmEvent );
            while( !m_xml.atEnd() )
            {
                m_xml.readNext();
                const QStringRef &n = m_xml.name();
                if( m_xml.isEndElement() && n == "event" )
                    break;

                if( m_xml.isStartElement() )
                {
                    const QXmlStreamAttributes &a = m_xml.attributes();
                    if( n == "title" )
                        event->setName( m_xml.readElementText() );
                    else if( n == "artists" )
                        artists = readEventArtists();
                    else if( n == "venue" )
                    {
                        LastFmVenueXmlParser venueParser( m_xml );
                        if( venueParser.read() )
                            event->setVenue( venueParser.venue() );
                    }
                    else if( n == "startDate" )
                        event->setDate( KDateTime::fromString( m_xml.readElementText(), "%a, %d %b %Y %H:%M:%S" ) );
                    else if( n == "image" && a.hasAttribute("size") )
                        event->setImageUrl( LastFmEvent::stringToImageSize(a.value("size").toString()), QUrl( m_xml.readElementText() ) );
                    else if( n == "url" )
                        event->setUrl( QUrl( m_xml.readElementText() ) );
                    else if( n == "attendance" )
                        event->setAttendance( m_xml.readElementText().toInt() );
                    else if( n == "cancelled" )
                        event->setCancelled( bool( m_xml.readElementText().toInt() ) );
                    else if( n == "tags" )
                        event->setTags( readEventTags() );
                    else
                        m_xml.skipCurrentElement();
                }
            }
            event->setHeadliner( artists.value("headliner") );
            event->setParticipants( artists.values("artist") );
            m_events << event;
        }
    }
    return !m_xml.error();
}

QStringList
LastFmEventXmlParser::readEventTags()
{
    QStringList tags;
    while( !m_xml.atEnd() )
    {
        m_xml.readNext();
        if( m_xml.isEndElement() && m_xml.name() == "tags" )
            break;

        if( m_xml.isStartElement() )
        {
            if( m_xml.name() == "tag" )
                tags << m_xml.readElementText();
            else
                m_xml.skipCurrentElement();
        }
    }
    return tags;
}

QHash<QString, QString>
LastFmEventXmlParser::readEventArtists()
{
    QHash<QString, QString> artists;
    while( !m_xml.atEnd() )
    {
        m_xml.readNext();
        if( m_xml.isEndElement() && m_xml.name() == "artists" )
            break;

        if( m_xml.isStartElement() )
        {
            if( m_xml.name() == "artist" )
                artists.insertMulti( "artist", m_xml.readElementText() );
            else if( m_xml.name() == "headliner" )
                artists.insert( "headliner", m_xml.readElementText() );
            else
                m_xml.skipCurrentElement();
        }
    }
    return artists;
}


LastFmVenueXmlParser::LastFmVenueXmlParser( QXmlStreamReader &reader )
    : m_xml( reader )
{}

bool
LastFmVenueXmlParser::read()
{
    m_venue = new LastFmVenue;
    while( !m_xml.atEnd() && !m_xml.hasError() )
    {
        m_xml.readNext();
        const QStringRef &n = m_xml.name();
        if( m_xml.isEndElement() && n == "venue" )
            break;

        if( m_xml.isStartElement() )
        {
            const QXmlStreamAttributes &a = m_xml.attributes();
            if( n == "id" )
                m_venue->id = m_xml.readElementText().toInt();
            else if( n == "name" )
                m_venue->name = m_xml.readElementText();
            else if( n == "location" )
            {
                LastFmLocationXmlParser locationParser( m_xml );
                if( locationParser.read() )
                    m_venue->location = locationParser.location();
            }
            else if( n == "url" )
                m_venue->url = QUrl( m_xml.readElementText() );
            else if( n == "website" )
                m_venue->website = QUrl( m_xml.readElementText() );
            else if( n == "phonenumber" )
                m_venue->phoneNumber = m_xml.readElementText();
            else if( n == "image" && a.hasAttribute("size") )
            {
                LastFmEvent::ImageSize size = LastFmEvent::stringToImageSize( a.value("size").toString() );
                m_venue->imageUrls[ size ] = QUrl( m_xml.readElementText() );
            }
            else
                m_xml.skipCurrentElement();
        }
    }
    return !m_xml.error();
}

LastFmVenueXmlParser::~LastFmVenueXmlParser()
{}

LastFmLocationXmlParser::LastFmLocationXmlParser( QXmlStreamReader &reader )
    : m_xml( reader )
{}

bool
LastFmLocationXmlParser::read()
{
    m_location = new LastFmLocation;
    while( !m_xml.atEnd() && !m_xml.hasError() )
    {
        m_xml.readNext();
        if( m_xml.isEndElement() && m_xml.name() == "location" )
            break;

        if( m_xml.isStartElement() )
        {
            if( m_xml.name() == "city" )
                m_location->city = m_xml.readElementText();
            else if( m_xml.name() == "country" )
                m_location->country = m_xml.readElementText();
            else if( m_xml.name() == "street" )
                m_location->street = m_xml.readElementText();
            else if( m_xml.name() == "postalcode" )
                m_location->postalCode = m_xml.readElementText();
            else if( m_xml.prefix() == "geo" && m_xml.name() == "point" )
                readGeoPoint();
            else
                m_xml.skipCurrentElement();
        }
    }
    return !m_xml.error();
}

LastFmLocationXmlParser::~LastFmLocationXmlParser()
{}

void
LastFmLocationXmlParser::readGeoPoint()
{
    while( !m_xml.atEnd() && !m_xml.hasError() )
    {
        m_xml.readNext();
        if( m_xml.isEndElement() && m_xml.name() == "point" )
            break;

        if( m_xml.isStartElement() )
        {
            if( m_xml.name() == "lat" )
                m_location->latitude = m_xml.readElementText().toDouble();
            else if( m_xml.name() == "long" )
                m_location->longitude = m_xml.readElementText().toDouble();
            else
                m_xml.skipCurrentElement();
        }
    }
}
