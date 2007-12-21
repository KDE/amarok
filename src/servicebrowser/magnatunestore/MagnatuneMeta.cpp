
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/



#include "MagnatuneMeta.h"

#include "amarok.h"
#include "debug.h"

#include "debug.h"

#include <KIcon>
#include <KLocale>

using namespace Meta;

MagnatuneMetaFactory::MagnatuneMetaFactory(const QString & dbPrefix)
    : ServiceMetaFactory( dbPrefix )
    , m_membershipPrefix( QString() )
    , m_userName( QString() )
    , m_password( QString() )
{
}

void MagnatuneMetaFactory::setMembershipInfo(QString prefix, QString userName, QString password)
{
    m_membershipPrefix = prefix;
    m_userName = userName;
    m_password = password;
}


int MagnatuneMetaFactory::getTrackSqlRowCount()
{
   return ServiceMetaFactory::getTrackSqlRowCount() + 1;
}

QString MagnatuneMetaFactory::getTrackSqlRows()
{
    DEBUG_BLOCK
    QString sqlRows = ServiceMetaFactory::getTrackSqlRows();

    sqlRows += ", ";
    sqlRows += tablePrefix() + "_tracks.preview_lofi ";

    return sqlRows;
}


TrackPtr MagnatuneMetaFactory::createTrack(const QStringList & rows)
{

    MagnatuneTrack * track = new MagnatuneTrack( rows );

    if ( !m_membershipPrefix.isEmpty() ) {
        QString url = track->url();
        url.replace( "http://he3.", "http://" + m_userName + ":" + m_password + "@" + m_membershipPrefix + "." );
        url.replace( ".mp3", "_nospeech.mp3" );
        track->setUrl( url );
    }

    return TrackPtr( track );
}

int MagnatuneMetaFactory::getAlbumSqlRowCount()
{
    return ServiceMetaFactory::getAlbumSqlRowCount() + 3;
}

QString MagnatuneMetaFactory::getAlbumSqlRows()
{
    DEBUG_BLOCK
    QString sqlRows = ServiceMetaFactory::getAlbumSqlRows();

    sqlRows += ", ";
    sqlRows += tablePrefix() + "_albums.cover_url, ";
    sqlRows += tablePrefix() + "_albums.year, ";
    sqlRows += tablePrefix() + "_albums.album_code ";


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
    DEBUG_BLOCK
    QString sqlRows = ServiceMetaFactory::getArtistSqlRows();

    sqlRows += ", ";
    sqlRows += tablePrefix() + "_artists.photo_url, ";
    sqlRows += tablePrefix() + "_artists.artist_page ";

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
    DEBUG_BLOCK
    m_lofiUrl = resultRow[7];
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
    : ServiceAlbumWithCover( name )
    , m_coverUrl()
    , m_launchYear( 0 )
    , m_albumCode()

{
}

MagnatuneAlbum::MagnatuneAlbum(const QStringList & resultRow)
    : ServiceAlbumWithCover( resultRow )
{
    debug() << "create album from result row: " << resultRow;


    m_coverUrl = resultRow[4];
    m_launchYear = resultRow[5].toInt();
    m_albumCode = resultRow[6];

}

MagnatuneAlbum::~ MagnatuneAlbum()
{
}


void MagnatuneAlbum::setLaunchYear( int launchYear )
{
    m_launchYear = launchYear;
}

int MagnatuneAlbum::launchYear( ) const
{
    return m_launchYear;
}

void MagnatuneAlbum::setAlbumCode(const QString & albumCode)
{
    m_albumCode = albumCode;
}

QString MagnatuneAlbum::albumCode()
{
    return m_albumCode;

}

void MagnatuneAlbum::setCoverUrl(const QString & coverUrl)
{
    m_coverUrl = coverUrl;
}

QString MagnatuneAlbum::coverUrl() const
{
    return m_coverUrl;
}


QList< QAction * > MagnatuneAlbum::customActions()
{
    DEBUG_BLOCK
            QList< QAction * > actions;
    QAction * action = new QAction( KIcon(Amarok::icon( "download" ) ), i18n( "&Buy" ), 0 );

    //TODO connect some slot to the action, also, give the damn action a parent please
    actions.append( action );
    return actions;
}






MagnatuneGenre::MagnatuneGenre( const QString & name )
    : ServiceGenre( name )
{
}

MagnatuneGenre::MagnatuneGenre( const QStringList & resultRow )
    : ServiceGenre( resultRow )
{
}







