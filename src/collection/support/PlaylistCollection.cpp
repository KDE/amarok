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
#include "playlistmanager/PlaylistProvider.h"

PlaylistCollection::PlaylistCollection( const Meta::PlaylistPtr &playlist )
        : Amarok::Collection()
        , MemoryCollection()
        , Meta::PlaylistObserver()
        , m_playlist( playlist )
{
    subscribeTo( playlist );
    acquireWriteLock();
    foreach( const Meta::TrackPtr &track, m_playlist->tracks() )
    {
        insertTrack( track );
    }
    releaseLock();
}

PlaylistCollection::~PlaylistCollection()
{
    //nothing to do?
}

QueryMaker*
PlaylistCollection::queryMaker()
{
    return new MemoryQueryMaker( this, collectionId() );
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
        acquireWriteLock();
        insertTrack( track );
        releaseLock();
    }
}

void
PlaylistCollection::trackRemoved( Meta::PlaylistPtr playlist, int position )
{
    //ok, what now? is the removed track still availabe at position
    //as this is not clear from the API, and apparently not used
    //anywhere, do it the hard way...
    acquireWriteLock();
    setTrackMap( TrackMap() );
    setArtistMap( ArtistMap() );
    setAlbumMap( AlbumMap() );
    setGenreMap( GenreMap() );
    setComposerMap( ComposerMap() );
    setYearMap( YearMap() );
    foreach( const Meta::TrackPtr &track, playlist->tracks() )
    {
        insertTrack( track );
    }
    releaseLock();
}

//should be moved to MemoryCollection I guess
void
PlaylistCollection::insertTrack( const Meta::TrackPtr &track )
{
    addTrack( track );
    if( track->artist() )
        addArtist( track->artist() );
    if( track->album() )
        addAlbum( track->album() );
    if( track->composer() )
        addComposer( track->composer() );
    if( track->genre() )
        addGenre( track->genre() );
    if( track->year() )
        addYear( track->year() );
}

Meta::PlaylistPtr
PlaylistCollection::playlist() const
{
    return m_playlist;
}
