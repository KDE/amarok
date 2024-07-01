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

#include "MagnatuneMeta.h"
#include "MagnatuneStore.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core-impl/support/UrlStatisticsStore.h"
#include "MagnatuneActions.h"
#include "MagnatuneConfig.h"

#include <QObject>
#include <QStandardPaths>

#include <KLocalizedString>

using namespace Meta;

MagnatuneMetaFactory::MagnatuneMetaFactory( const QString & dbPrefix, MagnatuneStore * store )
    : ServiceMetaFactory( dbPrefix )
    , m_membershipPrefix( QString() )
    , m_streamType( OGG )
    , m_userName( QString() )
    , m_password( QString() )
    , m_store( store )
{}

void MagnatuneMetaFactory::setMembershipInfo( const QString &prefix, const QString &userName, const QString &password )
{
    m_membershipPrefix = prefix;
    m_userName = userName;
    m_password = password;
}

void MagnatuneMetaFactory::setStreamType(int type)
{
    m_streamType = type;
}


int MagnatuneMetaFactory::getTrackSqlRowCount()
{
   return ServiceMetaFactory::getTrackSqlRowCount() + 2;
}

QString MagnatuneMetaFactory::getTrackSqlRows()
{
    QString sqlRows = ServiceMetaFactory::getTrackSqlRows();

    sqlRows += QStringLiteral(", ");
    sqlRows += tablePrefix() + QStringLiteral("_tracks.preview_lofi, ");
    sqlRows += tablePrefix() + QStringLiteral("_tracks.preview_ogg ");

    return sqlRows;
}

TrackPtr
MagnatuneMetaFactory::createTrack(const QStringList & rows)
{
    MagnatuneTrack *track = new MagnatuneTrack( rows );

    if ( m_streamType == OGG ) {
        track->setUidUrl( track->oggUrl() );
    } else if (  m_streamType == LOFI ) {
        track->setUidUrl( track->lofiUrl() );
    }
    track->setStatisticsProvider( Meta::StatisticsPtr( new UrlStatisticsStore( track ) ) );

    if ( !m_membershipPrefix.isEmpty() ) {
        QString url = track->uidUrl();
        url.replace( QStringLiteral("http://he3."), QStringLiteral("http://") + m_userName + QStringLiteral(":") + m_password + QStringLiteral("@") + m_membershipPrefix + QStringLiteral(".") );

        if ( m_streamType == MP3 ) {
            url.replace( QStringLiteral(".mp3"), QStringLiteral("_nospeech.mp3") );
        }  else if ( m_streamType == OGG ) {
            url.replace( QStringLiteral(".ogg"), QStringLiteral("_nospeech.ogg") );
        }

        track->setUidUrl( url );

        if ( m_membershipPrefix == QStringLiteral("download") )
            track->setDownloadMembership();
    }

    return Meta::TrackPtr( track );
}

int MagnatuneMetaFactory::getAlbumSqlRowCount()
{
    return ServiceMetaFactory::getAlbumSqlRowCount() + 3;
}

QString MagnatuneMetaFactory::getAlbumSqlRows()
{
    QString sqlRows = ServiceMetaFactory::getAlbumSqlRows();

    sqlRows += QStringLiteral(", ");
    sqlRows += tablePrefix() + QStringLiteral("_albums.cover_url, ");
    sqlRows += tablePrefix() + QStringLiteral("_albums.year, ");
    sqlRows += tablePrefix() + QStringLiteral("_albums.album_code ");


    return sqlRows;
}

AlbumPtr MagnatuneMetaFactory::createAlbum(const QStringList & rows)
{
    MagnatuneAlbum * album = new MagnatuneAlbum( rows );
    album->setStore( m_store );

    if ( m_membershipPrefix == QStringLiteral("download") )
        album->setDownloadMembership();

    album->setSourceName( QStringLiteral("Magnatune.com") );
    return AlbumPtr( album );
}

int MagnatuneMetaFactory::getArtistSqlRowCount()
{
    return ServiceMetaFactory::getArtistSqlRowCount() + 2;
}

QString MagnatuneMetaFactory::getArtistSqlRows()
{
    QString sqlRows = ServiceMetaFactory::getArtistSqlRows();

    sqlRows += QStringLiteral(", ");
    sqlRows += tablePrefix() + QStringLiteral("_artists.photo_url, ");
    sqlRows += tablePrefix() + QStringLiteral("_artists.artist_page ");

    return sqlRows;
}

ArtistPtr MagnatuneMetaFactory::createArtist(const QStringList & rows)
{
    MagnatuneArtist * artist = new MagnatuneArtist( rows );
    artist->setSourceName( QStringLiteral("Magnatune.com") );
    return ArtistPtr( artist );

    
}

GenrePtr MagnatuneMetaFactory::createGenre(const QStringList & rows)
{
    MagnatuneGenre * genre = new MagnatuneGenre( rows );
    genre->setSourceName( QStringLiteral("Magnatune.com") );
    return GenrePtr( genre );
}


///////////////////////////////////////////////////////////////////////////////
// class MagnatuneTrack
///////////////////////////////////////////////////////////////////////////////

MagnatuneTrack::MagnatuneTrack( const QString &name )
    : ServiceTrack( name )
    , m_downloadMembership ( false )
{}

MagnatuneTrack::MagnatuneTrack(const QStringList & resultRow)
    : ServiceTrack( resultRow )
    , m_downloadMembership ( false )
{
    m_lofiUrl = resultRow[7];
    m_oggUrl = resultRow[8];
}

QString MagnatuneTrack::lofiUrl()
{
    return m_lofiUrl;
}

void MagnatuneTrack::setLofiUrl(const QString & url)
{
    m_lofiUrl = url;
}

QString Meta::MagnatuneTrack::oggUrl() const
{
    return m_oggUrl;
}

void Meta::MagnatuneTrack::setOggUrl( const QString& url )
{
    m_oggUrl = url;
}

void Meta::MagnatuneTrack::setDownloadMembership()
{
    m_downloadMembership = true;
}

QString Meta::MagnatuneTrack::sourceName()
{
    return QStringLiteral("Magnatune.com");
}

QString Meta::MagnatuneTrack::sourceDescription()
{
    return i18n( "The non evil record label that is fair to artists and customers alike" );
}

QPixmap Meta::MagnatuneTrack::emblem()
{
    return QPixmap( QStandardPaths::locate( QStandardPaths::GenericDataLocation, QStringLiteral("amarok/images/emblem-magnatune.png") ) );
}


QList< QString > Meta::MagnatuneTrack::moods() const
{
    return m_moods;
}

void Meta::MagnatuneTrack::setMoods(const QList<QString> &moods)
{
    m_moods = moods;
}

void Meta::MagnatuneTrack::download()
{
    DEBUG_BLOCK
    MagnatuneAlbum * mAlbum = dynamic_cast<MagnatuneAlbum *> ( album().data() );
    if ( mAlbum )
        mAlbum->store()->downloadTrack( this );
}

void Meta::MagnatuneTrack::setAlbumPtr(const AlbumPtr &album )
{
    ServiceTrack::setAlbumPtr( album );

    //get year from magnatune album:
    MagnatuneAlbum * ma = dynamic_cast<MagnatuneAlbum *>( album.data() );
    if ( ma )
    {
        YearPtr year = YearPtr( new ServiceYear( QString::number( ma->launchYear() ) ) );
        setYear( year );
    }
}


///////////////////////////////////////////////////////////////////////////////
// class MagnatuneArtist
///////////////////////////////////////////////////////////////////////////////

MagnatuneArtist::MagnatuneArtist( const QString &name )
    : ServiceArtist( name )
{}

MagnatuneArtist::MagnatuneArtist(const QStringList & resultRow)
    : ServiceArtist( resultRow )
{
    auto list = QUrl::fromStringList( resultRow );

    if( list.size() < 5 )
        return;

    m_photoUrl = list.at(3);
    m_magnatuneUrl = list.at(4);
}

void MagnatuneArtist::setPhotoUrl( const QUrl &photoUrl )
{
    m_photoUrl = photoUrl;
}

QUrl MagnatuneArtist::photoUrl( ) const
{
    return m_photoUrl;
}

void MagnatuneArtist::setMagnatuneUrl( const QUrl & magnatuneUrl )
{
    m_magnatuneUrl = magnatuneUrl;
}

QUrl MagnatuneArtist::magnatuneUrl() const
{
    return m_magnatuneUrl;
}


///////////////////////////////////////////////////////////////////////////////
// class MagnatuneAlbum
///////////////////////////////////////////////////////////////////////////////

MagnatuneAlbum::MagnatuneAlbum( const QString &name )
    : ServiceAlbumWithCover( name )
    , m_coverUrl()
    , m_launchYear( 0 )
    , m_albumCode()
    , m_store( nullptr )
    , m_downloadMembership( false )

{}

MagnatuneAlbum::MagnatuneAlbum(const QStringList & resultRow)
    : ServiceAlbumWithCover( resultRow )
    , m_downloadMembership ( false )
{
    m_coverUrl = resultRow[4];
    m_launchYear = resultRow[5].toInt();
    m_albumCode = resultRow[6];

    m_store = nullptr;
}

MagnatuneAlbum::~ MagnatuneAlbum()
{}


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

void Meta::MagnatuneAlbum::setStore(MagnatuneStore * store)
{
    m_store = store;
}

MagnatuneStore * Meta::MagnatuneAlbum::store()
{
    return m_store;
}

void Meta::MagnatuneAlbum::setDownloadMembership()
{
    DEBUG_BLOCK
    m_downloadMembership = true;
}

void Meta::MagnatuneAlbum::download()
{
    DEBUG_BLOCK
    if ( store() )
        store()->downloadAlbum( this );
}

void Meta::MagnatuneAlbum::addToFavorites()
{
    DEBUG_BLOCK
    if ( store() )
        store()->addToFavorites( albumCode() );
}

///////////////////////////////////////////////////////////////////////////////
// class MagnatuneGenre
///////////////////////////////////////////////////////////////////////////////

MagnatuneGenre::MagnatuneGenre( const QString & name )
    : ServiceGenre( name )
{}

MagnatuneGenre::MagnatuneGenre( const QStringList & resultRow )
    : ServiceGenre( resultRow )
{}

