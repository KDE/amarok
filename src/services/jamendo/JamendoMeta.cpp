/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "JamendoMeta.h"

#include "JamendoService.h"

#include "core/support/Debug.h"
#include <QStandardPaths>

#include <QAction>

using namespace Meta;

JamendoMetaFactory::JamendoMetaFactory( const QString & dbPrefix, JamendoService * service )
    : ServiceMetaFactory( dbPrefix )
    , m_service( service )
{
}

TrackPtr JamendoMetaFactory::createTrack( const QStringList & rows )
{
    JamendoTrack * track = new JamendoTrack( rows );
    track->setService( m_service );
    return TrackPtr( track );
}

int
JamendoMetaFactory::getAlbumSqlRowCount()
{
    return ServiceMetaFactory::getAlbumSqlRowCount() + 6;
}

QString
JamendoMetaFactory::getAlbumSqlRows()
{
    QString sqlRows = ServiceMetaFactory::getAlbumSqlRows();

    sqlRows += ", ";
    sqlRows += tablePrefix() + "_albums.popularity, ";
    sqlRows += tablePrefix() + "_albums.cover_url, ";
    sqlRows += tablePrefix() + "_albums.launch_year, ";
    sqlRows += tablePrefix() + "_albums.genre, ";
    sqlRows += tablePrefix() + "_albums.mp3_torrent_url, "; // Deprecated
    sqlRows += tablePrefix() + "_albums.ogg_torrent_url "; // Deprecated

    return sqlRows;
}

AlbumPtr
JamendoMetaFactory::createAlbum( const QStringList & rows )
{
    JamendoAlbum * album = new JamendoAlbum( rows );
    album->setService( m_service );
    album->setSourceName( "Jamendo.com" );
    return AlbumPtr( album );
}

int
JamendoMetaFactory::getArtistSqlRowCount()
{
    return ServiceMetaFactory::getArtistSqlRowCount() + 4;
}

QString
JamendoMetaFactory::getArtistSqlRows()
{
    QString sqlRows = ServiceMetaFactory::getArtistSqlRows();

    sqlRows += ", ";
    sqlRows += tablePrefix() + "_artists.country, ";
    sqlRows += tablePrefix() + "_artists.photo_url, ";
    sqlRows += tablePrefix() + "_artists.jamendo_url, ";
    sqlRows += tablePrefix() + "_artists.home_url ";

    return sqlRows;
}

ArtistPtr
JamendoMetaFactory::createArtist( const QStringList & rows )
{
    JamendoArtist * artist = new JamendoArtist( rows );
    artist->setSourceName( "Jamendo.com" );
    return ArtistPtr( artist );
}

GenrePtr
JamendoMetaFactory::createGenre( const QStringList & rows )
{
    JamendoGenre * genre = new JamendoGenre( rows );
    genre->setSourceName( "Jamendo.com" );
    return GenrePtr( genre );
}

//// JamendoTrack ////

JamendoTrack::JamendoTrack( const QString &name )
    : ServiceTrack( name )
    , m_service ( 0 )
{
}

JamendoTrack::JamendoTrack( const QStringList & resultRow )
    : ServiceTrack( resultRow )
    , m_service ( 0 )
{
}

QString
Meta::JamendoTrack::sourceName()
{
    return "Jamendo.com";
}

QString
Meta::JamendoTrack::sourceDescription()
{
    return i18n( "A site where artists can freely share their music" );
}

QPixmap
Meta::JamendoTrack::emblem()
{
    return QPixmap( QStandardPaths::locate( QStandardPaths::GenericDataLocation, "amarok/images/emblem-jamendo.png" ) );
}


QString JamendoTrack::scalableEmblem()
{
    return QStandardPaths::locate( QStandardPaths::GenericDataLocation, "amarok/images/emblem-jamendo-scalable.svgz" );
}

void
Meta::JamendoTrack::setService(JamendoService * service)
{
    m_service = service;
}

QString JamendoTrack::type() const
{
    return "mp3";
}

//// JamendoArtist ////

JamendoArtist::JamendoArtist( const QString &name )
    : ServiceArtist( name )
{
}

JamendoArtist::JamendoArtist( const QStringList & resultRow )
    : ServiceArtist( resultRow )
{
    m_country = resultRow[3];
    m_photoURL = resultRow[4];
    m_jamendoURL = resultRow[5];
    m_homeURL = resultRow[6];
}

void
JamendoArtist::setCountry( const QString & country )
{
    m_country = country;
}

QString
JamendoArtist::country() const
{
    return m_country;
}


void
JamendoArtist::setPhotoURL( const QString &photoURL )
{
    m_photoURL = photoURL;
}

QString
JamendoArtist::photoURL( ) const
{
    return m_photoURL;
}

void
JamendoArtist::setHomeURL( const QString &homeURL )
{
    m_homeURL = homeURL;
}

QString
JamendoArtist::homeURL( ) const
{
    return m_homeURL;
}

void
JamendoArtist::setJamendoURL( const QString & jamendoURL )
{
    m_jamendoURL = jamendoURL;
}

QString
JamendoArtist::jamendoURL() const
{
    return m_jamendoURL;
}


//// JamendoAlbum ////

JamendoAlbum::JamendoAlbum( const QString &name )
    : ServiceAlbumWithCover( name )
{
}

JamendoAlbum::JamendoAlbum( const QStringList & resultRow )
    : ServiceAlbumWithCover( resultRow )
{
    m_popularity = resultRow[4].toFloat();
    m_coverURL = resultRow[5];
    m_launchYear = resultRow[6].toInt();
    m_genre = resultRow[7];
}

void
JamendoAlbum::setCoverUrl( const QString &coverURL )
{
    m_coverURL = coverURL;
}

QString
JamendoAlbum::coverUrl( ) const
{
    return m_coverURL;
}

void
JamendoAlbum::setLaunchYear( int launchYear )
{
    m_launchYear = launchYear;
}

int
JamendoAlbum::launchYear( ) const
{
    return m_launchYear;
}

void
JamendoAlbum::setGenre( const QString&genre )
{
    m_genre = genre;
}

QString
JamendoAlbum::genre( ) const
{
    return m_genre;
}

void
JamendoAlbum::setPopularity( float popularity )
{
    m_popularity = popularity;
}

float
JamendoAlbum::popularity() const
{
    return m_popularity;
}

void
Meta::JamendoAlbum::setService( JamendoService * service )
{
    m_service = service;
}

JamendoService *
Meta::JamendoAlbum::service()
{
    return m_service;
}


JamendoGenre::JamendoGenre( const QString & name )
    : ServiceGenre( name )
{
}

JamendoGenre::JamendoGenre( const QStringList & resultRow )
    : ServiceGenre( resultRow )
{
}

