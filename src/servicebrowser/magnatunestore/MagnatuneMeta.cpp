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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/



#include "MagnatuneMeta.h"


MagnatuneMetaFactory::MagnatuneMetaFactory(const QString & dbPrefix)
    : ServiceMetaFactory( dbPrefix )
{
}

int MagnatuneMetaFactory::getTrackSqlRowCount()
{
   return ServiceMetaFactory::getTrackSqlRowCount() + 1; 
}

QString MagnatuneMetaFactory::getTrackSqlRows()
{
    QString sqlRows = ServiceMetaFactory::getTrackSqlRows();
    
    sqlRows += ", ";
    sqlRows += tablePrefix() + "_tracks.preview_url_lofi ";
}


TrackPtr MagnatuneMetaFactory::createTrack(const QStringList & rows)
{
    return TrackPtr( new MagnatuneTrack( rows ) );
}

int MagnatuneMetaFactory::getAlbumSqlRowCount()
{
    return ServiceMetaFactory::getAlbumSqlRowCount() + 2;
}

QString MagnatuneMetaFactory::getAlbumSqlRows()
{
    QString sqlRows = ServiceMetaFactory::getAlbumSqlRows();

    sqlRows += ", ";
    sqlRows += tablePrefix() + "_albums.cover_url, ";
    sqlRows += tablePrefix() + "_albums.launch_year ";


    return sqlRows;
}

AlbumPtr MagnatuneMetaFactory::createAlbum(const QStringList & rows)
{
    return AlbumPtr( new MagnatuneAlbum( rows ) );
}

int MagnatuneMetaFactory::getArtistSqlRowCount()
{
    return ServiceMetaFactory::getArtistSqlRowCount() + 2;
}

QString MagnatuneMetaFactory::getArtistSqlRows()
{
    QString sqlRows = ServiceMetaFactory::getArtistSqlRows();

    sqlRows += ", ";
    sqlRows += tablePrefix() + "_artists.photo_url, ";
    sqlRows += tablePrefix() + "_artists.magnatune_url ";

    return sqlRows;
}

ArtistPtr MagnatuneMetaFactory::createArtist(const QStringList & rows)
{
    return ArtistPtr( new MagnatuneArtist( rows ) );
}


GenrePtr MagnatuneMetaFactory::createGenre(const QStringList & rows)
{
    return GenrePtr( new MagnatuneGenre( rows ) );
}


//// MagnatuneTrack ////

MagnatuneTrack::MagnatuneTrack( const QString &name )
    : ServiceTrack( name )
{
}

MagnatuneTrack::MagnatuneTrack(const QStringList & resultRow)
    : ServiceTrack( resultRow )
{
    m_lofiUrl = resultRow[9];
}

QString MagnatuneTrack::lofiUrl()
{
    return m_lofiUrl;
}

void MagnatuneTrack::setLofiUrl(const QString & url)
{
    m_lofiUrl = url;
}


//// MagnatuneArtist ////

MagnatuneArtist::MagnatuneArtist( const QString &name )
    : ServiceArtist( name )
{
}

MagnatuneArtist::MagnatuneArtist(const QStringList & resultRow)
    : ServiceArtist( resultRow )
{
    m_photoUrl = resultRow[3];
    m_magnatuneUrl = resultRow[4];


}

void MagnatuneArtist::setPhotoUrl( const QString &photoUrl )
{
    m_photoUrl = photoUrl;
}

QString MagnatuneArtist::photoUrl( ) const
{
    return m_photoUrl;
}

void MagnatuneArtist::setMagnatuneUrl( const QString & magnatuneUrl )
{
    m_magnatuneUrl = magnatuneUrl;
}

QString MagnatuneArtist::magnatuneUrl() const
{
    return m_magnatuneUrl;
}






//// MagnatuneAlbum ////

MagnatuneAlbum::MagnatuneAlbum( const QString &name )
    : ServiceAlbum( name )
{
}

MagnatuneAlbum::MagnatuneAlbum(const QStringList & resultRow)
    : ServiceAlbum( resultRow )
{

    m_coverUrl = resultRow[5];
    m_launchYear = resultRow[6].toInt();

}

void MagnatuneAlbum::setCoverUrl( const QString &coverUrl )
{
    m_coverUrl = coverUrl;
}

QString MagnatuneAlbum::coverUrl( ) const
{
    return m_coverUrl;
}

void MagnatuneAlbum::setLaunchYear( int launchYear )
{
    m_launchYear = launchYear;
}

int MagnatuneAlbum::launchYear( ) const
{
    return m_launchYear;
}



MagnatuneGenre::MagnatuneGenre(const QString & name)
    : ServiceGenre( name )
{
}

MagnatuneGenre::MagnatuneGenre(const QStringList & resultRow)
    : ServiceGenre( resultRow )
{
}














