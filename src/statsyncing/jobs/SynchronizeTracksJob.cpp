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

#include "SynchronizeTracksJob.h"

#include "core/meta/Meta.h"
#include "core/meta/Statistics.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "statsyncing/Controller.h"
#include "statsyncing/TrackTuple.h"

#include <ThreadWeaver/Thread>

using namespace StatSyncing;

static const int denom = 20; // Q_EMIT incementProgress() signal each N tracks
static const int fuzz = denom / 2;

SynchronizeTracksJob::SynchronizeTracksJob( const QList<TrackTuple> &tuples,
                                            const TrackList &tracksToScrobble,
                                            const Options &options, QObject *parent )
    : QObject( parent )
    , ThreadWeaver::Job()
    , m_abort( false )
    , m_tuples( tuples )
    , m_tracksToScrobble( tracksToScrobble )
    , m_updatedTracksCount( 0 )
    , m_options( options )
{
}

void
SynchronizeTracksJob::abort()
{
    m_abort = true;
}

void
SynchronizeTracksJob::run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread)
{
    Q_UNUSED(self);
    Q_UNUSED(thread);
    Q_EMIT totalSteps( ( m_tuples.size() + fuzz ) / denom );

    Controller *controller = Amarok::Components::statSyncingController();
    if( controller )
    {
        connect( this, &SynchronizeTracksJob::scrobble,
                 controller, &StatSyncing::Controller::scrobble );
        // we don't run an event loop, we must use direct connection for controller to talk to us
        connect( controller, &StatSyncing::Controller::trackScrobbled,
                 this, &SynchronizeTracksJob::slotTrackScrobbled, Qt::DirectConnection );
        connect( controller, &StatSyncing::Controller::scrobbleFailed,
                 this, &SynchronizeTracksJob::slotScrobbleFailed, Qt::DirectConnection );
    }
    else
        warning() << __PRETTY_FUNCTION__ << "StatSyncing::Controller not available!";

    // first, queue tracks for scrobbling, because after syncing their recent playcount is
    // reset
    for( const TrackPtr &track : m_tracksToScrobble )
    {
        Meta::TrackPtr metaTrack = track->metaTrack();
        int playcount = track->recentPlayCount();
        if( metaTrack && playcount > 0 )
        {
            m_scrobbledTracks << metaTrack;
            Q_EMIT scrobble( metaTrack, playcount, track->lastPlayed() );
        }
    }

    ProviderPtrSet updatedProviders;
    int i = 0;
    for( const TrackTuple &tuple : m_tuples )
    {
        if( m_abort )
            break;

        // no point in checking for hasUpdate() here, synchronize() is witty enough
        const ProviderPtrSet tupleUpdatedProviders = tuple.synchronize( m_options );
        updatedProviders |= tupleUpdatedProviders;
        m_updatedTracksCount += tupleUpdatedProviders.count();
        if( ( i + fuzz ) % denom == 0 )
            Q_EMIT incrementProgress();
        i++;
    }

    for( ProviderPtr provider : updatedProviders )
        provider->commitTracks();


    // we need to reset playCount of scrobbled tracks to reset their recent play count
    for( Meta::TrackPtr track : m_scrobbledTracks )
    {
        Meta::StatisticsPtr statistics = track->statistics();
        statistics->setPlayCount( statistics->playCount() );
    }

    if( !m_tracksToScrobble.isEmpty() )
        // wait 3 seconds so that we have chance to catch slotTrackScrobbled()..
        QObject::thread()->msleep( 3000 );
    if( controller )
        disconnect( controller, &StatSyncing::Controller::trackScrobbled, this, nullptr );
    disconnect( controller, &StatSyncing::Controller::scrobbleFailed, this, nullptr );

    Q_EMIT endProgressOperation( this );
}

void SynchronizeTracksJob::defaultBegin(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread)
{
    Q_EMIT started(self);
    ThreadWeaver::Job::defaultBegin(self, thread);
}

void SynchronizeTracksJob::defaultEnd(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread)
{
    ThreadWeaver::Job::defaultEnd(self, thread);
    if (!self->success()) {
        Q_EMIT failed(self);
    }
    Q_EMIT done(self);
}

void
SynchronizeTracksJob::slotTrackScrobbled( const ScrobblingServicePtr &service,
                                          const Meta::TrackPtr &track )
{
    slotScrobbleFailed( service, track, ScrobblingService::NoError );
}

void
SynchronizeTracksJob::slotScrobbleFailed( const ScrobblingServicePtr &service,
                                          const Meta::TrackPtr &track, int error )
{
    // only count tracks scrobbled by us. Still chance for false-positives, though
    if( m_scrobbledTracks.contains( track ) )
    {
        ScrobblingService::ScrobbleError errorEnum = ScrobblingService::ScrobbleError( error );
        m_scrobbles[ service ][ errorEnum ]++;
    }
}

int
SynchronizeTracksJob::updatedTracksCount() const
{
    return m_updatedTracksCount;
}

QMap<ScrobblingServicePtr, QMap<ScrobblingService::ScrobbleError, int> >
SynchronizeTracksJob::scrobbles()
{
    return m_scrobbles;
}
