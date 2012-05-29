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

#include "core/interfaces/Logger.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "statsyncing/Process.h"
#include "statsyncing/collection/CollectionProvider.h"

using namespace StatSyncing;

Controller::Controller( QObject* parent )
    : QObject( parent )
{
}

Controller::~Controller()
{
}


void
Controller::synchronize()
{
    if( m_currentProcess )
    {
        m_currentProcess.data()->raise();
        return;
    }

    QList<QSharedPointer<Provider> > providers;
    CollectionManager *manager = CollectionManager::instance();
    QHash<Collections::Collection *, CollectionManager::CollectionStatus> collHash =
        manager->collections();
    QHashIterator<Collections::Collection *, CollectionManager::CollectionStatus> it( collHash );
    while( it.hasNext() )
    {
        it.next();
        if( it.value() == CollectionManager::CollectionEnabled )
        {
            QSharedPointer<Provider> provider( new CollectionProvider( it.key() ) );
            providers.append( provider );
        }
    }

    if( providers.count() <= 1 )
    {
        // the text intentionally doesn't cope with 0 collections
        QString text = i18n( "You only seem to have one collection. statistics "
            "synchronization only makes sense if there is more than one collection." );
        Amarok::Components::logger()->longMessage( text );
        return;
    }

    m_currentProcess = new Process( providers, this );
    m_currentProcess.data()->start();
}
