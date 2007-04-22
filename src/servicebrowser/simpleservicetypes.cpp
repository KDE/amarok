/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#include "magnatunetypes.h"


//// SimpleServiceArtist ////

SimpleServiceArtist::SimpleServiceArtist( )
    : m_id(0)
{
    m_description = QString();
}

void SimpleServiceArtist::setId( int id )
{
    m_id = id;
}

int SimpleServiceArtist::getId( ) const
{
    return m_id;
}

void SimpleServiceArtist::setName( const QString &name )
{
    m_name = name;
}

QString SimpleServiceArtist::getName( ) const
{
    return m_name;
}

void SimpleServiceArtist::setDescription( const QString &description )
{
    m_description = description;
}

QString SimpleServiceArtist::getDescription( ) const
{ 
    return m_description;
}


//// SimpleServiceAlbum ////

SimpleServiceAlbum::SimpleServiceAlbum( )
    : m_id (0)
    , m_artistId (0)
{
}

void SimpleServiceAlbum::setId( int id )
{
    m_id = id;
}

int SimpleServiceAlbum::getId( ) const
{
    return m_id;
}

void SimpleServiceAlbum::setArtistId( int artistId )
{
    m_artistId = artistId;
}

int SimpleServiceAlbum::getArtistId( ) const
{
    return m_artistId;
}


void SimpleServiceAlbum::setName( const QString &name )
{
    m_name = name;
}

QString SimpleServiceAlbum::getName( ) const
{
    return m_name;
}

void SimpleServiceAlbum::setDescription(const QString description)
{
    m_description = description;
}

QString SimpleServiceAlbum::getDescription()
{
    return m_description;
}


//// SimpleServiceTrack ////

SimpleServiceTrack::SimpleServiceTrack( )
    : m_id( 0 )
    , m_trackNumber( 0 )
    , m_duration( 0 )
    , m_albumId( 0 )
{
}

void SimpleServiceTrack::setId( int id )
{
   m_id = id;
}

int SimpleServiceTrack::getId( ) const
{
    return m_id;
}
void SimpleServiceTrack::setAlbumId( int albumId )
{
    m_albumId = albumId;
}

int SimpleServiceTrack::getAlbumId( ) const
{
   return m_albumId;
}

void SimpleServiceTrack::setName( const QString &name )
{
    m_name = name;
}

QString SimpleServiceTrack::getName( ) const
{
   return m_name;
}

void SimpleServiceTrack::setTrackNumber( int trackNumber )
{
    m_trackNumber = trackNumber;
}

int SimpleServiceTrack::getTrackNumber( ) const
{
    return m_trackNumber;
}

void SimpleServiceTrack::setDuration( int duration )
{
    m_duration = duration;
}

int SimpleServiceTrack::getDuration( ) const
{
    return m_duration;
}

void SimpleServiceTrack::setURL( const QString &url )
{
    m_url = url;
}
 
QString SimpleServiceTrack::getURL( ) const
{
    return m_url;
}















