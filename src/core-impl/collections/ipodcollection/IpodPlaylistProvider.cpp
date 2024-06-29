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

#include "IpodPlaylistProvider.h"

#include "IpodCollection.h"
#include "IpodCollectionLocation.h"
#include "IpodPlaylist.h"
#include "core/capabilities/ActionsCapability.h"
#include "core/logger/Logger.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/FileCollectionLocation.h"

#include <QDir>
#include <QFileInfo>

#include <gpod/itdb.h>

IpodPlaylistProvider::IpodPlaylistProvider( IpodCollection* collection )
    : UserPlaylistProvider( collection )
    , m_coll( collection )
{
}

IpodPlaylistProvider::~IpodPlaylistProvider()
{
}

QString
IpodPlaylistProvider::prettyName() const
{
    return m_coll->prettyName();
}

QIcon
IpodPlaylistProvider::icon() const
{
    return m_coll->icon();
}

int
IpodPlaylistProvider::playlistCount() const
{
    return m_playlists.count();
}

Playlists::PlaylistList
IpodPlaylistProvider::playlists()
{
    return m_playlists;
}

Playlists::PlaylistPtr
IpodPlaylistProvider::addPlaylist(Playlists::PlaylistPtr playlist )
{
    return save( playlist->tracks(), playlist->name() );
}

Meta::TrackPtr
IpodPlaylistProvider::addTrack(const Meta::TrackPtr &track )
{
    QString name = QLocale().toString( QDateTime::currentDateTime() );
    return save( Meta::TrackList() << track , name )->tracks().last();
}

Playlists::PlaylistPtr
IpodPlaylistProvider::save( const Meta::TrackList &tracks, const QString &name )
{
    if( !isWritable() )
        return Playlists::PlaylistPtr();

    IpodPlaylist *playlist = new IpodPlaylist( tracks, name, m_coll );
    itdb_playlist_add( m_coll->m_itdb, playlist->itdbPlaylist(), -1 );
    Playlists::PlaylistPtr playlistPtr( playlist );
    m_playlists << playlistPtr;
    subscribeTo( playlistPtr );
    Q_EMIT playlistAdded( playlistPtr );
    Q_EMIT startWriteDatabaseTimer();
    return playlistPtr;
}

QActionList
IpodPlaylistProvider::providerActions()
{
    QScopedPointer<Capabilities::ActionsCapability> ac( m_coll->create<Capabilities::ActionsCapability>() );
    return ac->actions();  // all IpodCollection actions are parented, no memory leak here
}

QActionList
IpodPlaylistProvider::playlistActions( const Playlists::PlaylistList &playlists )
{
    QActionList actions;
    for( const Playlists::PlaylistPtr &playlist : playlists )
    {
        if( !m_playlists.contains( playlist ) )  // make following static cast safe
            continue;
        IpodPlaylist::Type type = AmarokSharedPointer<IpodPlaylist>::staticCast( playlist )->type();
        if( type == IpodPlaylist::Stale || type == IpodPlaylist::Orphaned )
        {
            actions << m_coll->m_consolidateAction;
            break;
        }
    }

    return actions;
}

QActionList
IpodPlaylistProvider::trackActions( const QMultiHash<Playlists::PlaylistPtr, int> &playlistTracks )
{
    return playlistActions( playlistTracks.uniqueKeys() );
}

bool
IpodPlaylistProvider::isWritable()
{
    return m_coll->isWritable();
}

void
IpodPlaylistProvider::renamePlaylist( Playlists::PlaylistPtr playlist, const QString &newName )
{
    if( !m_playlists.contains( playlist ) )  // make following static cast safe
        return;
    AmarokSharedPointer<IpodPlaylist> ipodPlaylist = AmarokSharedPointer<IpodPlaylist>::staticCast( playlist );
    if( ipodPlaylist->type() != IpodPlaylist::Normal )
        return;  // special playlists cannot be renamed

    playlist->setName( newName );
    Q_EMIT updated();
    Q_EMIT startWriteDatabaseTimer();
}

bool
IpodPlaylistProvider::deletePlaylists( const Playlists::PlaylistList &playlistlist )
{
    if( !isWritable() )
        return false;

    for( const Playlists::PlaylistPtr &playlist : playlistlist )
    {
        if( !m_playlists.contains( playlist ) )
            continue;
        if( AmarokSharedPointer<IpodPlaylist>::staticCast( playlist )->type() != IpodPlaylist::Normal )
            continue;  // special playlists cannot be removed using this method
        m_playlists.removeOne( playlist );

        unsubscribeFrom( playlist );
        IpodPlaylist *ipodPlaylist = static_cast<IpodPlaylist *>( playlist.data() );
        itdb_playlist_unlink( ipodPlaylist->itdbPlaylist() );

        Q_EMIT playlistRemoved( playlist );
        Q_EMIT startWriteDatabaseTimer();
    }
    return true;
}

void
IpodPlaylistProvider::metadataChanged(const Playlists::PlaylistPtr & )
{
    Q_EMIT startWriteDatabaseTimer();
}

void
IpodPlaylistProvider::trackAdded(const Playlists::PlaylistPtr &, const Meta::TrackPtr &, int )
{
    Q_EMIT startWriteDatabaseTimer();
}

void
IpodPlaylistProvider::trackRemoved(const Playlists::PlaylistPtr &, int )
{
    Q_EMIT startWriteDatabaseTimer();
}

void
IpodPlaylistProvider::scheduleCopyAndInsertToPlaylist( AmarokSharedPointer<IpodPlaylist> playlist )
{
    m_copyTracksTo.insert( playlist );
    QTimer::singleShot( 0, this, &IpodPlaylistProvider::slotCopyAndInsertToPlaylists );
}

void
IpodPlaylistProvider::removeTrackFromPlaylists( Meta::TrackPtr track )
{
    for( Playlists::PlaylistPtr playlist : m_playlists )
    {
        int trackIndex;
        // track may be multiple times in a playlist:
        while( ( trackIndex = playlist->tracks().indexOf( track ) ) >= 0 )
            playlist->removeTrack( trackIndex );
    }
}

bool
IpodPlaylistProvider::hasStaleOrOrphaned() const
{
    return m_stalePlaylist || m_orphanedPlaylist;
}

void
IpodPlaylistProvider::slotConsolidateStaleOrphaned()
{
    int matched = 0, added = 0, removed = 0, failed = 0;

    /* Sometimes users accidentally rename files on iPod. This creates a pair of a stale
     * iTunes database entry and an orphaned file. Find these specifically and move the files
     * back to their original location. */
    if( m_stalePlaylist && m_orphanedPlaylist )
    {
        QMap<Meta::TrackPtr, Meta::TrackPtr> orphanedToStale;
        for( Meta::TrackPtr orphanedTrack : m_orphanedPlaylist->tracks() )
        {
            Meta::TrackPtr matchingStaleTrack;
            for( Meta::TrackPtr staleTrack : m_stalePlaylist->tracks() )
            {
                if( orphanedAndStaleTracksMatch( orphanedTrack, staleTrack ) )
                {
                    matchingStaleTrack = staleTrack;
                    break;
                }
            }

            if( matchingStaleTrack )  // matching track found
            {
                orphanedToStale.insert( orphanedTrack, matchingStaleTrack );
                m_stalePlaylist->removeTrack( m_stalePlaylist->tracks().indexOf( matchingStaleTrack ) );
            }
        }

        QMapIterator<Meta::TrackPtr, Meta::TrackPtr> it( orphanedToStale );
        while( it.hasNext() )
        {
            it.next();
            Meta::TrackPtr orphanedTrack = it.key();
            Meta::TrackPtr staleTrack = it.value();
            m_orphanedPlaylist->removeTrack( m_orphanedPlaylist->tracks().indexOf( orphanedTrack ) );

            QString from = orphanedTrack->playableUrl().toLocalFile();
            QString to = staleTrack->playableUrl().toLocalFile();
            if( !QFileInfo( to ).absoluteDir().mkpath( QStringLiteral(".") ) )
            {
                warning() << __PRETTY_FUNCTION__ << "Failed to create directory path"
                          << QFileInfo( to ).absoluteDir().path();
                failed++;
                continue;
            }
            if( !QFile::rename( from, to ) )
            {
                warning() << __PRETTY_FUNCTION__ << "Failed to move track from" << from
                          << "to" << to;
                failed++;
                continue;
            }
            matched++;
        }
    }

    // remove remaining stale tracks
    if( m_stalePlaylist && m_stalePlaylist->trackCount() )
    {
        Collections::CollectionLocation *location = m_coll->location();
        // hide removal confirmation - these tracks are already deleted, don't confuse user
        static_cast<IpodCollectionLocation *>( location )->setHidingRemoveConfirm( true );
        removed += m_stalePlaylist->trackCount();
        location->prepareRemove( m_stalePlaylist->tracks() );
        // remove all tracks from the playlist, assume the removal succeeded
        while( m_stalePlaylist->trackCount() )
            m_stalePlaylist->removeTrack( 0 );
    }

    // add remaining orphaned tracks back to database
    if( m_orphanedPlaylist && m_orphanedPlaylist->trackCount() )
    {
        Collections::CollectionLocation *src = new Collections::FileCollectionLocation();
        Collections::CollectionLocation *dest = m_coll->location();
        added += m_orphanedPlaylist->trackCount();
        src->prepareMove( m_orphanedPlaylist->tracks(), dest );
        // remove all tracks from the playlist, assume the move succeeded
        while( m_orphanedPlaylist->trackCount() )
            m_orphanedPlaylist->removeTrack( 0 );
    }

    // if some of the playlists became empty, remove them completely. no need to
    // unsubscribe - we were not subscribed
    if( m_stalePlaylist && m_stalePlaylist->trackCount() == 0 )
    {
        m_playlists.removeOne( m_stalePlaylist );
        Q_EMIT playlistRemoved( m_stalePlaylist );
        m_stalePlaylist = nullptr;
    }
    if( m_orphanedPlaylist && m_orphanedPlaylist->trackCount() == 0 )
    {
        m_playlists.removeOne( m_orphanedPlaylist );
        Q_EMIT playlistRemoved( m_orphanedPlaylist );
        m_orphanedPlaylist = nullptr;
    }

    QString failedText = failed ? i18np("Failed to process one track. (more info about "
        "it is in the Amarok debugging log)", "Failed to process %1 tracks. (more info "
        "about these is in the Amarok debugging log)", failed ) : QString();
    QString text = i18nc( "Infrequently displayed message, don't bother with singular "
        "forms. %1 to %3 are numbers, %4 is the 'Failed to process ...' sentence or an "
        "empty string.", "Done consolidating iPod files. %1 orphaned tracks matched with "
        "stale iTunes database entries, %2 stale database entries removed, %3 orphaned "
        "tracks added back to the iTunes database. %4", matched, removed, added,
        failedText );
    Amarok::Logger::longMessage( text );
}

void
IpodPlaylistProvider::slotCopyAndInsertToPlaylists()
{
    QMutableSetIterator< AmarokSharedPointer<IpodPlaylist> > it( m_copyTracksTo );
    while( it.hasNext() )
    {
        AmarokSharedPointer<IpodPlaylist> ipodPlaylist = it.next();
        TrackPositionList tracks = ipodPlaylist->takeTracksToCopy();
        copyAndInsertToPlaylist( tracks, Playlists::PlaylistPtr::staticCast( ipodPlaylist ) );
        it.remove();
    }
}

void IpodPlaylistProvider::copyAndInsertToPlaylist( const TrackPositionList &tracks, Playlists::PlaylistPtr destPlaylist )
{
    QMap<Collections::Collection*, TrackPositionList> sourceCollections;
    for( const TrackPosition &pair : tracks )
    {
        Collections::Collection *coll = pair.first->collection();
        if( coll == m_coll )
            continue;

        if( sourceCollections.contains( coll ) )
            sourceCollections[ coll ] << pair;
        else
            sourceCollections.insert( coll, TrackPositionList() << pair );
    }

    for( Collections::Collection *coll : sourceCollections.keys() )
    {
        Meta::TrackList sourceTracks;
        QMap<Meta::TrackPtr, int> trackPlaylistPositions;
        for( const TrackPosition &pair : sourceCollections.value( coll ) )
        {
            sourceTracks << pair.first;
            trackPlaylistPositions.insert( pair.first, pair.second );
        }

        Collections::CollectionLocation *sourceLocation = coll
            ? coll->location() : new Collections::FileCollectionLocation();
        Q_ASSERT( sourceLocation );
        // prepareCopy() takes ownership of the pointers, we must create target collection every time
        IpodCollectionLocation *targetLocation = static_cast<IpodCollectionLocation *>( m_coll->location() );

        targetLocation->setDestinationPlaylist( destPlaylist, trackPlaylistPositions );
        sourceLocation->prepareCopy( sourceTracks, targetLocation );
    }
}

bool
IpodPlaylistProvider::orphanedAndStaleTracksMatch( const Meta::TrackPtr &orphaned, const Meta::TrackPtr &stale )
{
    if( orphaned->filesize() != stale->filesize() )  // first for performance reasons
        return false;
    if( orphaned->length() != stale->length() )
        return false;
    if( orphaned->name() != stale->name() )
        return false;
    if( orphaned->type() != stale->type() )
        return false;
    if( orphaned->trackNumber() != stale->trackNumber() )
        return false;
    if( orphaned->discNumber() != stale->discNumber() )
        return false;

    if( entitiesDiffer( orphaned->album(), stale->album() ) )
        return false;
    if( entitiesDiffer( orphaned->artist(), stale->artist() ) )
        return false;
    if( entitiesDiffer( orphaned->composer(), stale->composer() ) )
        return false;
    if( entitiesDiffer( orphaned->genre(), stale->genre() ) )
        return false;
    if( entitiesDiffer( orphaned->year(), stale->year() ) )
        return false;

    return true;
}

template <class T> bool
IpodPlaylistProvider::entitiesDiffer( T first, T second )
{
    return ( first ? first->name() : QString() ) != ( second ? second->name() : QString() );
}

