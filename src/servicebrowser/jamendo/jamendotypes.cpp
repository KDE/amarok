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

JamendoArtist::JamendoArtist( const QString &name )
    : ServiceArtist( name )
{
    setDescription( QString() );
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

QString JamendoArtist::homeURL( ) const
{
    return m_homeURL;
}

void JamendoArtist::setJamendoURL( const QString & jamendoURL )
{
    m_jamendoURL = jamendoURL;
}

QString JamendoArtist::jamendoURL() const
{
    return m_jamendoURL;
}






//// JamendoAlbum ////

JamendoAlbum::JamendoAlbum( const QString &name )
    : ServiceAlbum( name )
{
    setDescription( QString() );
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


//// JamendoTrack ////

JamendoTrack::JamendoTrack( const QString &name )
    : ServiceTrack( name )
{
}


void JamendoTrack::setArtistId( int artistId )
{
    m_artistId = artistId;
}

int JamendoTrack::getArtistId( ) const
{
   return m_artistId;
}



















