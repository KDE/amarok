/****************************************************************************************
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

#include "IpodPlaylist.h"

#include "IpodCollection.h"
#include "IpodMeta.h"
#include "IpodPlaylistProvider.h"
#include "core/playlists/PlaylistProvider.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/MemoryMeta.h"
#include "core-impl/playlists/providers/user/UserPlaylistProvider.h"

#include <gpod/itdb.h>


IpodPlaylist::IpodPlaylist( Itdb_Playlist *ipodPlaylist, IpodCollection *collection )
    : m_playlist( ipodPlaylist )
    , m_coll( collection )
    , m_type( Normal )
{
    Q_ASSERT( m_playlist && collection );
    for( GList *members = m_playlist->members; members; members = members->next )
    {
        Itdb_Track *itdbTrack = (Itdb_Track *) members->data;
        Q_ASSERT( itdbTrack );
        Meta::TrackPtr track = IpodMeta::Track::fromIpodTrack( itdbTrack );
        Q_ASSERT( track );
        track = collection->trackForUidUrl( track->uidUrl() ); // get MemoryMeta proxy track
        Q_ASSERT( track );
        m_tracks << track;
    }
}

IpodPlaylist::IpodPlaylist( const Meta::TrackList &tracks, const QString &name,
                            IpodCollection *collection, Type type )
    : m_coll( collection )
    , m_type( type )
{
    m_playlist = itdb_playlist_new( name.toUtf8().constData(), false /* Smart playlist */ );
    Q_ASSERT( m_playlist );

    if( m_type != Normal )
    {
        m_tracks = tracks;
        return; // m_playlist holds just the name in this case
    }

    int position = 0;
    int finalPosition = 0;
    for( Meta::TrackPtr track : tracks )
    {
        if( track->collection() == collection ) // track from associated collection
        {
            addIpodTrack( track, position );
            position++;
        }
        else
            m_tracksToCopy << TrackPosition( track, finalPosition );
        finalPosition++;  // yes increment every time, tracks are inserted in order so this is correct
    }

    if( !m_tracksToCopy.isEmpty() )
        scheduleCopyAndInsert();
}

IpodPlaylist::~IpodPlaylist()
{
    itdb_playlist_free( m_playlist );
}

QUrl
IpodPlaylist::uidUrl() const
{
    // integer reading is atomic, no lock needed
    QString collId = m_coll ? m_coll->collectionId() : QStringLiteral("removedipodcollection:/");
    return QUrl( QStringLiteral( "%1/playlists/%2" ).arg( collId ).arg( m_playlist->id ) );
}

QString
IpodPlaylist::name() const
{
    QReadLocker locker( &m_playlistLock );
    return QString::fromUtf8( m_playlist->name );
}

void
IpodPlaylist::setName( const QString &name )
{
    QWriteLocker locker( &m_playlistLock );
    g_free( m_playlist->name );
    m_playlist->name = g_strdup( name.toUtf8().constData() );
}

Playlists::PlaylistProvider*
IpodPlaylist::provider() const
{
    return m_coll ? m_coll->playlistProvider() : nullptr;
}

int
IpodPlaylist::trackCount() const
{
    return m_tracks.count();
}

Meta::TrackList
IpodPlaylist::tracks()
{
    return m_tracks;
}

void
IpodPlaylist::addTrack(const Meta::TrackPtr &track, int position )
{
    if( m_type != Normal || !m_coll || !m_coll->isWritable() )
        return;

    if( position < 0 || position > m_tracks.count() )
        position = m_tracks.count();

    if( track->collection() == m_coll.data() ) // track from associated collection
        addIpodTrack( track, position );
    else
    {
        m_tracksToCopy << TrackPosition( track, position );
        scheduleCopyAndInsert();
    }
}

void
IpodPlaylist::removeTrack( int position )
{
    // we should fail only if position is incorrect, prevent infinite loops in
    // IpodPlaylistProvider::removeTrackFromPlaylists()
    if( position < 0 || position >= m_tracks.count() )
        return;

    Meta::TrackPtr removedTrack = m_tracks.takeAt( position );
    if( m_type == Stale || m_type == Orphaned )
    {
        notifyObserversTrackRemoved( position );
        return; // do not fire following machinery for special playlists
    }

    AmarokSharedPointer<MemoryMeta::Track> proxyTrack = AmarokSharedPointer<MemoryMeta::Track>::dynamicCast( removedTrack );
    if( !proxyTrack )
    {
        error() << __PRETTY_FUNCTION__ << "track" << removedTrack.data() << "from m_track was not MemoryMeta track!";
        return;
    }

    AmarokSharedPointer<IpodMeta::Track> ipodTrack = AmarokSharedPointer<IpodMeta::Track>::dynamicCast( proxyTrack->originalTrack() );
    if( !proxyTrack )
    {
        error() << __PRETTY_FUNCTION__ << "originalTrack of the proxyTrack was not IpodMeta track!";
        return;
    }

    {
        // notify observers _without_ the lock held
        QWriteLocker locker( &m_playlistLock );
        itdb_playlist_remove_track( m_playlist, ipodTrack->itdbTrack() );
    }
    notifyObserversTrackRemoved( position );
}

Itdb_Playlist*
IpodPlaylist::itdbPlaylist()
{
    return m_playlist;
}

TrackPositionList
IpodPlaylist::takeTracksToCopy()
{
    TrackPositionList tracksToCopy = m_tracksToCopy;
    m_tracksToCopy.clear();
    return tracksToCopy;
}

void
IpodPlaylist::scheduleCopyAndInsert()
{
    Playlists::PlaylistProvider *prov = provider();
    if( !prov )
        return;  // we can do nothing
    static_cast<IpodPlaylistProvider *>( prov )->scheduleCopyAndInsertToPlaylist(
        AmarokSharedPointer<IpodPlaylist>( this ) );
}

void
IpodPlaylist::addIpodTrack( Meta::TrackPtr track, int position )
{
    Q_ASSERT( position >= 0 && position <= m_tracks.count() );

    Meta::TrackPtr proxyTrack = Meta::TrackPtr();
    AmarokSharedPointer<MemoryMeta::Track> memoryTrack = AmarokSharedPointer<MemoryMeta::Track>::dynamicCast( track );
    if( memoryTrack )
    {
        track = memoryTrack->originalTrack();  // iPod track is usually hidden below MemoryMeta proxy
        proxyTrack = track;
    }
    AmarokSharedPointer<IpodMeta::Track> ipodTrack = AmarokSharedPointer<IpodMeta::Track>::dynamicCast( track );
    if( !ipodTrack )
    {
        error() << __PRETTY_FUNCTION__ << "Could not get IpodMeta::Track out of supplied track."
                << ( memoryTrack ? "(but cast to MemoryMeta::Track succeeded)"
                                 : "(cast to MemoryMeta::Track failed too)" );
        return;
    }

    if( !proxyTrack)  // we got IpodTrack directly, expose its MemoryMeta proxy
        proxyTrack = m_coll ? m_coll->trackForUidUrl( ipodTrack->uidUrl() ) : Meta::TrackPtr();
    if( !proxyTrack )
    {
        error() << __PRETTY_FUNCTION__ << "was passed IpodMeta::Track but we could not find"
                << "MemoryMeta::Track proxy for it.";
        return;
    }

    Itdb_Track *itdbTrack = ipodTrack->itdbTrack();
    /* There is following code in libgpod's itdb_playlist_add_track():
     *     g_return_if_fail (pl->itdb);
     *     track->itdb = pl->itdb;
     * Just fool libgpod by setting itdb to assumed value
     */
    Itdb_iTunesDB *save = m_playlist->itdb;
    m_playlist->itdb = itdbTrack->itdb;
    itdb_playlist_add_track( m_playlist, itdbTrack, -1 );
    m_playlist->itdb = save;

    m_tracks.insert( position, proxyTrack );
    notifyObserversTrackAdded( proxyTrack, position );
}
