/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#include "PlaylistCollection.h"

#include "MemoryQueryMaker.h"
#include "PlaylistCollectionLocation.h"
#include "playlists/PlaylistProvider.h"

PlaylistCollection::PlaylistCollection( const Meta::PlaylistPtr &playlist )
        : Amarok::Collection()
        , Meta::PlaylistObserver()
        , m_playlist( playlist )
        , m_mc( new MemoryCollection() )
{
    subscribeTo( playlist );
    m_mc->acquireWriteLock();
    foreach( const Meta::TrackPtr &track, m_playlist->tracks() )
    {
        insertTrack( track );
    }
    m_mc->releaseLock();
}

PlaylistCollection::~PlaylistCollection()
{
    //nothing to do?
}

QueryMaker*
PlaylistCollection::queryMaker()
{
    return new MemoryQueryMaker( m_mc.toWeakRef(), collectionId() );
}

CollectionLocation*
PlaylistCollection::location() const
{
    return new PlaylistCollectionLocation( this );
}

QString
PlaylistCollection::collectionId() const
{
    return m_playlist->name(); //add prefix?
}

QString
PlaylistCollection::prettyName() const
{
    return m_playlist->prettyName();
}

KIcon
PlaylistCollection::icon() const
{
    if( m_playlist->provider() )
        return m_playlist->provider()->icon();
    else
        return KIcon();
}

void
PlaylistCollection::trackAdded( Meta::PlaylistPtr playlist, Meta::TrackPtr track, int position )
{
    Q_UNUSED( position );
    if( playlist != m_playlist )
        return;

    if( track )
    {
        m_mc->acquireWriteLock();
        insertTrack( track );
        m_mc->releaseLock();
    }
}

void
PlaylistCollection::trackRemoved( Meta::PlaylistPtr playlist, int position )
{
    Q_UNUSED( position )

    //ok, what now? is the removed track still availabe at position
    //as this is not clear from the API, and apparently not used
    //anywhere, do it the hard way...
    m_mc->acquireWriteLock();
    m_mc->setTrackMap( TrackMap() );
    m_mc->setArtistMap( ArtistMap() );
    m_mc->setAlbumMap( AlbumMap() );
    m_mc->setGenreMap( GenreMap() );
    m_mc->setComposerMap( ComposerMap() );
    m_mc->setYearMap( YearMap() );
    foreach( const Meta::TrackPtr &track, playlist->tracks() )
    {
        insertTrack( track );
    }
    m_mc->releaseLock();
}

//should be moved to MemoryCollection I guess
void
PlaylistCollection::insertTrack( const Meta::TrackPtr &track )
{
    m_mc->addTrack( track );
    if( track->artist() )
        m_mc->addArtist( track->artist() );
    if( track->album() )
        m_mc->addAlbum( track->album() );
    if( track->composer() )
        m_mc->addComposer( track->composer() );
    if( track->genre() )
        m_mc->addGenre( track->genre() );
    if( track->year() )
        m_mc->addYear( track->year() );
}

Meta::PlaylistPtr
PlaylistCollection::playlist() const
{
    return m_playlist;
}
