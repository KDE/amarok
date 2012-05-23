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

#include "Controller.h"

#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "statsyncing/collection/CollectionTrackDelegateProvider.h"
#include "statsyncing/jobs/MatchTracksJob.h"

#include <ThreadWeaver/Weaver>

using namespace StatSyncing;

Controller::Controller( QObject* parent )
    : QObject( parent )
{
}

Controller::~Controller()
{
    qDeleteAll( m_providers );
    m_providers.clear();
}

void
Controller::synchronize()
{
    DEBUG_BLOCK

    if( m_providers.isEmpty() ) // TODO: this is way too much naive
    {
        QHash<Collections::Collection *, CollectionManager::CollectionStatus> collHash =
            CollectionManager::instance()->collections();
        QHashIterator<Collections::Collection *, CollectionManager::CollectionStatus> it( collHash );
        while( it.hasNext() )
        {
            it.next();
            if( it.value() == CollectionManager::CollectionEnabled )
            {
                TrackDelegateProvider *provider = new CollectionTrackDelegateProvider( it.key() );
                debug() << "Adding provider" << provider->prettyName() << "with id" << provider->id();
                m_providers.append( provider );
            }
        }
    }

    MatchTracksJob *job = new MatchTracksJob( m_providers );
    connect( job, SIGNAL(done(ThreadWeaver::Job*)), job, SLOT(deleteLater()) );
    ThreadWeaver::Weaver::instance()->enqueue( job );
}
