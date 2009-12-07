#include "SyncedPlaylist.h"
#include <src/core/playlists/PlaylistProvider.h>

#include "core/support/Debug.h"

#include <KLocale>

using namespace Meta;
SyncedPlaylist::SyncedPlaylist( Playlists::PlaylistPtr playlist )
{
    addPlaylist( playlist );
}

KUrl
SyncedPlaylist::uidUrl() const
{
    return KUrl( QString( "amarok-syncedplaylist://1") );
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

QString
SyncedPlaylist::description() const
{
    if( isEmpty() )
        return i18n( "<Empty>" );
    QStringList providerNames;
    foreach( const Playlists::PlaylistPtr playlist, m_playlists )
    {
        if( playlist && playlist->provider() )
            providerNames << playlist->provider()->prettyName();
    }
    return i18n( "Synchronized on: %1" ).arg( providerNames.join( ", " ) );
}

TrackList
SyncedPlaylist::tracks()
{
    if( isEmpty() )
        return TrackList();

    return m_playlists.first()->tracks();
}

void
SyncedPlaylist::addTrack( Meta::TrackPtr track, int position )
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
SyncedPlaylist::trackAdded( Playlists::PlaylistPtr playlist, TrackPtr track, int position )
{
    if( !m_playlists.contains( playlist ) )
        return;

    if( playlist != m_playlists.first() )
        return; //we only apply changes to the master playlist to the others

    //update the others
    foreach( Playlists::PlaylistPtr playlistToUpdate, m_playlists )
    {
        if( playlistToUpdate == playlist )
            continue; //no use applying to the one that has already changed
        playlistToUpdate->addTrack( track, position );
    }

    //pass it on
    notifyObserversTrackAdded( track, position );
}

void
SyncedPlaylist::trackRemoved( Playlists::PlaylistPtr playlist, int position )
{
    if( !m_playlists.contains( playlist ) )
        return;

    if( playlist != m_playlists.first() )
        return; //we only apply changes to the master playlist to the others

    //update the others
    foreach( Playlists::PlaylistPtr playlistToUpdate, m_playlists )
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
    m_playlists << playlist;
}

bool
SyncedPlaylist::syncNeeded() const
{
    if( isEmpty() || m_playlists.count() == 1 )
        return false;

    /*use the first playlist as the base, if the others have a difference comapared to it
    a sync is needed */
    QListIterator<Playlists::PlaylistPtr> i( m_playlists );
    Playlists::PlaylistPtr master = i.next();
    int masterTrackCount = master->trackCount();
    while( i.hasNext() )
    {
        Playlists::PlaylistPtr slave = i.next();
        if( masterTrackCount != -1 )
        {
            int slaveTrackCount = slave->trackCount();
            //if the number of tracks is different a sync is certainly required
            if( slaveTrackCount != -1 && slaveTrackCount != masterTrackCount )
                return true;
        }
        //compare track by track
        TrackList masterTracks = master->tracks();
        TrackList slaveTracks = slave->tracks();
        for( int i = 0; i < masterTrackCount; i++ )
            if( masterTracks[i] != slaveTracks[i] )
                return true;
    }

    return false;
}

void
SyncedPlaylist::doSync() const
{
    DEBUG_BLOCK
    //HACK: only compare the first 2 for now
    QListIterator<Playlists::PlaylistPtr> i( m_playlists );
    Playlists::PlaylistPtr master = i.next();
    if( !i.hasNext() )
        return;

    Playlists::PlaylistPtr slave = i.next();
    QListIterator<TrackPtr> m( master->tracks() );
    //debug: print list
    int position = 0;
    debug() << "master playlist: " << master->name() << " " << master->uidUrl().url();
    while( m.hasNext() )
        debug() << QString( "%1 : %2" ).arg( position++ ).arg( m.next()->name() );
    m.toFront();

    TrackList slaveTracks = slave->tracks();
    //debug: print list
    position = 0;
    debug() << "slave playlist: " << slave->name() << " " << slave->uidUrl().url();
    foreach( const TrackPtr track, slaveTracks )
        debug() << QString( "%1 : %2" ).arg( position++ ).arg( track->name() );

    //Add first. Tracks in slave that are not in master will eventually shift to the end.
    position = 0;
    while( m.hasNext() )
    {
        TrackPtr track = m.next();
        if( position >= slaveTracks.size()
            || track->uidUrl() != slaveTracks.at( position )->uidUrl() )
        {
            debug() << "insert " << track->name() << " at " << position;
            slave->addTrack( track, position );
            //update local copy of the tracks
            slaveTracks = slave->tracks();
        }
        position++;
    }

    //debug: print list
    position = 0;
    debug() << "slave playlist after insertions:";
    foreach( const TrackPtr track, slaveTracks )
        debug() << QString( "%1 : %2" ).arg( position++ ).arg( track->name() );

    //Then remove everything after the position of the last track in master.
    //This removes any tracks that are not in master.
    position = master->tracks().size();
    while( position < slave->tracks().size() )
        slave->removeTrack( position );

    //debug: print list
    position = 0;
    debug() << "slave playlist after removal:";
    foreach( const TrackPtr track, slave->tracks() )
        debug() << QString( "%1 : %2" ).arg( position++ ).arg( track->name() );
}

void
SyncedPlaylist::removePlaylistsFrom( Playlists::PlaylistProvider *provider )
{
    foreach( Playlists::PlaylistPtr playlist, m_playlists )
    {
        if( playlist->provider() == provider )
        {
            unsubscribeFrom( playlist );
            m_playlists.removeAll( playlist );
        }
    }
}
