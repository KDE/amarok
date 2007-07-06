/* This file is part of the KDE project
   Copyright (C) 2007 Bart Cerneels <bart.cerneels@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include <QDebug>
#include <QMap>

#include "PodcastReader.h"
#include "PodcastMetaBase.h"
#include "debug.h"

PodcastReader::PodcastReader( PodcastCollection * collection )
        : QXmlStreamReader()
        , m_collection( collection )
{}

PodcastReader::~PodcastReader()
{
}

bool PodcastReader::read ( QIODevice *device )
{
    DEBUG_BLOCK
    setDevice( device );

    while ( !atEnd() )
    {
        readNext();

        if ( isStartElement() )
        {
            if ( name() == "rss" && attributes().value ( "version" ) == "2.0" )
            {
                readRss();
            }
            else
            {
                raiseError ( QObject::tr ( "The file is not an RSS version 2.0 file." ) );
            }
        }
    }

    return !error();
}

void PodcastReader::readRss()
{
    DEBUG_BLOCK
    Q_ASSERT ( isStartElement() && name() == "rss" );

    while ( !atEnd() )
    {
        readNext();

        if ( isEndElement() )
            break;

        if ( isStartElement() )
        {
            if ( name() == "channel" )
                readChannel();
            else
                readUnknownElement();
        }
    }
}

void PodcastReader::readChannel()
{
    DEBUG_BLOCK
    Q_ASSERT ( isStartElement() && name() == "channel" );
    PodcastAlbumPtr album = PodcastAlbumPtr( new PodcastAlbum() );
    PodcastArtistPtr artist = PodcastArtistPtr( new PodcastArtist( "testcast" ) );

    m_collection->acquireReadLock();

    while ( !atEnd() )
    {
        readNext();

        if ( isEndElement() )
            break;

        if ( isStartElement() )
        {
            if ( name() == "title" )
                album->setTitle( readTitle() );
            else if ( name() == "description" )
                readDescription();
            else if ( name() == "link" )
                readLink();
            else if ( name() == "item" )
            {
                PodcastTrackPtr item = readItem();
                Q_ASSERT( album );
                item->setAlbum( AlbumPtr::dynamicCast( album ) );
                album->addTrack( TrackPtr::dynamicCast( item ) );
                Q_ASSERT( artist );
                item->setArtist( ArtistPtr::dynamicCast( artist ) );
                artist->addTrack( TrackPtr::dynamicCast( item ) );
                m_collection->addTrack( item->name(), TrackPtr::dynamicCast( item ) );
            }
            else
                readUnknownElement();
        }
    }
    m_collection->addAlbum( album->name(), AlbumPtr::dynamicCast( album ) );
    m_collection->releaseLock();
}

PodcastTrackPtr
PodcastReader::readItem()
{
    DEBUG_BLOCK
    Q_ASSERT ( isStartElement() && name() == "item" );

    PodcastTrackPtr item = PodcastTrackPtr( new PodcastTrack() );

    while ( !atEnd() )
    {
        readNext();

        if ( isEndElement() )
            break;

        if ( isStartElement() )
        {
            if ( name() == "title" )
                item->setTitle( readTitle() );
            else if ( name() == "description" )
                item->setComment( readDescription() );
            else if ( name() == "link" )
                debug() << "link: " << readLink() << endl;
            else if ( name() == "pubDate" )
                debug() << "year: " << readPubDate() << endl;
            else if ( name() == "guid" )
                debug() << "guid: " << readGuid() << endl;
            else if ( name() == "enclosure" )
                readEnclosure();
            else
                readUnknownElement();
        }
    }

    return item;
}

QString
PodcastReader::readTitle()
{
    DEBUG_BLOCK
    Q_ASSERT ( isStartElement() && name() == "title" );

    return readElementText();
}

QString
PodcastReader::readDescription()
{
    DEBUG_BLOCK
    Q_ASSERT ( isStartElement() && name() == "description" );

    return readElementText();
}

QString
PodcastReader::readLink()
{
    DEBUG_BLOCK
    Q_ASSERT ( isStartElement() && name() == "link" );
    return readElementText();
}

QString
PodcastReader::readEnclosure()
{
    DEBUG_BLOCK
    Q_ASSERT ( isStartElement() && name() == "enclosure" );
    //TODO: need to get the url argument here
    return readElementText();
}

QString
PodcastReader::readGuid()
{
    DEBUG_BLOCK
    Q_ASSERT ( isStartElement() && name() == "guid" );
    return readElementText();
}

QString
PodcastReader::readPubDate()
{
    DEBUG_BLOCK
    Q_ASSERT ( isStartElement() && name() == "pubDate" );
    return readElementText();
}

void PodcastReader::readUnknownElement()
{
    DEBUG_BLOCK
    Q_ASSERT ( isStartElement() );

    debug() << "unknown element: " << name().toString() << endl;

    while ( !atEnd() )
    {
        readNext();

        if ( isEndElement() )
            break;

        if ( isStartElement() )
            readUnknownElement();
    }
}
