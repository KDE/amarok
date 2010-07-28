/****************************************************************************************
 * Copyright (c) 2010 Andrew Coder <andrew.coder@gmail.com                              *
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

#include "core/meta/Meta.h"
#include "core-impl/meta/default/DefaultMetaTypes.h"
#include "covermanager/CoverFetcher.h"
#include "PlaydarCollection.h"

#include <QDateTime>
#include <QList>
#include <QPixmap>
#include <QString>

#include <KSharedPtr>
#include <KUrl>

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
                                  QString &type,
                                  double score,
                                  qint64 length,
                                  int bitrate,
                                  int filesize,
                                  QString &source )
    : m_album( new PlaydarAlbum( album ) )
    , m_artist( new PlaydarArtist( artist ) )
    , m_composer( new PlaydarComposer( QString( "" ) ) )
    , m_genre( new PlaydarGenre( QString( "" ) ) )
    , m_year( new PlaydarYear( QString( "" ) ) )
    , m_labelList(  )
    , m_sid( sid )
    , m_uidUrl( )
    , m_playableUrl( playableUrl )
    , m_name( name )
    , m_type( type )
    , m_score( score )
    , m_length( length )
    , m_bitrate( bitrate )
    , m_filesize( filesize )
    , m_trackNumber( 0 )
    , m_discNumber( 0 )
    , m_createDate( QDateTime::currentDateTime() )
    , m_comment( QString( "" ) )
    , m_rating( 0 )
    , m_source( source )
{
    m_uidUrl.setProtocol( QString( "playdar" ) );
    m_uidUrl.addPath( source );
    m_uidUrl.addQueryItem( QString( "artist" ), artist );
    m_uidUrl.addQueryItem( QString( "album" ), album );
    m_uidUrl.addQueryItem( QString( "title" ), name );
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

QString
Meta::PlaydarTrack::prettyName() const
{
    return m_name;
}

KUrl
Meta::PlaydarTrack::playableUrl() const
{
    return KUrl( m_playableUrl );
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

bool
Meta::PlaydarTrack::isPlayable() const
{
    /** TODO: Can this be smart? */
    return true;
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
    foreach( const PlaydarLabelPtr &label, m_labelList )
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

void
Meta::PlaydarTrack::setScore( double newScore )
{
    m_score = newScore;
}

int
Meta::PlaydarTrack::rating() const
{
    return m_rating;
}

void
Meta::PlaydarTrack::setRating( int newRating )
{
    m_rating = newRating;
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

uint
Meta::PlaydarTrack::lastPlayed() const
{
    return 0;
}

uint
Meta::PlaydarTrack::firstPlayed() const
{
    return 0;
}

int
Meta::PlaydarTrack::playCount() const
{
    return 0;
}

QString
Meta::PlaydarTrack::type() const
{
    return m_type;
}

void
Meta::PlaydarTrack::prepareToPlay()
{
    /** TODO: Anything? */
}

void
Meta::PlaydarTrack::finishedPlaying( double playedFraction )
{
    /** TODO: Anything */
    Q_UNUSED( playedFraction );
}

bool
Meta::PlaydarTrack::inCollection() const
{
    if( m_collection )
        return true;
    else
        return false;
}

Collections::Collection*
Meta::PlaydarTrack::collection() const
{
    return m_collection;
}

QString
Meta::PlaydarTrack::cachedLyrics() const
{
    return QString( "" );
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
    foreach( const PlaydarLabelPtr &labelPtr, m_labelList )
    {
        if( labelPtr->name() == label->name() )
        {
            m_labelList.removeOne( labelPtr );
            return;
        }
    }
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
    if( m_collection )
    {
        PlaydarTrackPtr sharedThis( this );
        m_collection->addNewTrack( sharedThis );
    }
}

void
Meta::PlaydarTrack::setAlbum( PlaydarAlbumPtr album )
{
    m_album = album;
}

void
Meta::PlaydarTrack::setArtist( PlaydarArtistPtr artist )
{
    m_artist = artist;
}

void
Meta::PlaydarTrack::setComposer( PlaydarComposerPtr composer )
{
    m_composer = composer;
}

void
Meta::PlaydarTrack::setGenre( PlaydarGenrePtr genre )
{
    m_genre = genre;
}

void
Meta::PlaydarTrack::setYear( PlaydarYearPtr year )
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

QString
Meta::PlaydarArtist::prettyName() const
{
    return name();
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
Meta::PlaydarArtist::addTrack( PlaydarTrackPtr newTrack )
{
    m_tracks.append( TrackPtr::staticCast( newTrack ) );
}

void
Meta::PlaydarArtist::addAlbum( PlaydarAlbumPtr newAlbum )
{
    m_albums.append( AlbumPtr::staticCast( newAlbum ) );
}

Meta::PlaydarAlbum::PlaydarAlbum( const QString &name )
    : m_name( name )
    , m_tracks( )
    , m_isCompilation( false )
    , m_albumArtist( 0 )
{
    //Do nothing...
}

Meta::PlaydarAlbum::~PlaydarAlbum()
{
    //Do nothing...
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

QString
Meta::PlaydarAlbum::prettyName() const
{
    return name();
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

QPixmap
Meta::PlaydarAlbum::image( int size )
{
    if ( m_cover.isNull() )
    {
        if( !m_suppressImageAutoFetch && !m_name.isEmpty() )
            CoverFetcher::instance()->queueAlbum( AlbumPtr(this) );
        
        return Meta::Album::image( size );
    }
    
    if ( m_coverSizeMap.contains( size ) )
        return m_coverSizeMap.value( size );
    
    QPixmap scaled = m_cover.scaled( size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation );
    
    m_coverSizeMap.insert( size, scaled );
    return scaled;
}

KUrl
Meta::PlaydarAlbum::imageLocation( int size )
{
    if( !m_cover.isNull() )
        return KUrl();

    return Meta::Album::imageLocation( size );
}

bool
Meta::PlaydarAlbum::canUpdateImage() const
{
    return true;
}

void
Meta::PlaydarAlbum::setImage( const QPixmap &pixmap )
{
    m_cover = pixmap;
}

void
Meta::PlaydarAlbum::removeImage()
{
    m_coverSizeMap.clear();
    m_cover = QPixmap();
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
Meta::PlaydarAlbum::addTrack( PlaydarTrackPtr newTrack )
{
    m_tracks.append( TrackPtr::staticCast( newTrack ) );
}

void
Meta::PlaydarAlbum::setAlbumArtist( PlaydarArtistPtr newArtist )
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

QString
Meta::PlaydarComposer::prettyName() const
{
    return name();
}

Meta::TrackList
Meta::PlaydarComposer::tracks()
{
    return m_tracks;
}

void
Meta::PlaydarComposer::addTrack( PlaydarTrackPtr newTrack )
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

QString
Meta::PlaydarGenre::prettyName() const
{
    return name();
}

Meta::TrackList
Meta::PlaydarGenre::tracks()
{
    return m_tracks;
}

void
Meta::PlaydarGenre::addTrack( PlaydarTrackPtr newTrack )
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

QString
Meta::PlaydarYear::prettyName() const
{
    return name();
}

Meta::TrackList
Meta::PlaydarYear::tracks()
{
    return m_tracks;
}

void
Meta::PlaydarYear::addTrack( PlaydarTrackPtr newTrack )
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

QString
Meta::PlaydarLabel::prettyName() const
{
    return name();
}

void
Meta::PlaydarLabel::addTrack( PlaydarTrackPtr newTrack )
{
    m_tracks.append( Meta::TrackPtr::staticCast( newTrack ) );
}
