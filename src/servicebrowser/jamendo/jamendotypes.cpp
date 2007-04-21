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



#include "jamendotypes.h"


//// JamendoArtist ////

JamendoArtist::JamendoArtist( )
    : m_id(0)
{
    m_description = QString();
}

void JamendoArtist::setId( int id )
{
    m_id = id;
}

int JamendoArtist::getId( ) const
{
    return m_id;
}

void JamendoArtist::setName( const QString &name )
{
    m_name = name;
}

QString JamendoArtist::getName( ) const
{
    return m_name;
}

void JamendoArtist::setDescription( const QString &description )
{
    m_description = description;
}

QString JamendoArtist::getDescription( ) const
{ 
    return m_description;
}

void JamendoArtist::setPhotoURL( const QString &photoURL )
{
    m_photoURL = photoURL;
}

QString JamendoArtist::getPhotoURL( ) const
{
    return m_photoURL;
}

void JamendoArtist::setHomeURL( const QString &homeURL )
{
    m_homeURL = homeURL;
}

QString JamendoArtist::getHomeURL( ) const
{
    return m_homeURL;
}

void JamendoArtist::setJamendoURL( const QString & jamendoURL )
{
    m_jamendoURL = jamendoURL;
}

QString JamendoArtist::getJamendoURL() const
{
    return m_jamendoURL;
}






//// JamendoAlbum ////

JamendoAlbum::JamendoAlbum( )
    : m_id (0)
{
}

void JamendoAlbum::setId( int id )
{
    m_id = id;
}

int JamendoAlbum::getId( ) const
{
    return m_id;
}

void JamendoAlbum::setArtistId( int artistId )
{
    m_artistId = artistId;
}

int JamendoAlbum::getArtistId( ) const
{
    return m_artistId;
}


void JamendoAlbum::setName( const QString &name )
{
    m_name = name;
}

QString JamendoAlbum::getName( ) const
{
    return m_name;
}

void JamendoAlbum::setCoverURL( const QString &coverURL )
{
    m_coverURL = coverURL;
}

QString JamendoAlbum::getCoverURL( ) const
{
    return m_coverURL;
}

void JamendoAlbum::setLaunchDate( const QDate &launchDate )
{
    m_launchDate = launchDate;
}

QDate JamendoAlbum::getLaunchDate( ) const
{
    return m_launchDate;
}

void JamendoAlbum::setJamendoTags(const QStringList & tags)
{
    m_tags = tags;
}

QStringList JamendoAlbum::getJamendoTags() const
{
    return m_tags;
}

void JamendoAlbum::setGenre( const QString&genre )
{
    m_genre = genre;
}

QString JamendoAlbum::getGenre( ) const
{
    return m_genre;
}

void JamendoAlbum::setDescription( const QString description )
{
    m_description = description;
}

QString JamendoAlbum::getDescription()
{
    return m_description;
}









//// JamendoTrack ////

JamendoTrack::JamendoTrack( )
    : m_id( 0 )
    , m_trackNumber( 0 )
    , m_duration( 0 )
    , m_albumId( 0 )
{
}

void JamendoTrack::setId( int id )
{
   m_id = id;
}

int JamendoTrack::getId( ) const
{
    return m_id;
}

void JamendoTrack::setArtistId( int artistId )
{
    m_artistId = artistId;
}

int JamendoTrack::getArtistId( ) const
{
   return m_artistId;
}

void JamendoTrack::setAlbumId( int albumId )
{
    m_albumId = albumId;
}

int JamendoTrack::getAlbumId( ) const
{
   return m_albumId;
}



void JamendoTrack::setName( const QString &name )
{
    m_name = name;
}

QString JamendoTrack::getName( ) const
{
   return m_name;
}

void JamendoTrack::setTrackNumber( int trackNumber )
{
    m_trackNumber = trackNumber;
}

int JamendoTrack::getTrackNumber( ) const
{
    return m_trackNumber;
}

void JamendoTrack::setDuration( int duration )
{
    m_duration = duration;
}

int JamendoTrack::getDuration( ) const
{
    return m_duration;
}

void JamendoTrack::setLofiURL( const QString &lofiURL )
{
    m_lofiURL = lofiURL;
}
 
QString JamendoTrack::getLofiURL( ) const
{
    return m_lofiURL;
}



















