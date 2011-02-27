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

#include "core/meta/support/MetaConstants.h"
#include "core/support/Debug.h"
#include "MusicBrainzMeta.h"

MusicBrainzXmlParser::MusicBrainzXmlParser( QString &doc )
                    : ThreadWeaver::Job()
                    , m_doc( "musicbrainz" )
                    , m_type( 0 )
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
    QVariantMap track;

    if( e.hasAttribute( "id" ) )
    {
        id = e.attribute( "id" );
        if( id.isEmpty() )
            return QString();
        if( tracks.contains( id ) )
            track = tracks.value( id );
        else
            track.insert( MusicBrainz::TRACKID, id );
        if( track.isEmpty() )
            return id;
    }
    else
        return id;

    if( e.hasAttribute( "ext:score" ) )
        track.insert( Meta::Field::SCORE, e.attribute( "ext:score" ).toInt() );

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
                track.insert( Meta::Field::TITLE, dElement.text() );
            else if( elementName == "duration" )
            {
                int length = dElement.text().toInt();
                if( length > 0 )
                    track.insert( Meta::Field::LENGTH, length );
            }
            else if( elementName == "artist" )
            {
                track.insert( MusicBrainz::ARTISTID, parseArtist( dElement ) );
                track.insert( Meta::Field::ARTIST,
                              artists.value( track.value( MusicBrainz::ARTISTID ).toString() ) );
            }
            else if( elementName == "release-list" )
            {
                currentTrackOffsets.clear();
                track.insert( MusicBrainz::RELEASELIST, parseReleaseList( dElement ) );
                track.insert( MusicBrainz::TRACKOFFSET, currentTrackOffsets );
            }
        }
        dNode = dNode.nextSibling();
    }

    tracks.insert( id, track );
    return id;
}

QStringList
MusicBrainzXmlParser::parseReleaseList( const QDomElement &e )
{
    QDomNode dNode = e.firstChild();
    QDomElement dElement;
    QStringList list;

    while( !dNode.isNull() )
    {
        if( dNode.isElement() )
        {
            dElement = dNode.toElement();

            if( dElement.tagName() == "release" )
                list << parseRelease( dElement );
        }
        dNode = dNode.nextSibling();
    }

    return list;
}

QString
MusicBrainzXmlParser::parseRelease( const QDomElement &e )
{
    QString id;
    QVariantMap release;

    if( e.hasAttribute( "id" ) )
    {
        id = e.attribute( "id" );
        if( id.isEmpty() )
            return QString();
        if( releases.contains( id ) )
            release = releases.value( id );
        else
            release.insert( MusicBrainz::RELEASEID, id );
        if( release.isEmpty() )
            return id;
    }
    else
        return id;

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
            {
                QString releaseName = dElement.text();
                QRegExp diskNrMatcher( "^.*\\(disc (\\d+).*\\)$" );
                if( diskNrMatcher.exactMatch( releaseName ) )
                {
                    release.insert( Meta::Field::DISCNUMBER, diskNrMatcher.cap( 1 ) );
                    //releaseName.truncate( releaseName.indexOf( "(disc " ) );
                }

                release.insert( Meta::Field::TITLE, releaseName.trimmed() );
            }
            else if( elementName == "artist" )
            {
                QString artistID = parseArtist( dElement );
                if( !artistID.isEmpty() && artists.contains( artistID ) )
                {
                    release.insert( MusicBrainz::ARTISTID, artistID );
                    release.insert( Meta::Field::ARTIST,
                                    artists.value( artistID ) );
                }
            }
            else if( elementName == "track-list" && dElement.hasAttribute( "offset" ) )
            {
                int offset = dElement.attribute( "offset" ).toInt() + 1;
                if( offset > 0 )
                    currentTrackOffsets.insert( id, offset );
            }
            else if( elementName == "release-event-list" )
            {
                int year = parseReleaseEventList( dElement );
                if( year > 0 )
                    release.insert( Meta::Field::YEAR, year );
            }
        }
        dNode = dNode.nextSibling();
    }

    releases.insert( id, release );
    return id;
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
            {
                unsigned int year = parseReleaseEvent( dElement );
                if( year )
                    years.append( year );
            }
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
    int year = 0 ;
    if( e.hasAttribute( "date" ) )
    {
        QRegExp yearCutter( "^(\\d{4}).*$" );
        if( yearCutter.exactMatch( e.attribute( "date" ) ) )
            year = yearCutter.cap( 1 ).toInt();

    }
    return year;
}

QString
MusicBrainzXmlParser::parseArtist( const QDomElement &e )
{
    QString id;
    QString artist;

    if( e.hasAttribute( "id" ) )
    {
        id = e.attribute( "id" );
        if( id.isEmpty() )
            return QString();
    }
    else
        return id;

    QDomNode dNode = e.firstChild();
    QDomElement dElement;
    while( !dNode.isNull() )
    {
        if( dNode.isElement() )
        {
            dElement = dNode.toElement();

            if( dElement.tagName() == "name" )
                artist = dElement.text();
        }
        dNode = dNode.nextSibling();
    }

    artists.insert( id, artist );
    return id;
}

QVariantMap
MusicBrainzXmlParser::grabTrackByLength( const quint64 length )
{
    QString chosenTrack;
    quint64 min = length;
    quint64 difference = 0;
    foreach( QString trackID, tracks.keys() )
    {
        difference = qAbs< qint64 >( length - tracks.value( trackID ).value( Meta::Field::LENGTH ).toULongLong() );
        if( difference < min )
        {
            chosenTrack = trackID;
            min = difference;
        }
    }

    QVariantMap track = chosenTrack.isEmpty() ? tracks.values().first() : tracks.value( chosenTrack );
    QString releaseId = track.value( MusicBrainz::RELEASELIST ).toStringList().first();
    QVariantMap release = releases.value( releaseId );
    track.insert( MusicBrainz::RELEASEID, releaseId );
    track.insert( Meta::Field::ALBUM, release.value( Meta::Field::TITLE ).toString() );
    track.insert( Meta::Field::DISCNUMBER, release.value( Meta::Field::DISCNUMBER ).toInt() );
    track.insert( Meta::Field::TRACKNUMBER,
                  track.value( MusicBrainz::TRACKOFFSET ).toMap().value( releaseId ).toInt() );
    track.remove( MusicBrainz::RELEASELIST );
    track.remove( MusicBrainz::TRACKOFFSET );
    return track;
}

QVariantMap
MusicBrainzXmlParser::grabTrackByID( const QString &ID )
{
    if( !tracks.contains( ID ) )
        return QVariantMap();

    QVariantMap track = tracks.value( ID );
    QString releaseId = track.value( MusicBrainz::RELEASELIST ).toStringList().first();
    QVariantMap release = releases.value( releaseId );
    track.insert( MusicBrainz::RELEASEID, releaseId );
    track.insert( Meta::Field::ALBUM, release.value( Meta::Field::TITLE ).toString() );
    track.insert( Meta::Field::DISCNUMBER, release.value( Meta::Field::DISCNUMBER ).toInt() );
    track.insert( Meta::Field::TRACKNUMBER,
                  track.value( MusicBrainz::TRACKOFFSET ).toMap().value( releaseId ).toInt() );
    track.remove( MusicBrainz::RELEASELIST );
    track.remove( MusicBrainz::TRACKOFFSET );
    return track;
}
