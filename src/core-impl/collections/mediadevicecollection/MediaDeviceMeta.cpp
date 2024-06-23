/****************************************************************************************
 * Copyright (c) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
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

#include "MediaDeviceMeta.h"

#include "SvgHandler.h"
#include "core/capabilities/ActionsCapability.h"
#include "core/support/Debug.h"
#include "core-impl/capabilities/AlbumActionsCapability.h"
#include "core-impl/collections/mediadevicecollection/MediaDeviceCollection.h"
#include "core-impl/collections/mediadevicecollection/MediaDeviceTrackEditor.h"
#include "core-impl/collections/mediadevicecollection/handler/capabilities/ArtworkCapability.h"
#include "covermanager/CoverCache.h"
#include "covermanager/CoverFetchingActions.h"

#include <QIcon>
#include <QUrl>

#include <QAction>

using namespace Meta;

MediaDeviceTrack::MediaDeviceTrack( Collections::MediaDeviceCollection *collection )
    : Meta::Track()
    , m_collection( collection )
    , m_artist( nullptr )
    , m_album( nullptr )
    , m_genre( nullptr )
    , m_composer( nullptr )
    , m_year( nullptr )
    , m_image()
    , m_comment()
    , m_name()
    , m_type()
    , m_bitrate( 0 )
    , m_filesize( 0 )
    , m_length( 0 )
    , m_discNumber( 0 )
    , m_samplerate( 0 )
    , m_trackNumber( 0 )
    , m_playCount( 0 )
    , m_rating( 0 )
    , m_bpm( 0 )
    , m_replayGain( 0 )
    , m_playableUrl()
{
}

MediaDeviceTrack::~MediaDeviceTrack()
{
    //nothing to do
}

QString
MediaDeviceTrack::name() const
{
    return m_name;
}

QUrl
MediaDeviceTrack::playableUrl() const
{
    return m_playableUrl;
}

QString
MediaDeviceTrack::uidUrl() const
{
    return m_playableUrl.isLocalFile() ? m_playableUrl.toLocalFile() : m_playableUrl.url();
}

QString
MediaDeviceTrack::prettyUrl() const
{
    if( m_playableUrl.isLocalFile() )
        return m_playableUrl.toLocalFile();

    QString collName = m_collection ? m_collection->prettyName() : i18n( "Unknown Collection" );
    QString artistName = artist()? artist()->prettyName() : i18n( "Unknown Artist" );
    // Check name() to prevent infinite recursion
    QString trackName = !name().isEmpty()? prettyName() : i18n( "Unknown track" );

    return  QStringLiteral( "%1: %2 - %3" ).arg( collName, artistName, trackName );
}

QString
MediaDeviceTrack::notPlayableReason() const
{
    return localFileNotPlayableReason( playableUrl().toLocalFile() );
}

bool
MediaDeviceTrack::isEditable() const
{
    if( m_collection )
        return m_collection->isWritable();
    return false;
}

AlbumPtr
MediaDeviceTrack::album() const
{
    return AlbumPtr::staticCast( m_album );
}

ArtistPtr
MediaDeviceTrack::artist() const
{
    return ArtistPtr::staticCast( m_artist );
}

GenrePtr
MediaDeviceTrack::genre() const
{
    return GenrePtr::staticCast( m_genre );
}

ComposerPtr
MediaDeviceTrack::composer() const
{
    return ComposerPtr::staticCast( m_composer );
}

YearPtr
MediaDeviceTrack::year() const
{
    return YearPtr::staticCast( m_year );
}

QString
MediaDeviceTrack::comment() const
{
    return m_comment;
}

void
MediaDeviceTrack::setComment( const QString &newComment )
{
    m_comment = newComment;
}

double
MediaDeviceTrack::score() const
{
    return 0.0;
}

void
MediaDeviceTrack::setScore( double newScore )
{
    Q_UNUSED( newScore )
}

int
MediaDeviceTrack::rating() const
{
    return m_rating;
}

void
MediaDeviceTrack::setRating( int newRating )
{
    if( newRating == m_rating )
        return;
    m_rating = newRating;
    // this method is _not_ called though TrackEditor, notify observers manually
    notifyObservers();
}

qint64
MediaDeviceTrack::length() const
{
    return m_length;
}

void
MediaDeviceTrack::setFileSize( int newFileSize )
{
    m_filesize = newFileSize;
}

int
MediaDeviceTrack::filesize() const
{
    return m_filesize;
}

int
MediaDeviceTrack::bitrate() const
{
    return m_bitrate;
}

void
MediaDeviceTrack::setBitrate( int newBitrate )
{
    m_bitrate = newBitrate;
}

int
MediaDeviceTrack::sampleRate() const
{
    return m_samplerate;
}

void
MediaDeviceTrack::setSamplerate( int newSamplerate )
{
    m_samplerate = newSamplerate;
}

qreal
MediaDeviceTrack::bpm() const
{
    return m_bpm;
}
void
MediaDeviceTrack::setBpm( const qreal newBpm )
{
    m_bpm = newBpm;
}

int
MediaDeviceTrack::trackNumber() const
{
    return m_trackNumber;
}

void
MediaDeviceTrack::setTrackNumber( int newTrackNumber )
{
    m_trackNumber = newTrackNumber;
}

int
MediaDeviceTrack::discNumber() const
{
    return m_discNumber;
}

void
MediaDeviceTrack::setDiscNumber( int newDiscNumber )
{
    m_discNumber = newDiscNumber;
}

int
MediaDeviceTrack::playCount() const
{
    return m_playCount;
}

void
MediaDeviceTrack::setPlayCount( const int newCount )
{
    m_playCount = newCount;
}

QDateTime
MediaDeviceTrack::lastPlayed() const
{
    return m_lastPlayed;
}

void
MediaDeviceTrack::setLastPlayed( const QDateTime &newTime )
{
    m_lastPlayed = newTime;
}

qreal
MediaDeviceTrack::replayGain( ReplayGainTag mode ) const
{
    /* no known non-UMS portable media player is able to differentiante between different
     * replay gain modes (track & album), so store only one value */
    switch( mode ) {
        case Meta::ReplayGain_Track_Gain:
        case Meta::ReplayGain_Album_Gain:
            return m_replayGain;
        case Meta::ReplayGain_Track_Peak:
        case Meta::ReplayGain_Album_Peak:
            // no default label so that compiler emits a warning when new enum value is added
            break;
    }
    return 0.0;
}

void
MediaDeviceTrack::setReplayGain( qreal newReplayGain )
{
    m_replayGain = newReplayGain;
}

QString
MediaDeviceTrack::type() const
{
    if( m_type.isEmpty() && !m_playableUrl.path().isEmpty() )
    {
        QString path = m_playableUrl.path();
        return path.mid( path.lastIndexOf( QLatin1Char('.') ) + 1 );
    }
    return m_type;
}

void
MediaDeviceTrack::setType( const QString & type )
{
    m_type = type;
}

void
MediaDeviceTrack::prepareToPlay()
{
    Meta::MediaDeviceTrackPtr ptr = Meta::MediaDeviceTrackPtr( this );

    if( m_collection && m_collection->handler() )
        m_collection->handler()->prepareToPlay( ptr );
}

// TODO: employ observers (e.g. Handler) to take care of updated
// data
/*
void
MediaDeviceTrack::subscribe( Observer *observer )
{
    Q_UNUSED( observer )    //read only
}

void
MediaDeviceTrack::unsubscribe( Observer *observer )
{
    Q_UNUSED( observer )    //read only
}
*/
// TODO: implement this for MediaDeviceCollectionLocation
bool
MediaDeviceTrack::inCollection() const
{
    return m_collection;  // true is m_collection is not null pointer, false otherwise
}

Collections::Collection*
MediaDeviceTrack::collection() const
{
    return m_collection.data();
}

TrackEditorPtr
MediaDeviceTrack::editor()
{
    return TrackEditorPtr( isEditable() ? new MediaDeviceTrackEditor( this ) : nullptr );
}

StatisticsPtr
MediaDeviceTrack::statistics()
{
    return StatisticsPtr( this );
}

void
MediaDeviceTrack::setAlbum( const QString &newAlbum )
{
    if( !m_collection )
        return;

    MediaDeviceAlbumPtr albumPtr;
    MediaDeviceTrackPtr track( this );
    AlbumMap albumMap = m_collection->memoryCollection()->albumMap();

    // do cleanup of soon to be previous album

    MediaDeviceArtistPtr albumArtist;
    QString albumArtistName;
    albumPtr = m_album;
    if ( !albumPtr.isNull() )
    {
        albumArtist = MediaDeviceArtistPtr::staticCast( albumPtr->albumArtist() );
        if( albumArtist )
            albumArtistName = albumArtist->name();
        // remove track from previous album's tracklist
        albumPtr->remTrack( track );
        // if album's tracklist is empty, remove album from albummap
        if( albumPtr->tracks().isEmpty() )
            albumMap.remove( AlbumPtr::staticCast( albumPtr ) );
    }

    // change to a new album

    // check for the existence of the album to be set to,
    // if album exists, reuse, else create

    if ( albumMap.contains( newAlbum, albumArtistName ) )
    {
        albumPtr = MediaDeviceAlbumPtr::staticCast( albumMap.value( newAlbum, albumArtistName ) );
    }
    else
    {
        albumPtr = MediaDeviceAlbumPtr( new MediaDeviceAlbum( m_collection.data(), newAlbum ) );
        albumPtr->setAlbumArtist( albumArtist );
        albumMap.insert( AlbumPtr::staticCast( albumPtr ) );
    }

    // add track to album's tracklist
    albumPtr->addTrack( track );
    // set track's album to the new album
    setAlbum( albumPtr );

    m_collection->memoryCollection()->acquireWriteLock();
    m_collection->memoryCollection()->setAlbumMap( albumMap );
    m_collection->memoryCollection()->releaseLock();
}

void
MediaDeviceTrack::setAlbumArtist( const QString &newAlbumArtist )
{
    if( !m_collection )
        return;

    if( m_album.isNull() || newAlbumArtist.isEmpty() )
        return;

    MediaDeviceArtistPtr artistPtr;
    ArtistMap artistMap = m_collection->memoryCollection()->artistMap();

    if( artistMap.contains( newAlbumArtist ) )
        artistPtr = MediaDeviceArtistPtr::staticCast( artistMap.value( newAlbumArtist ) );
    else
    {
        artistPtr = MediaDeviceArtistPtr( new MediaDeviceArtist( newAlbumArtist ) );
        artistMap.insert( newAlbumArtist, ArtistPtr::staticCast( artistPtr ) );
    }

    m_album->setAlbumArtist( artistPtr );

    m_collection->memoryCollection()->acquireWriteLock();
    m_collection->memoryCollection()->setArtistMap( artistMap );
    m_collection->memoryCollection()->releaseLock();
}

void
MediaDeviceTrack::setArtist( const QString &newArtist )
{
    if( !m_collection )
        return;

    MediaDeviceArtistPtr artistPtr;
    MediaDeviceTrackPtr track( this );
    ArtistMap artistMap = m_collection->memoryCollection()->artistMap();

    // do cleanup of soon to be previous artist

    artistPtr = m_artist;
    // remove track from previous artist's tracklist
    if ( !artistPtr.isNull() )
    {
        artistPtr->remTrack( track );
        // if artist's tracklist is empty, remove artist from artistmap
        if( artistPtr->tracks().isEmpty() )
            artistMap.remove( artistPtr->name() );
    }

    // change to a new artist

    // check for the existence of the artist to be set to,
    // if artist exists, reuse, else create

    if ( artistMap.contains( newArtist ) )
    {
        artistPtr = MediaDeviceArtistPtr::staticCast( artistMap.value( newArtist ) );
    }
    else
    {
        artistPtr = MediaDeviceArtistPtr( new MediaDeviceArtist( newArtist ) );
        artistMap.insert( newArtist, ArtistPtr::staticCast( artistPtr ) );
    }

    // add track to artist's tracklist
    artistPtr->addTrack( track );
    // set track's artist to the new artist
    setArtist( artistPtr );

    m_collection->memoryCollection()->acquireWriteLock();
    m_collection->memoryCollection()->setArtistMap( artistMap );
    m_collection->memoryCollection()->releaseLock();
}

void
MediaDeviceTrack::setGenre( const QString &newGenre )
{
    if( !m_collection )
        return;

    MediaDeviceGenrePtr genrePtr;
    MediaDeviceTrackPtr track( this );
    GenreMap genreMap = m_collection->memoryCollection()->genreMap();

    // do cleanup of soon to be previous genre

    genrePtr = m_genre;
    if ( !genrePtr.isNull() )
    {
        // remove track from previous genre's tracklist
        genrePtr->remTrack( track );
        // if genre's tracklist is empty, remove genre from genremap
        if( genrePtr->tracks().isEmpty() )
            genreMap.remove( genrePtr->name() );
    }

    // change to a new genre

    // check for the existence of the genre to be set to,
    // if genre exists, reuse, else create

    if ( genreMap.contains( newGenre ) )
    {
        genrePtr = MediaDeviceGenrePtr::staticCast( genreMap.value( newGenre ) );
    }
    else
    {
        genrePtr = MediaDeviceGenrePtr( new MediaDeviceGenre( newGenre ) );
        genreMap.insert( newGenre, GenrePtr::staticCast( genrePtr ) );
    }

    // add track to genre's tracklist
    genrePtr->addTrack( track );
    // set track's genre to the new genre
    setGenre( genrePtr );

    m_collection->memoryCollection()->acquireWriteLock();
    m_collection->memoryCollection()->setGenreMap( genreMap );
    m_collection->memoryCollection()->releaseLock();
}

void
MediaDeviceTrack::setComposer( const QString &newComposer )
{
    if( !m_collection )
        return;

    MediaDeviceComposerPtr composerPtr;
    MediaDeviceTrackPtr track( this );
    ComposerMap composerMap = m_collection->memoryCollection()->composerMap();

    // do cleanup of soon to be previous composer

    composerPtr = m_composer;
    if ( !composerPtr.isNull() )
    {
        // remove track from previous composer's tracklist
        composerPtr->remTrack( track );
        // if composer's tracklist is empty, remove composer from composermap
        if( composerPtr->tracks().isEmpty() )
            composerMap.remove( composerPtr->name() );
    }

    // change to a new composer

    // check for the existence of the composer to be set to,
    // if composer exists, reuse, else create

    if ( composerMap.contains( newComposer ) )
    {
        composerPtr = MediaDeviceComposerPtr::staticCast( composerMap.value( newComposer ) );
    }
    else
    {
        composerPtr = MediaDeviceComposerPtr( new MediaDeviceComposer( newComposer ) );
        composerMap.insert( newComposer, ComposerPtr::staticCast( composerPtr ) );
    }

    // add track to composer's tracklist
    composerPtr->addTrack( track );
    // set track's composer to the new composer
    setComposer( composerPtr );

    m_collection->memoryCollection()->acquireWriteLock();
    m_collection->memoryCollection()->setComposerMap( composerMap );
    m_collection->memoryCollection()->releaseLock();
}

void
MediaDeviceTrack::setYear( int newYear )
{
    if( !m_collection )
        return;

    MediaDeviceYearPtr yearPtr;
    MediaDeviceTrackPtr track( this );
    YearMap yearMap = m_collection->memoryCollection()->yearMap();

    // do cleanup of soon to be previous year

    yearPtr = m_year;
    if ( !yearPtr.isNull() )
    {
        // remove track from previous year's tracklist
        yearPtr->remTrack( track );
        // if year's tracklist is empty, remove year from yearmap
        if( yearPtr->tracks().isEmpty() )
            yearMap.remove( yearPtr->year() );
    }

    // change to a new year

    // check for the existence of the year to be set to,
    // if year exists, reuse, else create

    if ( yearMap.contains( newYear ) )
    {
        yearPtr = MediaDeviceYearPtr::staticCast( yearMap.value( newYear ) );
    }
    else
    {
        yearPtr = MediaDeviceYearPtr( new MediaDeviceYear( QString::number(newYear) ) );
        yearMap.insert( newYear, YearPtr::staticCast( yearPtr ) );
    }

    // add track to year's tracklist
    yearPtr->addTrack( track );
    // set track's year to the new year
    setYear( yearPtr );

    m_collection->memoryCollection()->acquireWriteLock();
    m_collection->memoryCollection()->setYearMap( yearMap );
    m_collection->memoryCollection()->releaseLock();
}

void
MediaDeviceTrack::setAlbum( MediaDeviceAlbumPtr album )
{
    m_album = album;
}

void
MediaDeviceTrack::setArtist( MediaDeviceArtistPtr artist )
{
    m_artist = artist;
}

void
MediaDeviceTrack::setGenre( MediaDeviceGenrePtr genre )
{
    m_genre = genre;
}

void
MediaDeviceTrack::setComposer( MediaDeviceComposerPtr composer )
{
    m_composer = composer;
}

void
MediaDeviceTrack::setYear( MediaDeviceYearPtr year )
{
    m_year = year;
}

QString
MediaDeviceTrack::title() const
{
    return m_name;
}

void
MediaDeviceTrack::setTitle( const QString &title )
{
    m_name = title;
}

void
MediaDeviceTrack::setLength( qint64 length )
{
    m_length = length;
}

void
MediaDeviceTrack::commitChanges()
{
    notifyObservers();
}

//MediaDeviceArtist

MediaDeviceArtist::MediaDeviceArtist( const QString &name )
    : Meta::Artist()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

MediaDeviceArtist::~MediaDeviceArtist()
{
    //nothing to do
}

QString
MediaDeviceArtist::name() const
{
    return m_name;
}

TrackList
MediaDeviceArtist::tracks()
{
    return m_tracks;
}

void
MediaDeviceArtist::addTrack( MediaDeviceTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void
MediaDeviceArtist::remTrack( MediaDeviceTrackPtr track )
{
    m_tracks.removeOne( TrackPtr::staticCast( track ) );
}

//---------------MediaDeviceAlbum-----------------------------------

MediaDeviceAlbum::MediaDeviceAlbum( Collections::MediaDeviceCollection *collection, const QString &name )
    : Meta::Album()
    , m_collection( collection )
    , m_artworkCapability()
    , m_name( name )
    , m_tracks()
    , m_isCompilation( false )
    , m_hasImagePossibility( true ) // assume it has a cover until proven otherwise
    , m_hasImageChecked( false )
    , m_image( QImage() )
    , m_albumArtist( nullptr )
{
    MediaDeviceHandler *handler = m_collection->handler();
    if( handler && handler->hasCapabilityInterface( Handler::Capability::Artwork ) )
        m_artworkCapability = handler->create<Handler::ArtworkCapability>();
}

MediaDeviceAlbum::~MediaDeviceAlbum()
{
    if( m_artworkCapability )
        m_artworkCapability->deleteLater();
    CoverCache::invalidateAlbum( this );
}

QString
MediaDeviceAlbum::name() const
{
    return m_name;
}

bool
MediaDeviceAlbum::isCompilation() const
{
    return m_isCompilation;
}

void
MediaDeviceAlbum::setIsCompilation( bool compilation )
{
    m_isCompilation = compilation;
}

bool
MediaDeviceAlbum::hasAlbumArtist() const
{
    return !m_albumArtist.isNull();
}

ArtistPtr
MediaDeviceAlbum::albumArtist() const
{
    return ArtistPtr::staticCast( m_albumArtist );
}

TrackList
MediaDeviceAlbum::tracks()
{
    return m_tracks;
}

bool
MediaDeviceAlbum::hasImage( int size ) const
{
    Q_UNUSED( size )

    if( !m_hasImageChecked )
        m_hasImagePossibility = ! const_cast<MediaDeviceAlbum*>( this )->image().isNull();
    return m_hasImagePossibility;
}

QImage
MediaDeviceAlbum::image( int size ) const
{
    if( m_name.isEmpty() || !m_hasImagePossibility || m_tracks.isEmpty() )
        return Meta::Album::image( size );

    if( m_image.isNull() && m_artworkCapability )
    {
        MediaDeviceTrackPtr track = MediaDeviceTrackPtr::staticCast( m_tracks.first() );
        m_image = m_artworkCapability->getCover( track );
        m_hasImagePossibility = !m_image.isNull();
        m_hasImageChecked = true;
        CoverCache::invalidateAlbum( this );
    }

    if( !m_image.isNull() )
    {
        if( !size )
            return m_image;
        return m_image.scaled( QSize( size, size ), Qt::KeepAspectRatio );
    }
    return Meta::Album::image( size );
}

bool
MediaDeviceAlbum::canUpdateImage() const
{
    if( m_artworkCapability )
        return m_artworkCapability->canUpdateCover();
    return false;
}

void
MediaDeviceAlbum::setImage( const QImage &image )
{
    if( m_artworkCapability && m_artworkCapability->canUpdateCover() )
    {
        // reset to initial values, let next call to image() re-fetch it
        m_hasImagePossibility = true;
        m_hasImageChecked = false;

        m_artworkCapability->setCover( MediaDeviceAlbumPtr( this ), image );
        CoverCache::invalidateAlbum( this );
    }
}

void
MediaDeviceAlbum::setImagePath( const QString &path )
{
    if( m_artworkCapability && m_artworkCapability->canUpdateCover() )
    {
        // reset to initial values, let next call to image() re-fetch it
        m_hasImagePossibility = true;
        m_hasImageChecked = false;

        m_artworkCapability->setCoverPath( MediaDeviceAlbumPtr( this ), path );
        CoverCache::invalidateAlbum( this );
    }
}

bool
MediaDeviceAlbum::hasCapabilityInterface( Capabilities::Capability::Type type ) const
{
    switch( type )
    {
        case Capabilities::Capability::Actions:
            return true;
        default:
            return false;
    }
}

Capabilities::Capability*
MediaDeviceAlbum::createCapabilityInterface( Capabilities::Capability::Type type )
{
    switch( type )
    {
        case Capabilities::Capability::Actions:
            return new Capabilities::AlbumActionsCapability( Meta::AlbumPtr( this ) );

        default:
            return nullptr;
    }
}

void
MediaDeviceAlbum::addTrack( MediaDeviceTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void
MediaDeviceAlbum::remTrack( MediaDeviceTrackPtr track )
{
    m_tracks.removeOne( TrackPtr::staticCast( track ) );
}

void
MediaDeviceAlbum::setAlbumArtist( MediaDeviceArtistPtr artist )
{
    m_albumArtist = artist;
}

//MediaDeviceComposer

MediaDeviceComposer::MediaDeviceComposer( const QString &name )
    : Meta::Composer()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

MediaDeviceComposer::~MediaDeviceComposer()
{
    //nothing to do
}

QString
MediaDeviceComposer::name() const
{
    return m_name;
}

TrackList
MediaDeviceComposer::tracks()
{
    return m_tracks;
}

void
MediaDeviceComposer::addTrack( MediaDeviceTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void
MediaDeviceComposer::remTrack( MediaDeviceTrackPtr track )
{
    m_tracks.removeOne( TrackPtr::staticCast( track ) );
}

//---------------MediaDeviceGenre-----------------------------------

MediaDeviceGenre::MediaDeviceGenre( const QString &name )
    : Meta::Genre()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

MediaDeviceGenre::~MediaDeviceGenre()
{
    //nothing to do
}

QString
MediaDeviceGenre::name() const
{
    return m_name;
}

TrackList
MediaDeviceGenre::tracks()
{
    return m_tracks;
}

void
MediaDeviceGenre::addTrack( MediaDeviceTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void
MediaDeviceGenre::remTrack( MediaDeviceTrackPtr track )
{
    m_tracks.removeOne( TrackPtr::staticCast( track ) );
}


//MediaDeviceYear

MediaDeviceYear::MediaDeviceYear( const QString &name )
    : Meta::Year()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

MediaDeviceYear::~MediaDeviceYear()
{
    //nothing to do
}

QString
MediaDeviceYear::name() const
{
    return m_name;
}

TrackList
MediaDeviceYear::tracks()
{
    return m_tracks;
}

void
MediaDeviceYear::addTrack( MediaDeviceTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void
MediaDeviceYear::remTrack( MediaDeviceTrackPtr track )
{
    m_tracks.removeOne( TrackPtr::staticCast( track ) );
}
