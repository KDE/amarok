/****************************************************************************************
 * Copyright (c) 2012 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include "MetaProxyWorker.h"

#include "core/meta/Meta.h"
#include "core-impl/collections/support/CollectionManager.h"

using namespace MetaProxy;

Worker::Worker( const QUrl &url, Collections::TrackProvider *provider )
    : QObject()
    , ThreadWeaver::Job()
    , m_url( url )
    , m_provider( provider )
    , m_stepsDoneReceived( 0 )
{
}

void
Worker::run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread)
{
    Q_UNUSED(self);
    Q_UNUSED(thread);
    Meta::TrackPtr track;

    if( m_provider )
    {
        track = m_provider->trackForUrl( m_url );
        Q_EMIT finishedLookup( track );
        return;
    }

    track = CollectionManager::instance()->trackForUrl( m_url );
    if( track )
    {
        Q_EMIT finishedLookup( track );
        return;
    }

    // no TrackProvider has a track for us yet, query new ones that are added.
    if( !track )
    {
        connect( CollectionManager::instance(), &CollectionManager::trackProviderAdded,
                 this, &Worker::slotNewTrackProvider, Qt::DirectConnection ); // we may live in a thread w/out event loop
        connect( CollectionManager::instance(),&CollectionManager::collectionAdded,
                 this, &Worker::slotNewCollection, Qt::DirectConnection ); // we may live in a thread w/out event loop
        return;
    }

}

void
Worker::defaultBegin(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread)
{
    Q_EMIT started(self);
    ThreadWeaver::Job::defaultBegin(self, thread);
}

void
Worker::defaultEnd(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread)
{
    ThreadWeaver::Job::defaultEnd(self, thread);
    if (!self->success()) {
        Q_EMIT failed(self);
    }
    Q_EMIT done(self);
}

void
Worker::slotNewTrackProvider( Collections::TrackProvider *newTrackProvider )
{
    if( !newTrackProvider )
        return;

    if( newTrackProvider->possiblyContainsTrack( m_url ) )
    {
        Meta::TrackPtr track = newTrackProvider->trackForUrl( m_url );
        Q_EMIT finishedLookup( track );
    }
}

void
Worker::slotNewCollection( Collections::Collection *newCollection )
{
    slotNewTrackProvider( newCollection );
}
