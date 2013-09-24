/****************************************************************************************
 * Copyright (c) 2010 Andrew Coder <andrew.coder@gmail.com>                             *
 * Copyright (c) 2012 Ryan Feng <odayfans@gmail.com>                                    *
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

#include "SpotifyMeta.h"

#include "SpotifyCollection.h"

#include "amarokconfig.h"
#include "core-impl/meta/default/DefaultMetaTypes.h"
#include "core/meta/Meta.h"
#include "covermanager/CoverCache.h"
#include "covermanager/CoverFetcher.h"

#include <QDateTime>
#include <QList>
#include <QPixmap>
#include <QString>

#include <KSharedPtr>
#include <KUrl>

namespace Collections
{
    class Collection;
    class SpotifyCollection;
}

Meta::SpotifyTrack::SpotifyTrack( const QString &playableUrl,
                                  const QString &name,
                                  const QString &artist,
                                  const QString &album,
                                  const int     year,
                                  const int     trackNumber,
                                  const int     discNumber,
                                  const QString &genre,
                                  const QString &mimetype,
                                  const double score,
                                  const qint64 length,
                                  const int bitrate,
                                  const int filesize,
                                  const QString &source )
    : m_album( new SpotifyAlbum( album ) )
    , m_artist( new SpotifyArtist( artist ) )
    , m_composer( new SpotifyComposer( QString( "" ) ) )
    , m_genre( new SpotifyGenre( genre ) )
    , m_year( new SpotifyYear( QString::number( year ) ) )
    , m_labelList(  )
    , m_uidUrl( )
    , m_playableUrl( playableUrl )
    , m_name( name )
    , m_mimetype( mimetype )
    , m_score( score )
    , m_length( length )
    , m_bitrate( bitrate )
    , m_filesize( filesize )
    , m_trackNumber( trackNumber )
    , m_discNumber( discNumber )
    , m_createDate( QDateTime::currentDateTime() )
    , m_comment( QString( "" ) )
    , m_rating( 0 )
    , m_playcount( 0 )
    , m_source( source )
{
    m_uidUrl.setProtocol( QString( "spotify" ) );
    m_uidUrl.addPath( source );
    m_uidUrl.addQueryItem( QString( "artist" ), artist );
    m_uidUrl.addQueryItem( QString( "album" ), album );
    m_uidUrl.addQueryItem( QString( "title" ), name );
    m_uidUrl.addQueryItem( QString( "genre" ), genre );
}

Meta::SpotifyTrack::~SpotifyTrack()
{
    //Do nothing...
}

QString
Meta::SpotifyTrack::name() const
{
    return m_name;
}

KUrl
Meta::SpotifyTrack::playableUrl() const
{
    return KUrl( m_playableUrl );
}

QString
Meta::SpotifyTrack::prettyUrl() const
{
    return uidUrl();
}

QString
Meta::SpotifyTrack::uidUrl() const
{
    return m_uidUrl.url();
}


bool
Meta::SpotifyTrack::isPlayable() const
{
    return m_collection.data();
}

Meta::AlbumPtr
Meta::SpotifyTrack::album() const
{
    return AlbumPtr::staticCast( m_album );
}

Meta::ArtistPtr
Meta::SpotifyTrack::artist() const
{
    return ArtistPtr::staticCast( m_artist );
}

Meta::ComposerPtr
Meta::SpotifyTrack::composer() const
{
    return ComposerPtr::staticCast( m_composer );
}

Meta::GenrePtr
Meta::SpotifyTrack::genre() const
{
    return GenrePtr::staticCast( m_genre );
}

Meta::YearPtr
Meta::SpotifyTrack::year() const
{
    return YearPtr::staticCast( m_year );
}

Meta::LabelList
Meta::SpotifyTrack::labels() const
{
    Meta::LabelList labelList;
    foreach( const SpotifyLabelPtr &label, m_labelList )
    {
        labelList.append( LabelPtr::staticCast( label ) );
    }

    return labelList;
}

qreal
Meta::SpotifyTrack::bpm() const
{
    /** TODO: Can we do better here? */
    return -1.0;
}

QString
Meta::SpotifyTrack::comment() const
{
    return m_comment;
}

double
Meta::SpotifyTrack::score() const
{
    return m_score;
}

void
Meta::SpotifyTrack::setScore( double newScore )
{
    m_score = newScore;
}

int
Meta::SpotifyTrack::rating() const
{
    return m_rating;
}

void
Meta::SpotifyTrack::setRating( int newRating )
{
    m_rating = newRating;
}

qint64
Meta::SpotifyTrack::length() const
{
    return m_length;
}

int
Meta::SpotifyTrack::filesize() const
{
    return m_filesize;
}

int
Meta::SpotifyTrack::sampleRate() const
{
    return 0;
}

int
Meta::SpotifyTrack::bitrate() const
{
    return m_bitrate;
}

QDateTime
Meta::SpotifyTrack::createDate() const
{
    return m_createDate;
}

int
Meta::SpotifyTrack::trackNumber() const
{
    return m_trackNumber;
}

int
Meta::SpotifyTrack::discNumber() const
{
    return m_discNumber;
}

int
Meta::SpotifyTrack::playCount() const
{
    return m_playcount;
}

QString
Meta::SpotifyTrack::type() const
{
    return QString( "stream" );
}

QString
Meta::SpotifyTrack::notPlayableReason() const
{
    return QString();
}

QString
Meta::SpotifyTrack::mimetype() const
{
    return m_mimetype;
}

void
Meta::SpotifyTrack::prepareToPlay()
{
    /** TODO: Anything? */
}

void
Meta::SpotifyTrack::finishedPlaying( double playedFraction )
{
    if( playedFraction >= 1.0 )
    {
        m_playcount++;
        notifyObservers();
    }
}

bool
Meta::SpotifyTrack::inCollection() const
{
    return m_collection.data();
}

Collections::Collection*
Meta::SpotifyTrack::collection() const
{
    return m_collection.data();
}

QString
Meta::SpotifyTrack::cachedLyrics() const
{
    return QString( "" );
}

void
Meta::SpotifyTrack::setCachedLyrics( const QString &lyrics )
{
    Q_UNUSED( lyrics );
}

void
Meta::SpotifyTrack::addLabel( const QString &label )
{
    SpotifyLabelPtr newLabel( new SpotifyLabel( label ) );

    m_labelList.append( newLabel );
}

void
Meta::SpotifyTrack::addLabel( const LabelPtr &label )
{
    SpotifyLabelPtr newLabel( new SpotifyLabel( label->name() ) );

    m_labelList.append( newLabel );
}

void
Meta::SpotifyTrack::removeLabel( const LabelPtr &label )
{
    foreach( const SpotifyLabelPtr &labelPtr, m_labelList )
    {
        if( labelPtr->name() == label->name() )
        {
            m_labelList.removeOne( labelPtr );
            return;
        }
    }
}

QString
Meta::SpotifyTrack::source() const
{
    return m_source;
}

void
Meta::SpotifyTrack::addToCollection( Collections::SpotifyCollection *collection )
{
    m_collection = collection;
    if( m_collection.data() )
    {
        SpotifyTrackPtr sharedThis( this );
        m_collection.data()->addNewTrack( sharedThis );
    }
}

void
Meta::SpotifyTrack::setAlbum( SpotifyAlbumPtr album )
{
    m_album = album;
}

void
Meta::SpotifyTrack::setArtist( SpotifyArtistPtr artist )
{
    m_artist = artist;
}

void
Meta::SpotifyTrack::setComposer( SpotifyComposerPtr composer )
{
    m_composer = composer;
}

void
Meta::SpotifyTrack::setGenre( SpotifyGenrePtr genre )
{
    m_genre = genre;
}

void
Meta::SpotifyTrack::setYear( SpotifyYearPtr year )
{
    m_year = year;
}

Meta::SpotifyAlbumPtr
Meta::SpotifyTrack::spotifyAlbum()
{
    return m_album;
}

Meta::SpotifyArtistPtr
Meta::SpotifyTrack::spotifyArtist()
{
    return m_artist;
}

Meta::SpotifyComposerPtr
Meta::SpotifyTrack::spotifyComposer()
{
    return m_composer;
}

Meta::SpotifyGenrePtr
Meta::SpotifyTrack::spotifyGenre()
{
    return m_genre;
}

Meta::SpotifyYearPtr
Meta::SpotifyTrack::spotifyYear()
{
    return m_year;
}

Meta::SpotifyLabelList
Meta::SpotifyTrack::spotifyLabels()
{
    return m_labelList;
}

Meta::SpotifyArtist::SpotifyArtist( const QString &name )
    : m_name( name )
    , m_tracks( )
    , m_albums( )
{
    //Do nothing...
}

Meta::SpotifyArtist::~SpotifyArtist()
{
    //Do nothing...
}

QString
Meta::SpotifyArtist::name() const
{
    return m_name;
}

Meta::TrackList
Meta::SpotifyArtist::tracks()
{
    return m_tracks;
}

Meta::AlbumList
Meta::SpotifyArtist::albums()
{
    return m_albums;
}

void
Meta::SpotifyArtist::addTrack( SpotifyTrackPtr newTrack )
{
    m_tracks.append( TrackPtr::staticCast( newTrack ) );
}

void
Meta::SpotifyArtist::addAlbum( SpotifyAlbumPtr newAlbum )
{
    m_albums.append( AlbumPtr::staticCast( newAlbum ) );
}

Meta::SpotifyAlbum::SpotifyAlbum( const QString &name )
    : m_name( name )
    , m_tracks( )
    , m_isCompilation( false )
    , m_albumArtist( 0 )
    , m_suppressImageAutoFetch( false )
    , m_triedToFetchCover( false )
{
    //Do nothing...
}

Meta::SpotifyAlbum::~SpotifyAlbum()
{
    CoverCache::invalidateAlbum( this );
}

bool
Meta::SpotifyAlbum::isCompilation() const
{
    return m_isCompilation;
}

QString
Meta::SpotifyAlbum::name() const
{
    return m_name;
}

bool
Meta::SpotifyAlbum::hasAlbumArtist() const
{
    if( !m_albumArtist.isNull() )
        return true;
    else
        return false;
}

Meta::ArtistPtr
Meta::SpotifyAlbum::albumArtist() const
{
    return m_albumArtist;
}

Meta::TrackList
Meta::SpotifyAlbum::tracks()
{
    return m_tracks;
}

bool
Meta::SpotifyAlbum::hasImage( int size ) const
{
    Q_UNUSED( size );

    if( !m_cover.isNull() )
        return true;
    else
        return false;
}

QImage
Meta::SpotifyAlbum::image( int size ) const
{
    if ( m_cover.isNull() )
    {
        if( !m_suppressImageAutoFetch && !m_name.isEmpty() &&
            !m_triedToFetchCover && AmarokConfig::autoGetCoverArt() )
        {
            m_triedToFetchCover = true;
            CoverFetcher::instance()->queueAlbum( Meta::AlbumPtr(const_cast<SpotifyAlbum*>(this)) );
        }

        return Meta::Album::image( size );
    }

    return size <= 1 ? m_cover : m_cover.scaled( size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation );
}

KUrl
Meta::SpotifyAlbum::imageLocation( int size )
{
    if( !m_cover.isNull() )
        return KUrl();

    return Meta::Album::imageLocation( size );
}

bool
Meta::SpotifyAlbum::canUpdateImage() const
{
    return true;
}

void
Meta::SpotifyAlbum::setImage( const QImage &image )
{
    m_cover = image;
    CoverCache::invalidateAlbum( this );
}

void
Meta::SpotifyAlbum::removeImage()
{
    m_cover = QImage();
    CoverCache::invalidateAlbum( this );
}

void
Meta::SpotifyAlbum::setSuppressImageAutoFetch( const bool suppress )
{
    m_suppressImageAutoFetch = suppress;
}

bool
Meta::SpotifyAlbum::suppressImageAutoFetch() const
{
    return m_suppressImageAutoFetch;
}

void
Meta::SpotifyAlbum::addTrack( SpotifyTrackPtr newTrack )
{
    m_tracks.append( TrackPtr::staticCast( newTrack ) );
}

void
Meta::SpotifyAlbum::setAlbumArtist( SpotifyArtistPtr newArtist )
{
    m_albumArtist = ArtistPtr::staticCast( newArtist );
}

Meta::SpotifyComposer::SpotifyComposer( const QString &name )
    : m_name( name )
    , m_tracks( )
{
    //Do nothing...
}

Meta::SpotifyComposer::~SpotifyComposer()
{
    //Do nothing...
}

QString
Meta::SpotifyComposer::name() const
{
    return m_name;
}

Meta::TrackList
Meta::SpotifyComposer::tracks()
{
    return m_tracks;
}

void
Meta::SpotifyComposer::addTrack( SpotifyTrackPtr newTrack )
{
    m_tracks.append( Meta::TrackPtr::staticCast( newTrack ) );
}

Meta::SpotifyGenre::SpotifyGenre( const QString &name )
    : m_name( name )
    , m_tracks( )
{
    //Do nothing...
}

Meta::SpotifyGenre::~SpotifyGenre()
{
    //Do nothing...
}

QString
Meta::SpotifyGenre::name() const
{
    return m_name;
}

Meta::TrackList
Meta::SpotifyGenre::tracks()
{
    return m_tracks;
}

void
Meta::SpotifyGenre::addTrack( SpotifyTrackPtr newTrack )
{
    m_tracks.append( Meta::TrackPtr::staticCast( newTrack ) );
}

Meta::SpotifyYear::SpotifyYear( const QString &name )
    : m_name( name )
    , m_tracks( )
{
    //Do nothing...
}

Meta::SpotifyYear::~SpotifyYear()
{
    //Do nothing...
}

QString
Meta::SpotifyYear::name() const
{
    return m_name;
}

Meta::TrackList
Meta::SpotifyYear::tracks()
{
    return m_tracks;
}

void
Meta::SpotifyYear::addTrack( SpotifyTrackPtr newTrack )
{
    m_tracks.append( Meta::TrackPtr::staticCast( newTrack ) );
}

Meta::SpotifyLabel::SpotifyLabel( const QString &name )
    : m_name( name )
    , m_tracks( )
{
    //Do nothing...
}

Meta::SpotifyLabel::~SpotifyLabel()
{
    //Do nothing...
}

QString
Meta::SpotifyLabel::name() const
{
    return m_name;
}

void
Meta::SpotifyLabel::addTrack( SpotifyTrackPtr newTrack )
{
    m_tracks.append( Meta::TrackPtr::staticCast( newTrack ) );
}
