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

QImage
MemoryMeta::Album::image( int size ) const
{
    if( size > 1 && size <= 1000 && !m_image.isNull() )
        return m_image.scaled( size, size, Qt::KeepAspectRatio, Qt::FastTransformation );
    return m_image;
}

MemoryMeta::Track::Track(const Meta::TrackPtr& originalTrack)
    : m_track( originalTrack )
    , m_album( 0 )
    , m_artist( 0 )
    , m_composer( 0 )
    , m_genre( 0 )
    , m_year( 0 )
{
}

void
MemoryMeta::Track::setAlbum( const Meta::AlbumPtr &album )
{
    m_album = album;
}

void
MemoryMeta::Track::setArtist( const Meta::ArtistPtr &artist )
{
    m_artist = artist;
}

void
MemoryMeta::Track::setComposer( const Meta::ComposerPtr &composer )
{
    m_composer = composer;
}

void
MemoryMeta::Track::setGenre( const Meta::GenrePtr &genre )
{
    m_genre = genre;
}

void
MemoryMeta::Track::setYear( const Meta::YearPtr &year )
{
    m_year = year;
}

MemoryMeta::MapAdder::MapAdder(MemoryCollection* memoryCollection)
    : m_mc( memoryCollection )
{
    m_mc->acquireWriteLock();
}

MemoryMeta::MapAdder::~MapAdder()
{
    m_mc->releaseLock();
}

Meta::TrackPtr MemoryMeta::MapAdder::addTrack(Meta::TrackPtr track)
{
    Track *memoryTrack = new Track( track );
    Meta::TrackPtr metaTrackPtr = Meta::TrackPtr( memoryTrack );
    m_mc->addTrack( metaTrackPtr );

    QString artistName = track->artist().isNull() ? QString() : track->artist()->name();
    Meta::ArtistPtr artist = m_mc->artistMap().value( artistName );
    if( artist.isNull() )
    {
        artist = Meta::ArtistPtr( new Artist( artistName ) );
        m_mc->addArtist( artist );
    }
    static_cast<Artist *>( artist.data() )->addTrack( metaTrackPtr );
    memoryTrack->setArtist( artist );

    QString albumName = track->album().isNull() ? QString() : track->album()->name();
    Meta::AlbumPtr album = m_mc->albumMap().value( albumName );
    if( album.isNull() )
    {
        album = Meta::AlbumPtr( new Album( albumName ) );
        m_mc->addAlbum( album );
    }
    QString albumArtistName;
    if( track->album() && track->album()->hasAlbumArtist() && track->album()->albumArtist() )
        albumArtistName = track->album()->albumArtist()->name();
    Meta::ArtistPtr albumArtist;
    if( !albumArtistName.isEmpty() )
    {
        albumArtist = m_mc->artistMap().value( albumArtistName );
        if( albumArtist.isNull() )
        {
            albumArtist = Meta::ArtistPtr( new Artist( albumArtistName ) );
            m_mc->addArtist( albumArtist );
        }
        // no need to albumArtist->addTrack(), this is not populated for album artists
    }
    bool isCompilation = track->album().isNull() ? false : track->album()->isCompilation();
    Album *memoryAlbum = static_cast<Album *>( album.data() );
    memoryAlbum->addTrack( metaTrackPtr );
    memoryAlbum->setAlbumArtist( albumArtist );  // TODO: do it the other way around
    // be deterministic wrt track adding order:
    memoryAlbum->setIsCompilation( memoryAlbum->isCompilation() || isCompilation );
    QImage albumImage = track->album().isNull() ? QImage() : track->album()->image();
    if( !albumImage.isNull() )
    {
        /* We overwrite album image only if it is bigger than the old one */
        int memoryImageArea = album->image().width() * album->image().height();
        int albumImageArea = albumImage.width() * albumImage.height();
        if( albumImageArea > memoryImageArea )
            album->setImage( albumImage );
    }
    memoryTrack->setAlbum( album );

    QString genreName = track->genre().isNull() ? QString() : track->genre()->name();
    Meta::GenrePtr genre = m_mc->genreMap().value( genreName );
    if( genre.isNull() )
    {
        genre = Meta::GenrePtr( new Genre( genreName ) );
        m_mc->addGenre( genre );
    }
    static_cast<Genre *>( genre.data() )->addTrack( metaTrackPtr );
    memoryTrack->setGenre( genre );

    QString composerName = track->composer().isNull() ? QString() : track->composer()->name();
    Meta::ComposerPtr composer = m_mc->composerMap().value( composerName );
    if( composer.isNull() )
    {
        composer = Meta::ComposerPtr( new Composer( composerName ) );
        m_mc->addComposer( composer );
    }
    static_cast<Composer *>( composer.data() )->addTrack( metaTrackPtr );
    memoryTrack->setComposer( composer );

    int year = track->year().isNull() ? 0 : track->year()->year();
    Meta::YearPtr yearPtr = m_mc->yearMap().value( year );
    if( yearPtr.isNull() )
    {
        yearPtr = Meta::YearPtr( new Year( year ? QString::number( year ) : QString() ) );
        m_mc->addYear( yearPtr );
    }
    static_cast<Year *>( yearPtr.data() )->addTrack( metaTrackPtr );
    memoryTrack->setYear( yearPtr );

    //TODO:labels

    return metaTrackPtr;
}
