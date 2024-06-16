/****************************************************************************************
 * Copyright (c) 2011 Bart Cerneels <bart.cerneels@kde.org>                             *
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#include "MemoryMeta.h"

#include "core/meta/Statistics.h"
#include "core/meta/TrackEditor.h"
#include "core-impl/capabilities/AlbumActionsCapability.h"
#include "covermanager/CoverCache.h"


using namespace MemoryMeta;

Meta::TrackList
Base::tracks()
{
    // construct AmarokSharedPointers on demand, see m_track comment
    QReadLocker locker( &m_tracksLock );
    Meta::TrackList list;
    for( Track *track : m_tracks )
    {
        list << Meta::TrackPtr( track );
    }
    return list;
}

void
Base::addTrack( Track *track )
{
    QWriteLocker locker( &m_tracksLock );
    m_tracks.append( track );
}

void
Base::removeTrack( Track *track )
{
    QWriteLocker locker( &m_tracksLock );
    m_tracks.removeOne( track );
}

Album::Album( const Meta::AlbumPtr &other )
    : MemoryMeta::Base( other->name() )
    , m_isCompilation( other->isCompilation() )
    , m_canUpdateCompilation( other->canUpdateCompilation() )
    , m_image( other->image() )
    , m_canUpdateImage( other->canUpdateImage() )
{
    if( other->hasAlbumArtist() && other->albumArtist() )
        m_albumArtist = Meta::ArtistPtr( new Artist( other->albumArtist()->name() ) );
}

bool
Album::hasCapabilityInterface( Capabilities::Capability::Type type ) const
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
Album::createCapabilityInterface( Capabilities::Capability::Type type )
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
Album::setCompilation( bool isCompilation )
{
    // we re-create the albums for each track - that's how MemoryMeta works - by
    // aggregating multiple entities of same name into one.
    for( Meta::TrackPtr track : tracks() )
    {
        Track *memoryTrack = static_cast<Track *>( track.data() );
        Meta::AlbumPtr album = memoryTrack->originalTrack()->album();
        if( album && album->canUpdateCompilation() )
            album->setCompilation( isCompilation );
    }
    // no notifyObservers() etc needed - this is done from down-up by proxied tracks
}

QImage
Album::image( int size ) const
{
    if( size > 1 && size <= 1000 && !m_image.isNull() )
        return m_image.scaled( size, size, Qt::KeepAspectRatio, Qt::FastTransformation );
    return m_image;
}

void
Album::setImage( const QImage &image )
{
    for( Meta::TrackPtr track : tracks() )
    {
        Track *memoryTrack = static_cast<Track *>( track.data() );
        Meta::AlbumPtr album = memoryTrack->originalTrack()->album();
        if( album && album->canUpdateImage() )
            album->setImage( image );
    }
    // no notifyObservers() etc needed - this is done from down-up by proxied tracks
}

void
Album::removeImage()
{
    for( Meta::TrackPtr track : tracks() )
    {
        Track *memoryTrack = static_cast<Track *>( track.data() );
        Meta::AlbumPtr album = memoryTrack->originalTrack()->album();
        if( album && album->canUpdateImage() )
            album->removeImage();
    }
    // no notifyObservers() etc needed - this is done from down-up by proxied tracks
}

void
Album::updateCachedValues()
{
    m_isCompilation = false;
    m_canUpdateCompilation = false;
    m_image = QImage();
    m_canUpdateImage = false;
    for( Meta::TrackPtr track : tracks() )
    {
        Track *memoryTrack = static_cast<Track *>( track.data() );
        Meta::AlbumPtr album = memoryTrack->originalTrack()->album();

        if( !album )
            continue;

        if( !m_isCompilation )
            m_isCompilation = album->isCompilation();
        if( !m_canUpdateCompilation )
            m_canUpdateCompilation = album->canUpdateCompilation();
        if( m_image.isNull() && album->hasImage() )
            m_image = album->image();
        if( !m_canUpdateImage )
            m_canUpdateImage = album->canUpdateImage();
    }
}

Track::Track(const Meta::TrackPtr& originalTrack)
    : m_track( originalTrack )
    , m_album( nullptr )
    , m_artist( nullptr )
    , m_composer( nullptr )
    , m_genre( nullptr )
    , m_year( nullptr )
{
    Q_ASSERT( originalTrack );
}

Track::~Track()
{
    // all following static casts are valid - there is no way attributes could have been
    // set to different Meta::* subclasses
    if( m_album )
        static_cast<Album *>( m_album.data() )->removeTrack( this );
    if( m_artist )
        static_cast<Artist *>( m_artist.data() )->removeTrack( this );
    if( m_composer )
        static_cast<Composer *>( m_composer.data() )->removeTrack( this );
    if( m_genre )
        static_cast<Genre *>( m_genre.data() )->removeTrack( this );
    if( m_year )
        static_cast<Year *>( m_year.data() )->removeTrack( this );
}

Meta::TrackEditorPtr
Track::editor()
{
    return m_track->editor();
}

Meta::StatisticsPtr
Track::statistics()
{
    return m_track->statistics();
}

void
Track::setAlbum( Album *album )
{
    if( m_album )
        static_cast<Album *>( m_album.data() )->removeTrack( this );
    if( album )
        album->addTrack( this );
    m_album = Meta::AlbumPtr( album );
}

void
Track::setArtist( Artist *artist )
{
    if( m_artist )
        static_cast<Artist *>( m_artist.data() )->removeTrack( this );
    if( artist )
        artist->addTrack( this );
    m_artist = Meta::ArtistPtr( artist );
}

void
Track::setComposer( Composer *composer )
{
    if( m_composer )
        static_cast<Composer *>( m_composer.data() )->removeTrack( this );
    if( composer )
        composer->addTrack( this );
    m_composer = Meta::ComposerPtr( composer );
}

void
Track::setGenre( Genre *genre )
{
    if( m_genre )
        static_cast<Genre *>( m_genre.data() )->removeTrack( this );
    if( genre )
        genre->addTrack( this );
    m_genre = Meta::GenrePtr( genre );
}

void
Track::setYear( Year *year )
{
    if( m_year )
        static_cast<Year *>( m_year.data() )->removeTrack( this );
    if( year )
        year->addTrack( this );
    m_year = Meta::YearPtr( year );
}

MapChanger::MapChanger( MemoryCollection *memoryCollection )
    : m_mc( memoryCollection )
{
    m_mc->acquireWriteLock();
}

MapChanger::~MapChanger()
{
    m_mc->releaseLock();
}

Meta::TrackPtr
MapChanger::addTrack( Meta::TrackPtr track )
{
    if( !track )
        return Meta::TrackPtr(); // nothing to do
    if( m_mc->trackMap().contains( track->uidUrl() ) )
        return Meta::TrackPtr();

    Track *memoryTrack = new Track( track );
    return addExistingTrack( track, memoryTrack );
}

Meta::TrackPtr
MapChanger::addExistingTrack( Meta::TrackPtr track, Track *memoryTrack )
{
    Meta::TrackPtr metaTrackPtr = Meta::TrackPtr( memoryTrack );
    m_mc->addTrack( metaTrackPtr );

    QString artistName = track->artist().isNull() ? QString() : track->artist()->name();
    Meta::ArtistPtr artist = m_mc->artistMap().value( artistName );
    if( artist.isNull() )
    {
        artist = Meta::ArtistPtr( new Artist( artistName ) );
        m_mc->addArtist( artist );
    }
    memoryTrack->setArtist( static_cast<Artist *>( artist.data() ) );

    Meta::AlbumPtr trackAlbum = track->album();
    QString albumName = trackAlbum ? trackAlbum->name() : QString();
    QString albumArtistName;
    if( trackAlbum && trackAlbum->hasAlbumArtist() && trackAlbum->albumArtist() )
        albumArtistName = trackAlbum->albumArtist()->name();
    Meta::AlbumPtr album = m_mc->albumMap().value( albumName, albumArtistName );
    if( !album )
    {
        Meta::ArtistPtr albumArtist;
        const bool isCompilation = trackAlbum ? trackAlbum->isCompilation() : false;
        // we create even empty-named album artists if the album is not compilation
        // because Collection Browser wouldn't show these otherwise
        if( !albumArtistName.isEmpty() || !isCompilation )
        {
            /* Even if MemoryQueryMaker doesn't need albumArtists to be in MemoryCollection
             * maps, we add her into artist map so that album artist has the same instance as
             * identically-named artist (of potentially different tracks) */
            albumArtist = m_mc->artistMap().value( albumArtistName );
            if( !albumArtist )
            {
                albumArtist = Meta::ArtistPtr( new Artist( albumArtistName ) );
                m_mc->addArtist( albumArtist );
            }
        }

        album = Meta::AlbumPtr( new Album( albumName, albumArtist ) );
        m_mc->addAlbum( album );
    }
    Album *memoryAlbum = static_cast<Album *>( album.data() );
    memoryTrack->setAlbum( memoryAlbum );
    memoryAlbum->updateCachedValues();

    QString genreName = track->genre().isNull() ? QString() : track->genre()->name();
    Meta::GenrePtr genre = m_mc->genreMap().value( genreName );
    if( genre.isNull() )
    {
        genre = Meta::GenrePtr( new Genre( genreName ) );
        m_mc->addGenre( genre );
    }
    memoryTrack->setGenre( static_cast<Genre *>( genre.data() ) );

    QString composerName = track->composer().isNull() ? QString() : track->composer()->name();
    Meta::ComposerPtr composer = m_mc->composerMap().value( composerName );
    if( composer.isNull() )
    {
        composer = Meta::ComposerPtr( new Composer( composerName ) );
        m_mc->addComposer( composer );
    }
    memoryTrack->setComposer( static_cast<Composer *>( composer.data() ) );

    int year = track->year().isNull() ? 0 : track->year()->year();
    Meta::YearPtr yearPtr = m_mc->yearMap().value( year );
    if( yearPtr.isNull() )
    {
        yearPtr = Meta::YearPtr( new Year( year ? QString::number( year ) : QString() ) );
        m_mc->addYear( yearPtr );
    }
    memoryTrack->setYear( static_cast<Year *>( yearPtr.data() ) );

    //TODO: labels (when doing this, don't forget to tweak removeTrack too)

    return metaTrackPtr;
}

Meta::TrackPtr
MapChanger::removeTrack( Meta::TrackPtr track )
{
    if( !track )
        return Meta::TrackPtr(); // nothing to do

    TrackMap trackMap = m_mc->trackMap();
    ArtistMap artistMap = m_mc->artistMap();
    AlbumMap albumMap = m_mc->albumMap();
    GenreMap genreMap = m_mc->genreMap();
    ComposerMap composerMap = m_mc->composerMap();
    YearMap yearMap = m_mc->yearMap();

    /* Ensure that we have the memory track (not the underlying one) and that it is
     * actually present in MemoryCollection */
    track = trackMap.value( track->uidUrl() );
    if( !track )
        return Meta::TrackPtr(); // was not in collection, nothing to do

    /* Track added using mapadder are MemoryMeta::Tracks, but cope with different too: */
    Track *memoryTrack = dynamic_cast<Track *>( track.data() );

    trackMap.remove( track->uidUrl() );

    /* Remove potentially dangling entities from memory collection. We cannot simply use
     * if( entity && entity->tracks().count() == 1 ) because it would be racy: entity could
     * have referenced a track that is no longer in MemoryCollection, but not yet destroyed.
     *
     * When track to remove is MemoryMeta::Track, copy and reassign Meta:: entities so
     * that the track is detached from MemoryCollection completely. */

    Meta::ArtistPtr artist = track->artist();
    if( artist && !hasTrackInMap( artist->tracks(), trackMap )
               && !referencedAsAlbumArtist( artist, albumMap ) )
        artistMap.remove( artist->name() );
    if( artist && memoryTrack )
        memoryTrack->setArtist( new Artist( artist->name() ) );

    Meta::AlbumPtr album = track->album();
    if( album && !hasTrackInMap( album->tracks(), trackMap ) )
    {
        albumMap.remove( album );
        Meta::ArtistPtr albumArtist = album->hasAlbumArtist() ? album->albumArtist() : Meta::ArtistPtr();
        if( albumArtist && !hasTrackInMap( albumArtist->tracks(), trackMap )
                        && !referencedAsAlbumArtist( albumArtist, albumMap ) )
            artistMap.remove( albumArtist->name() );
    }
    if( album && memoryTrack )
        memoryTrack->setAlbum( new Album( album ) ); // copy-like constructor

    Meta::GenrePtr genre = track->genre();
    if( genre && !hasTrackInMap( genre->tracks(), trackMap ) )
        genreMap.remove( genre->name() );
    if( genre && memoryTrack )
        memoryTrack->setGenre( new Genre( genre->name() ) );

    Meta::ComposerPtr composer = track->composer();
    if( composer && !hasTrackInMap( composer->tracks(), trackMap ) )
        composerMap.remove( composer->name() );
    if( composer && memoryTrack )
        memoryTrack->setComposer( new Composer( composer->name() ) );

    Meta::YearPtr year = track->year();
    if( year && !hasTrackInMap( year->tracks(), trackMap ) )
        yearMap.remove( year->year() );
    if( year && memoryTrack )
        memoryTrack->setYear( new Year( year->name() ) );

    m_mc->setTrackMap( trackMap );
    m_mc->setArtistMap( artistMap );
    m_mc->setAlbumMap( albumMap );
    m_mc->setGenreMap( genreMap );
    m_mc->setComposerMap( composerMap );
    m_mc->setYearMap( yearMap );

    if( memoryTrack )
        return memoryTrack->originalTrack();
    return Meta::TrackPtr();
}

bool
MapChanger::trackChanged( Meta::TrackPtr track )
{
    if( !track )
        return false;
    // make sure we have a track from memory collection:
    track = m_mc->trackMap().value( track->uidUrl() );
    if( !track )
        return false;
    Track *memoryTrack = dynamic_cast<Track *>( track.data() );
    if( !memoryTrack )
        return false;

    Meta::TrackPtr originalTrack = memoryTrack->originalTrack();
    if( !originalTrack )
        return false; // paranoia

    bool mapsNeedUpdating = false;
    // make album first so that invalidateAlbum is called every time it is needed
    if( entitiesDiffer( originalTrack->album().data(), memoryTrack->album().data() ) )
    {
        CoverCache::invalidateAlbum( memoryTrack->album().data() );
        mapsNeedUpdating = true;
    }
    else if( entitiesDiffer( originalTrack->artist().data(), memoryTrack->artist().data() ) )
        mapsNeedUpdating = true;
    else if( entitiesDiffer( originalTrack->genre().data(), memoryTrack->genre().data() ) )
        mapsNeedUpdating = true;
    else if( entitiesDiffer( originalTrack->composer().data(), memoryTrack->composer().data() ) )
        mapsNeedUpdating = true;
    else if( entitiesDiffer( originalTrack->year().data(), memoryTrack->year().data() ) )
        mapsNeedUpdating = true;

    if( mapsNeedUpdating )
    {
        // we hold write lock so we can do the following trick:
        removeTrack( track );
        addExistingTrack( originalTrack, memoryTrack );
    }

    memoryTrack->notifyObservers();
    return mapsNeedUpdating;
}

bool
MapChanger::hasTrackInMap( const Meta::TrackList &needles, const TrackMap &haystack )
{
    for( Meta::TrackPtr track : needles )
    {
        if( track && haystack.contains( track->uidUrl() ) )
            return true;
    }
    return false;
}

bool
MapChanger::referencedAsAlbumArtist( const Meta::ArtistPtr &artist, const AlbumMap &haystack )
{
    for( Meta::AlbumPtr album : haystack )
    {
        if( album && album->hasAlbumArtist() && album->albumArtist() == artist )
            return true;
    }
    return false;
}

bool MapChanger::entitiesDiffer( const Meta::Base *first, const Meta::Base *second )
{
    if( !first && !second )
        return false;  // both null -> do not differ
    if( !first || !second )
        return true;  // one of them null -> do differ

    return first->name() != second->name();
}

bool MapChanger::entitiesDiffer( const Meta::Album *first, const Meta::Album *second )
{
    if( !first && !second )
        return false;  // both null -> do not differ
    if( !first || !second )
        return true;  // one of them null -> do differ

    if( first->name() != second->name() )
        return true;
    if( first->isCompilation() != second->isCompilation() )
        return true;
    if( first->hasImage() != second->hasImage() )
        return true;
    if( entitiesDiffer( first->albumArtist().data(), second->albumArtist().data() ) )
        return true;

    // we only compare images using dimension, it would be too costly otherwise
    if( first->image().width() != second->image().width() )
        return true;
    if( first->image().height() != second->image().height() )
        return true;

    return false;
}
