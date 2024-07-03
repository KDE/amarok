/****************************************************************************************
 * Copyright (c) 2010 Andrew Coder <andrew.coder@gmail.com>                             *
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

#include "PlaydarMeta.h"

#include "amarokconfig.h"
#include "AmarokSharedPointer.h"
#include "core/meta/Meta.h"
#include "core-impl/meta/default/DefaultMetaTypes.h"
#include "core-impl/support/UrlStatisticsStore.h"
#include "covermanager/CoverFetcher.h"
#include "covermanager/CoverCache.h"
#include "PlaydarCollection.h"

#include <QDateTime>
#include <QList>
#include <QString>
#include <QUrl>
#include <QUrlQuery>

namespace Collections
{
    class Collection;
    class PlaydarCollection;
}

Meta::PlaydarTrack::PlaydarTrack( QString &sid,
                                  QString &playableUrl,
                                  QString &name,
                                  QString &artist,
                                  QString &album,
                                  QString &mimetype,
                                  double score,
                                  qint64 length,
                                  int bitrate,
                                  int filesize,
                                  QString &source )
    : m_album( new PlaydarAlbum( album ) )
    , m_artist( new PlaydarArtist( artist ) )
    , m_composer( new PlaydarComposer( QStringLiteral( "" ) ) )
    , m_genre( new PlaydarGenre( QStringLiteral( "" ) ) )
    , m_year( new PlaydarYear( QStringLiteral( "" ) ) )
    , m_labelList(  )
    , m_sid( sid )
    , m_uidUrl( )
    , m_playableUrl( playableUrl )
    , m_name( name )
    , m_mimetype( mimetype )
    , m_score( score )
    , m_length( length )
    , m_bitrate( bitrate )
    , m_filesize( filesize )
    , m_trackNumber( 0 )
    , m_discNumber( 0 )
    , m_createDate( QDateTime::currentDateTime() )
    , m_comment( QStringLiteral( "" ) )
    , m_source( source )
{
    QUrlQuery query;
    m_uidUrl.setScheme( QStringLiteral( "playdar" ) );
    m_uidUrl.setPath(m_uidUrl.path() + QLatin1Char('/') + source );
    query.addQueryItem( QStringLiteral( "artist" ), artist );
    query.addQueryItem( QStringLiteral( "album" ), album );
    query.addQueryItem( QStringLiteral( "title" ), name );
    m_uidUrl.setQuery( query );
    m_statsStore = new UrlStatisticsStore( this );
}

Meta::PlaydarTrack::~PlaydarTrack()
{
    //Do nothing...
}

QString
Meta::PlaydarTrack::name() const
{
    return m_name;
}

QUrl
Meta::PlaydarTrack::playableUrl() const
{
    return QUrl( m_playableUrl );
}

QString
Meta::PlaydarTrack::prettyUrl() const
{
    return uidUrl();
}

QString
Meta::PlaydarTrack::uidUrl() const
{
    return m_uidUrl.url();
}

QString
Meta::PlaydarTrack::sid() const
{
    return m_sid;
}

QString
Meta::PlaydarTrack::notPlayableReason() const
{
    if( !m_collection.data() )
        return i18n( "Source collection removed" );
    return QString();
}

Meta::AlbumPtr
Meta::PlaydarTrack::album() const
{
    return AlbumPtr::staticCast( m_album );
}

Meta::ArtistPtr
Meta::PlaydarTrack::artist() const
{
    return ArtistPtr::staticCast( m_artist );
}

Meta::ComposerPtr
Meta::PlaydarTrack::composer() const
{
    return ComposerPtr::staticCast( m_composer );
}

Meta::GenrePtr
Meta::PlaydarTrack::genre() const
{
    return GenrePtr::staticCast( m_genre );
}

Meta::YearPtr
Meta::PlaydarTrack::year() const
{
    return YearPtr::staticCast( m_year );
}

Meta::LabelList
Meta::PlaydarTrack::labels() const
{
    Meta::LabelList labelList;
    for( const PlaydarLabelPtr &label : m_labelList )
    {
        labelList.append( LabelPtr::staticCast( label ) );
    }
    
    return labelList;
}

qreal
Meta::PlaydarTrack::bpm() const
{
    /** TODO: Can we do better here? */
    return -1.0;
}

QString
Meta::PlaydarTrack::comment() const
{
    return m_comment;
}

double
Meta::PlaydarTrack::score() const
{
    return m_score;
}

qint64
Meta::PlaydarTrack::length() const
{
    return m_length;
}

int
Meta::PlaydarTrack::filesize() const
{
    return m_filesize;
}

int
Meta::PlaydarTrack::sampleRate() const
{
    return 0;
}

int
Meta::PlaydarTrack::bitrate() const
{
    return m_bitrate;
}

QDateTime
Meta::PlaydarTrack::createDate() const
{
    return m_createDate;
}

int
Meta::PlaydarTrack::trackNumber() const
{
    return m_trackNumber;
}

int
Meta::PlaydarTrack::discNumber() const
{
    return m_discNumber;
}

QString
Meta::PlaydarTrack::type() const
{
    return QStringLiteral( "stream" );
}

QString
Meta::PlaydarTrack::mimetype() const
{
    return m_mimetype;
}

bool
Meta::PlaydarTrack::inCollection() const
{
    return m_collection.data();
}

Collections::Collection*
Meta::PlaydarTrack::collection() const
{
    return m_collection.data();
}

QString
Meta::PlaydarTrack::cachedLyrics() const
{
    return QStringLiteral( "" );
}

void
Meta::PlaydarTrack::setCachedLyrics( const QString &lyrics )
{
    Q_UNUSED( lyrics );
}

void
Meta::PlaydarTrack::addLabel( const QString &label )
{
    PlaydarLabelPtr newLabel( new PlaydarLabel( label ) );
    
    m_labelList.append( newLabel );
}

void
Meta::PlaydarTrack::addLabel( const LabelPtr &label )
{
    PlaydarLabelPtr newLabel( new PlaydarLabel( label->name() ) );
    
    m_labelList.append( newLabel );
}

void
Meta::PlaydarTrack::removeLabel( const LabelPtr &label )
{
    for( const PlaydarLabelPtr &labelPtr : m_labelList )
    {
        if( labelPtr->name() == label->name() )
        {
            m_labelList.removeOne( labelPtr );
            return;
        }
    }
}

Meta::StatisticsPtr
Meta::PlaydarTrack::statistics()
{
    return m_statsStore;
}

QString
Meta::PlaydarTrack::source() const
{
    return m_source;
}

void
Meta::PlaydarTrack::addToCollection( Collections::PlaydarCollection *collection )
{
    m_collection = collection;
    if( m_collection.data() )
    {
        PlaydarTrackPtr sharedThis( this );
        m_collection->addNewTrack( sharedThis );
    }
}

void
Meta::PlaydarTrack::setAlbum( const PlaydarAlbumPtr &album )
{
    m_album = album;
}

void
Meta::PlaydarTrack::setArtist( const PlaydarArtistPtr &artist )
{
    m_artist = artist;
}

void
Meta::PlaydarTrack::setComposer( const PlaydarComposerPtr &composer )
{
    m_composer = composer;
}

void
Meta::PlaydarTrack::setGenre( const PlaydarGenrePtr &genre )
{
    m_genre = genre;
}

void
Meta::PlaydarTrack::setYear( const PlaydarYearPtr &year )
{
    m_year = year;
}

Meta::PlaydarAlbumPtr
Meta::PlaydarTrack::playdarAlbum()
{
    return m_album;
}

Meta::PlaydarArtistPtr
Meta::PlaydarTrack::playdarArtist()
{
    return m_artist;
}

Meta::PlaydarComposerPtr
Meta::PlaydarTrack::playdarComposer()
{
    return m_composer;
}

Meta::PlaydarGenrePtr
Meta::PlaydarTrack::playdarGenre()
{
    return m_genre;
}

Meta::PlaydarYearPtr
Meta::PlaydarTrack::playdarYear()
{
    return m_year;
}

Meta::PlaydarLabelList
Meta::PlaydarTrack::playdarLabels()
{
    return m_labelList;
}

Meta::PlaydarArtist::PlaydarArtist( const QString &name )
    : m_name( name )
    , m_tracks( )
    , m_albums( )
{
    //Do nothing...
}

Meta::PlaydarArtist::~PlaydarArtist()
{
    //Do nothing...
}

QString
Meta::PlaydarArtist::name() const
{
    return m_name;
}

Meta::TrackList
Meta::PlaydarArtist::tracks()
{
    return m_tracks;
}

Meta::AlbumList
Meta::PlaydarArtist::albums()
{
    return m_albums;
}

void
Meta::PlaydarArtist::addTrack( const PlaydarTrackPtr &newTrack )
{
    m_tracks.append( TrackPtr::staticCast( newTrack ) );
}

void
Meta::PlaydarArtist::addAlbum( const PlaydarAlbumPtr &newAlbum )
{
    m_albums.append( AlbumPtr::staticCast( newAlbum ) );
}

Meta::PlaydarAlbum::PlaydarAlbum( const QString &name )
    : m_name( name )
    , m_tracks( )
    , m_isCompilation( false )
    , m_albumArtist( nullptr )
    , m_suppressImageAutoFetch( false )
    , m_triedToFetchCover( false )
{
    //Do nothing...
}

Meta::PlaydarAlbum::~PlaydarAlbum()
{
    CoverCache::invalidateAlbum( this );
}

bool
Meta::PlaydarAlbum::isCompilation() const
{
    return m_isCompilation;
}

QString
Meta::PlaydarAlbum::name() const
{
    return m_name;
}

bool
Meta::PlaydarAlbum::hasAlbumArtist() const
{
    if( !m_albumArtist.isNull() )
        return true;
    else
        return false;
}

Meta::ArtistPtr
Meta::PlaydarAlbum::albumArtist() const
{
    return m_albumArtist;
}

Meta::TrackList
Meta::PlaydarAlbum::tracks()
{
    return m_tracks;
}

bool
Meta::PlaydarAlbum::hasImage( int size ) const
{
    Q_UNUSED( size );
    
    if( !m_cover.isNull() )
        return true;
    else
        return false;
}

QImage
Meta::PlaydarAlbum::image( int size ) const
{
    if ( m_cover.isNull() )
    {
        if( !m_suppressImageAutoFetch && !m_name.isEmpty() &&
            !m_triedToFetchCover && AmarokConfig::autoGetCoverArt() )
        {
            m_triedToFetchCover = true;
            CoverFetcher::instance()->queueAlbum( Meta::AlbumPtr(const_cast<PlaydarAlbum*>(this)) );
        }

        return Meta::Album::image( size );
    }

    return size <= 1 ? m_cover : m_cover.scaled( size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation );
}

QUrl
Meta::PlaydarAlbum::imageLocation( int size )
{
    if( !m_cover.isNull() )
        return QUrl();

    return Meta::Album::imageLocation( size );
}

bool
Meta::PlaydarAlbum::canUpdateImage() const
{
    return true;
}

void
Meta::PlaydarAlbum::setImage( const QImage &image )
{
    m_cover = image;
    CoverCache::invalidateAlbum( this );
}

void
Meta::PlaydarAlbum::removeImage()
{
    m_cover = QImage();
    CoverCache::invalidateAlbum( this );
}

void
Meta::PlaydarAlbum::setSuppressImageAutoFetch( const bool suppress )
{
    m_suppressImageAutoFetch = suppress;
}

bool
Meta::PlaydarAlbum::suppressImageAutoFetch() const
{
    return m_suppressImageAutoFetch;
}

void
Meta::PlaydarAlbum::addTrack( const PlaydarTrackPtr &newTrack )
{
    m_tracks.append( TrackPtr::staticCast( newTrack ) );
}

void
Meta::PlaydarAlbum::setAlbumArtist( const PlaydarArtistPtr &newArtist )
{
    m_albumArtist = ArtistPtr::staticCast( newArtist );
}

Meta::PlaydarComposer::PlaydarComposer( const QString &name )
    : m_name( name )
    , m_tracks( )
{
    //Do nothing...
}

Meta::PlaydarComposer::~PlaydarComposer()
{
    //Do nothing...
}

QString
Meta::PlaydarComposer::name() const
{
    return m_name;
}

Meta::TrackList
Meta::PlaydarComposer::tracks()
{
    return m_tracks;
}

void
Meta::PlaydarComposer::addTrack( const PlaydarTrackPtr &newTrack )
{
    m_tracks.append( Meta::TrackPtr::staticCast( newTrack ) );
}

Meta::PlaydarGenre::PlaydarGenre( const QString &name )
    : m_name( name )
    , m_tracks( )
{
    //Do nothing...
}

Meta::PlaydarGenre::~PlaydarGenre()
{
    //Do nothing...
}

QString
Meta::PlaydarGenre::name() const
{
    return m_name;
}

Meta::TrackList
Meta::PlaydarGenre::tracks()
{
    return m_tracks;
}

void
Meta::PlaydarGenre::addTrack( const PlaydarTrackPtr &newTrack )
{
    m_tracks.append( Meta::TrackPtr::staticCast( newTrack ) );
}

Meta::PlaydarYear::PlaydarYear( const QString &name )
    : m_name( name )
    , m_tracks( )
{
    //Do nothing...
}

Meta::PlaydarYear::~PlaydarYear()
{
    //Do nothing...
}

QString
Meta::PlaydarYear::name() const
{
    return m_name;
}

Meta::TrackList
Meta::PlaydarYear::tracks()
{
    return m_tracks;
}

void
Meta::PlaydarYear::addTrack( const PlaydarTrackPtr &newTrack )
{
    m_tracks.append( Meta::TrackPtr::staticCast( newTrack ) );
}

Meta::PlaydarLabel::PlaydarLabel( const QString &name )
    : m_name( name )
    , m_tracks( )
{
    //Do nothing...
}

Meta::PlaydarLabel::~PlaydarLabel()
{
    //Do nothing...
}

QString
Meta::PlaydarLabel::name() const
{
    return m_name;
}

void
Meta::PlaydarLabel::addTrack( const PlaydarTrackPtr &newTrack )
{
    m_tracks.append( Meta::TrackPtr::staticCast( newTrack ) );
}
