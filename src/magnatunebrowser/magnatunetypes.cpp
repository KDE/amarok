/*
  Copyright (c) 2006  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/



#include "magnatunetypes.h"


//// MagnatuneArtist ////

MagnatuneArtist::MagnatuneArtist( )
    : m_id(0)
{
}

void MagnatuneArtist::setId( int id )
{
    m_id = id;
}

int MagnatuneArtist::getId( ) const
{
    return m_id;
}

void MagnatuneArtist::setName( QString name )
{
    m_name = name;
}

QString MagnatuneArtist::getName( ) const
{
    return m_name;
}

void MagnatuneArtist::setDescription( QString description )
{
    m_description = description;
}

QString MagnatuneArtist::getDescription( ) const
{ 
    return m_description;
}

void MagnatuneArtist::setPhotoURL( QString photoURL )
{
    m_photoURL = photoURL;
}

QString MagnatuneArtist::getPhotoURL( ) const
{
    return m_photoURL;
}

void MagnatuneArtist::setHomeURL( QString homeURL )
{
    m_homeURL = homeURL;
}

QString MagnatuneArtist::getHomeURL( ) const
{
    return m_homeURL;
}








//// MagnatuneAlbum ////

MagnatuneAlbum::MagnatuneAlbum( )
    : m_id (0)
    , m_artistId(-1)
{
}

void MagnatuneAlbum::setId( int id )
{
    m_id = id;
}

int MagnatuneAlbum::getId( ) const
{
    return m_id;
}

void MagnatuneAlbum::setArtistId( int artistId )
{
    m_artistId = artistId;
}

int MagnatuneAlbum::getArtistId( ) const
{
    return m_artistId;
}


void MagnatuneAlbum::setName( QString name )
{
    m_name = name;
}

QString MagnatuneAlbum::getName( ) const
{
    return m_name;
}

void MagnatuneAlbum::setCoverURL( QString coverURL )
{
    m_coverURL = coverURL;
}

QString MagnatuneAlbum::getCoverURL( ) const
{
    return m_coverURL;
}

void MagnatuneAlbum::setLaunchDate( QDate launchDate )
{
    m_launchDate = launchDate;
}

QDate MagnatuneAlbum::getLaunchDate( ) const
{
    return m_launchDate;
}

void MagnatuneAlbum::setAlbumCode( QString albumCode )
{
    m_albumCode = albumCode;
}

QString MagnatuneAlbum::getAlbumCode( ) const
{
    return m_albumCode;
}

void MagnatuneAlbum::setMp3Genre( QString mp3Genre )
{
    m_mp3Genre = mp3Genre;
}

QString MagnatuneAlbum::getMp3Genre( ) const
{
    return m_mp3Genre;
}

void MagnatuneAlbum::setMagnatuneGenres( QString magnatuneGenres )
{
    m_magnatuneGenres = magnatuneGenres;
}

QString MagnatuneAlbum::getMagnatuneGenres( ) const
{
    return m_magnatuneGenres;
}









//// MagnatuneTrack ////

MagnatuneTrack::MagnatuneTrack( )
    : m_id( 0 )
    , m_trackNumber( 0 )
    , m_duration( 0 )
    , m_albumId( 0 )
    , m_artistId( 0 )
{
}

void MagnatuneTrack::setId( int id )
{
   m_id = id;
}

int MagnatuneTrack::getId( ) const
{
    return m_id;
}

void MagnatuneTrack::setArtistId( int artistId )
{
    m_artistId = artistId;
}

int MagnatuneTrack::getArtistId( ) const
{
   return m_artistId;
}

void MagnatuneTrack::setAlbumId( int albumId )
{
    m_albumId = albumId;
}

int MagnatuneTrack::getAlbumId( ) const
{
   return m_albumId;
}



void MagnatuneTrack::setName( QString name )
{
    m_name = name;
}

QString MagnatuneTrack::getName( ) const
{
   return m_name;
}

void MagnatuneTrack::setTrackNumber( int trackNumber )
{
    m_trackNumber = trackNumber;
}

int MagnatuneTrack::getTrackNumber( ) const
{
    return m_trackNumber;
}

void MagnatuneTrack::setDuration( int duration )
{
    m_duration = duration;
}

int MagnatuneTrack::getDuration( ) const
{
    return m_duration;
}

void MagnatuneTrack::setHifiURL( QString hifiURL )
{
   m_hifiURL = hifiURL;
}

QString MagnatuneTrack::getHifiURL( ) const
{
    return m_hifiURL;
}

void MagnatuneTrack::setLofiURL( QString lofiURL )
{
    m_lofiURL = lofiURL;
}
 
QString MagnatuneTrack::getLofiURL( ) const
{
    return m_lofiURL;
}














