/****************************************************************************************
 * Copyright (c) 2010 Bart Cerneels <bart.cerneels@kde.org>                             *
 * Copyright (c) 2011 Lucas Lira Gomes <x8lucas8x@gmail.com>                            *
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

#include "SyncedPlaylist.h"

#include "core/meta/Meta.h"
#include "core/playlists/PlaylistProvider.h"
#include "core/support/Debug.h"

#include <KLocalizedString>

using namespace Meta;

SyncedPlaylist::SyncedPlaylist( const Playlists::PlaylistPtr &playlist )
{
    addPlaylist( playlist );
}

QUrl
SyncedPlaylist::uidUrl() const
{
    return QUrl( QStringLiteral( "amarok-syncedplaylist://" ) +  m_playlists.first()->name() );
}

QString
SyncedPlaylist::name() const
{
    if( isEmpty() )
        return i18n( "<Empty>" );
    return m_playlists.first()->name();
}

QString
SyncedPlaylist::prettyName() const
{
    if( isEmpty() )
        return i18n( "<Empty>" );
    return m_playlists.first()->prettyName();
}

Playlists::PlaylistProvider*
SyncedPlaylist::provider() const
{
    return m_playlists.first()->provider();
}

TrackList
SyncedPlaylist::tracks()
{
    if( isEmpty() )
        return TrackList();

    return m_playlists.first()->tracks();
}

int
SyncedPlaylist::trackCount() const
{
    if( isEmpty() )
        return -1;

    return m_playlists.first()->trackCount();
}

void
SyncedPlaylist::addTrack( const Meta::TrackPtr &track, int position )
{
    //only apply it to the first, the rest will follow in trackAdded()
    m_playlists.first()->addTrack( track, position );
}

void
SyncedPlaylist::removeTrack( int position )
{
    //only apply it to the first, the rest will follow in trackRemoved()
    m_playlists.first()->removeTrack( position );
}

void
SyncedPlaylist::metadataChanged( const Playlists::PlaylistPtr &playlist )
{
    if( !m_playlists.contains( playlist ) )
        return;

    // we pass on every subplaylist change because our description changes
    notifyObserversMetadataChanged();
}

void
SyncedPlaylist::tracksLoaded( Playlists::PlaylistPtr playlist )
{
    if( !m_playlists.contains( playlist ) )
        return;

    // TODO: me may give more thought to this and Q_EMIT tracksLoaded() only when all subplaylists load
    notifyObserversTracksLoaded();
}

void
SyncedPlaylist::trackAdded( const Playlists::PlaylistPtr &playlist, const TrackPtr &track, int position )
{
    if( !m_playlists.contains( playlist ) )
        return;

    if( playlist != m_playlists.first() )
        return; //we only apply changes to the master playlist to the others

    //update the others
    for( Playlists::PlaylistPtr playlistToUpdate : m_playlists )
    {
        if( playlistToUpdate == playlist )
            continue; //no use applying to the one that has already changed
        playlistToUpdate->addTrack( track, position );
    }

    //pass it on
    notifyObserversTrackAdded( track, position );
}

void
SyncedPlaylist::trackRemoved( const Playlists::PlaylistPtr &playlist, int position )
{
    if( !m_playlists.contains( playlist ) )
        return;

    if( playlist != m_playlists.first() )
        return; //we only apply changes to the master playlist to the others

    //update the others
    for( Playlists::PlaylistPtr playlistToUpdate : m_playlists )
    {
        if( playlistToUpdate == playlist )
            continue; //no use applying to the one that has already changed
        playlistToUpdate->removeTrack( position );
    }

    //pass it on
    notifyObserversTrackRemoved( position );
}

bool
SyncedPlaylist::isEmpty() const
{
    return m_playlists.isEmpty();
}

void
SyncedPlaylist::addPlaylist( Playlists::PlaylistPtr playlist )
{
    if( m_playlists.contains( playlist ) )
        return;

    //Only subscribe to the master playlist's changes
    if( m_playlists.isEmpty() )
        subscribeTo( playlist );
    else
    {
        //Deny syncing between playlists in the same provider because
        //there is no use case for it and it does make the code more complex
        if ( (*(m_playlists.begin()))->provider() == playlist->provider() )
        {
            error() << "BUG: You cannot synchronize playlists with the same provider!!!";
            return;
        }
    }

    m_playlists << playlist;
}

bool
SyncedPlaylist::syncNeeded() const
{
    DEBUG_BLOCK
    if( isEmpty() || m_playlists.count() == 1 )
        return false;

    /* Use the first playlist as the base, if the others have a difference
       compared to it a sync is needed */

    QList<Playlists::PlaylistPtr>::const_iterator i = m_playlists.begin();
    Playlists::PlaylistPtr master = *i;
    int masterTrackCount = master->trackCount();
    ++i; //From now on its only slaves on the iterator
    debug() << "Master Playlist: " << master->name() << " - " << master->uidUrl().url();
    debug() << "Master track count: " << masterTrackCount;

    for( ;i != m_playlists.end(); ++i)
    {

        //Playlists::PlaylistPtr slave = i.next();
        Playlists::PlaylistPtr slave = *i;

        debug() << "Slave Playlist: " << slave->name() << " - " << slave->uidUrl().url();
        if( masterTrackCount != -1 )
        {
            int slaveTrackCount = slave->trackCount();
            debug() << "Slave track count: " << slaveTrackCount;
            //If the number of tracks is different a sync is certainly required
            if( slaveTrackCount != -1 && slaveTrackCount != masterTrackCount )
                return true;
        }

        //Compare track by track
        debug() << "Comparing track by track";

        TrackList masterTracks = master->tracks();
        TrackList slaveTracks = slave->tracks();

        for( int i = 0; i < masterTrackCount; i++ )
            if( !( *masterTracks[i] == *slaveTracks[i] ) )
                return true;

    }

    debug() << "No sync needed";

    return false;
}

void
SyncedPlaylist::doSync() const
{
    DEBUG_BLOCK

    QList<Playlists::PlaylistPtr>::const_iterator i = m_playlists.begin();
    Playlists::PlaylistPtr master = *i;
    ++i; //From now on its only slaves on the iterator

    QListIterator<TrackPtr> m( master->tracks() );
    //debug: print list
    int position = 0;
    debug() << "Master Playlist: " << master->name() << " - " << master->uidUrl().url();
    while( m.hasNext() )
        debug() << QStringLiteral( "%1 : %2" ).arg( position++ ).arg( m.next()->name() );
    m.toFront();

    for( ;i != m_playlists.end(); ++i)
    {
        Playlists::PlaylistPtr slave = *i;
        TrackList slaveTracks = slave->tracks();
        //debug: print list
        position = 0;
        debug() << "Slave Playlist: " << slave->name() << " - " << slave->uidUrl().url();
        for( const TrackPtr &track : slaveTracks )
            debug() << QStringLiteral( "%1 : %2" ).arg( position++ ).arg( track->name() );

        //Add first. Tracks in slave that are not in master will eventually shift to the end.
        position = 0;
        while( m.hasNext() )
        {
            TrackPtr track = m.next();
            if( position >= slaveTracks.size()
                    || track->uidUrl() != slaveTracks.at( position )->uidUrl() )
            {
                debug() << QStringLiteral( "insert %2 at %1" ).arg( position ).arg( track->name() );
                slave->addTrack( track, position );

                slave->syncTrackStatus( position, track );

                //update local copy of the tracks
                slaveTracks = slave->tracks();
            }
            position++;
        }

        //debug: print list
        position = 0;
        debug() << "slave playlist after insertions:";
        for( const TrackPtr &track : slaveTracks )
            debug() << QStringLiteral( "%1 : %2" ).arg( position++ ).arg( track->name() );

        //Then remove everything after the position of the last track in master.
        //This removes any tracks that are not in master.
        position = master->tracks().size();

        for( int removeCount = slave->trackCount() - 1; removeCount >= 0; removeCount-- )
            slave->removeTrack( position );

        //debug: print list
        position = 0;
        debug() << "slave playlist after removal:";
        for( const TrackPtr &track : slave->tracks() )
            debug() << QStringLiteral( "%1 : %2" ).arg( position++ ).arg( track->name() );

    }
}

void
SyncedPlaylist::removePlaylistsFrom( Playlists::PlaylistProvider *provider )
{
    for( Playlists::PlaylistPtr playlist : m_playlists )
    {
        if( playlist->provider() == provider )
        {
            unsubscribeFrom( playlist );
            m_playlists.removeAll( playlist );
        }
    }
}

Playlists::PlaylistPtr SyncedPlaylist::master() const
{
    if( m_playlists.isEmpty() )
        return Playlists::PlaylistPtr();

    return m_playlists.first();
}

Playlists::PlaylistList SyncedPlaylist::slaves() const
{
    if( m_playlists.size() < 2 )
        return Playlists::PlaylistList();

    Playlists::PlaylistList slaves;

    std::copy( m_playlists.begin() + 1, m_playlists.end(), slaves.begin() );

    return slaves;
}
