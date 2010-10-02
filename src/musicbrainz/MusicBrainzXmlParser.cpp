/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
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

#define DEBUG_PREFIX "MusicBrainzXmlParser"

#include "MusicBrainzXmlParser.h"

#include "core/support/Debug.h"

MusicBrainzXmlParser::MusicBrainzXmlParser( QString &doc )
                    : ThreadWeaver::Job()
                    , m_doc( "musicbrainz" )
{
    m_doc.setContent( doc );
}

void
MusicBrainzXmlParser::run()
{
    DEBUG_BLOCK
    QDomElement docElem = m_doc.documentElement();
    parseElement( docElem );
}

int
MusicBrainzXmlParser::type()
{
    return m_type;
}

void
MusicBrainzXmlParser::parseElement( const QDomElement &e )
{
    QString elementName = e.tagName();
    if( elementName == "track-list" )
    {
        m_type = TrackList;
        parseTrackList( e );
    }
    else if( elementName == "release" )
    {
        m_type = Release;
        parseRelease( e );
    }
    else if( elementName == "track" )
    {
        m_type = Track;
        parseTrack( e );
    }
    else
        parseChildren( e );
}

void
MusicBrainzXmlParser::parseChildren( const QDomElement &e )
{
    QDomNode child = e.firstChild();
    while( !child.isNull() )
    {
        if( child.isElement() )
            parseElement( child.toElement() );
        child = child.nextSibling();
    }
}

QStringList
MusicBrainzXmlParser::parseTrackList( const QDomElement &e )
{
    QDomNode dNode = e.firstChild();
    QDomElement dElement;
    QStringList list;

    while( !dNode.isNull() )
    {
        if( dNode.isElement() )
        {
            dElement = dNode.toElement();

            if( dElement.tagName() == "track" )
                list << parseTrack( dElement );
        }
        dNode = dNode.nextSibling();
    }
    return list;
}

QString
MusicBrainzXmlParser::parseTrack( const QDomElement &e )
{
    QString id;
    MusicBrainzTrack *track;

    if( e.hasAttribute( "id" ) )
    {
        id = e.attribute( "id" );
        if( id.isEmpty() )
            return QString();
        if( tracks.contains( id ) )
            track = new MusicBrainzTrack( tracks.value( id ) );
        else
            track = new MusicBrainzTrack( id );
        if( !track )
            return id;
    }
    else return QString();

    QDomNode dNode = e.firstChild();
    QDomElement dElement;
    QString elementName;

    while( !dNode.isNull() )
    {
        if( dNode.isElement() )
        {
            dElement = dNode.toElement();
            elementName = dElement.tagName();

            if( elementName == "title" )
                track->setTitle( dElement.text() );
            else if( elementName == "duration" )
                track->setLength( dElement.text().toInt() );
            else if( elementName == "artist" )
                track->setArtistId( parseArtist( dElement ) );
            else if( elementName == "release-list" )
            {
                parseReleaseList( dElement );
                for( int i = 0; i < m_curReleaseList.count(); i++ )
                    track->addRelease( m_curReleaseList[i], m_curOffsetList[i] );
            }
        }
        dNode = dNode.nextSibling();
    }

    tracks.insert( id, *track );
    return id;
}

void
MusicBrainzXmlParser::parseReleaseList( const QDomElement &e )
{
    m_curReleaseList.clear();
    m_curOffsetList.clear();

    QDomNode dNode = e.firstChild();
    QDomElement dElement;

    while( !dNode.isNull() )
    {
        if( dNode.isElement() )
        {
            dElement = dNode.toElement();

            if( dElement.tagName() == "release" )
                parseRelease( dElement );
        }
        dNode = dNode.nextSibling();
    }
}

void
MusicBrainzXmlParser::parseRelease( const QDomElement &e )
{
    QString id;
    MusicBrainzRelease *release;

    if( e.hasAttribute( "id" ) )
    {
        id = e.attribute( "id" );
        if( id.isEmpty() )
            return;
        if( releases.contains( id ) )
            release = new MusicBrainzRelease( releases.value( id ) );
        else
            release = new MusicBrainzRelease( id );
        if( !release )
            return;
    }
    else
        return;

    if( e.hasAttribute( "type" ) )
        release->setType( e.attribute( "type" ) );

    QDomNode dNode = e.firstChild();
    QDomElement dElement;
    QString elementName;

    while( !dNode.isNull() )
    {
        if( dNode.isElement() )
        {
            dElement = dNode.toElement();
            elementName = dElement.tagName();

            if( elementName == "title" )
                release->setTitle( dElement.text() );
            else if( elementName == "text-representation" && dElement.hasAttribute( "language" ) )
                release->setLanguage( dElement.attribute( "language" ) );
            else if( elementName == "artist" )
                release->setArtistId( parseArtist( dElement ) );
            else if( elementName == "track-list" )
            {
                if( dElement.hasAttribute( "offset" ) )
                {
                    m_curReleaseList.append( id );
                    m_curOffsetList.append( dElement.attribute( "offset" ).toInt() );
                    parseTrackList( dElement );
                }
                else
                {
                    QStringList trackList;
                    int duration = 0;
                    trackList << parseTrackList( dElement );
                    foreach( QString track, trackList )
                    {
                        release->addTrack( track );
                        duration += tracks.value( track ).length();
                    }
                    release->setLength( duration );
                }
            }
            else if( elementName == "release-event-list" )
                release->setYear( parseReleaseEventList( dElement ) );
        }
        dNode = dNode.nextSibling();
    }

    releases.insert( id, *release );
}

int
MusicBrainzXmlParser::parseReleaseEventList( const QDomElement &e )
{
    QDomNode dNode = e.firstChild();
    QDomElement dElement;
    QList < int > years;

    while( !dNode.isNull() )
    {
        if( dNode.isElement() )
        {
            dElement = dNode.toElement();

            if( dElement.tagName() == "event" )
                years << parseReleaseEvent( dElement );
        }
        dNode = dNode.nextSibling();
    }

    if( years.empty() )
        return 0;

    qSort( years );
    return years.first();
}

int
MusicBrainzXmlParser::parseReleaseEvent( const QDomElement &e )
{
    int year;
    if( e.hasAttribute( "date" ) )
    {
        QRegExp yearCutter( "^(\\d{4}).*$" );
        if( yearCutter.exactMatch( e.attribute( "date" ) ) )
            year = yearCutter.cap( 1 ).toInt();
        else
            year = 0;

    }
    else
        year = 0;
    return year;
}

QString
MusicBrainzXmlParser::parseArtist( const QDomElement &e )
{
    QString id;
    MusicBrainzArtist *artist;

    if( e.hasAttribute( "id" ) )
    {
        id = e.attribute( "id" );
        if( id.isEmpty() )
            return QString();
        if( artists.contains( id ) )
            artist = new MusicBrainzArtist( artists.value( id ) );
        else
            artist = new MusicBrainzArtist( id );
        if( !artist )
            return id;
    }
    else return QString();

    if( e.hasAttribute( "type" ) )
        artist->setType( e.attribute( "type" ) );

    QDomNode dNode = e.firstChild();
    QDomElement dElement;
    QString elementName;
    while( !dNode.isNull() )
    {
        if( dNode.isElement() )
        {
            dElement = dNode.toElement();
            elementName = dElement.tagName();

            if( elementName == "name" )
                artist->setName( dElement.text() );
            else if( elementName == "sort-name" )
                artist->setSortName( dElement.text() );
            else if( elementName == "disambiguation" )
                artist->setDescription( dElement.text() );
            else if( elementName == "life-span" )
            {
                if( dElement.hasAttribute( "begin" ) )
                    artist->setDateBegin( dElement.attribute( "begin" ) );
                if( dElement.hasAttribute( "end" ) )
                    artist->setDateEnd( dElement.attribute( "end" ) );
            }
            else if( elementName == "release-list" )
                parseReleaseList( dElement );
        }
        dNode = dNode.nextSibling();
    }

    artists.insert( id, *artist );
    return id;
}
