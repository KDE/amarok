/***************************************************************************
 *   Copyright (c) 2006, 2007                                              *
 *        Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                   *
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


//// MagnatuneArtist ////

MagnatuneArtist::MagnatuneArtist( )
    : SimpleServiceArtist()
{
    m_description = QString();
}


void MagnatuneArtist::setPhotoURL( const QString &photoURL )
{
    m_photoURL = photoURL;
}

QString MagnatuneArtist::getPhotoURL( ) const
{
    return m_photoURL;
}

void MagnatuneArtist::setHomeURL( const QString &homeURL )
{
    m_homeURL = homeURL;
}

QString MagnatuneArtist::getHomeURL( ) const
{
    return m_homeURL;
}



//// MagnatuneAlbum ////

MagnatuneAlbum::MagnatuneAlbum( )
    : SimpleServiceAlbum()
{
}

void MagnatuneAlbum::setCoverURL( const QString &coverURL )
{
    m_coverURL = coverURL;
}

QString MagnatuneAlbum::getCoverURL( ) const
{
    return m_coverURL;
}

void MagnatuneAlbum::setLaunchDate( const QDate &launchDate )
{
    m_launchDate = launchDate;
}

QDate MagnatuneAlbum::getLaunchDate( ) const
{
    return m_launchDate;
}

void MagnatuneAlbum::setAlbumCode( const QString &albumCode )
{
    m_albumCode = albumCode;
}

QString MagnatuneAlbum::getAlbumCode( ) const
{
    return m_albumCode;
}

void MagnatuneAlbum::setMp3Genre( const QString &mp3Genre )
{
    m_mp3Genre = mp3Genre;
}

QString MagnatuneAlbum::getMp3Genre( ) const
{
    return m_mp3Genre;
}

void MagnatuneAlbum::setMagnatuneGenres( const QString &magnatuneGenres )
{
    m_magnatuneGenres = magnatuneGenres;
}

QString MagnatuneAlbum::getMagnatuneGenres( ) const
{
    return m_magnatuneGenres;
}



//// MagnatuneTrack ////

MagnatuneTrack::MagnatuneTrack( )
    : SimpleServiceTrack ()
    , m_artistId( 0 )
{
}

void MagnatuneTrack::setArtistId( int artistId )
{
    m_artistId = artistId;
}

int MagnatuneTrack::getArtistId( ) const
{
   return m_artistId;
}

void MagnatuneTrack::setLofiURL( const QString &lofiURL )
{
    m_lofiURL = lofiURL;
}
 
QString MagnatuneTrack::getLofiURL( ) const
{
    return m_lofiURL;
}

void MagnatuneTrack::setMoods(const QStringList & moods)
{
    m_moods = moods;
}

QStringList MagnatuneTrack::getMoods() const
{
    return m_moods;
}














