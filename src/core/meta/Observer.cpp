/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2013 Matěj Laitl <matej@laitl.cz>                                      *
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
 ***************************************************************************************/

#include "Observer.h"

#include "core/meta/Base.h"

using namespace Meta;

Observer::~Observer()
{
    // Unsubscribe all stray Meta subscriptions:
    for( Base *ptr : m_subscriptions )
    {
        if( ptr )
            ptr->unsubscribe( this );
    }
}

void
Observer::metadataChanged( const TrackPtr &track )
{
    Q_UNUSED( track );
}

void
Observer::metadataChanged( const ArtistPtr &artist )
{
    Q_UNUSED( artist );
}

void
Observer::metadataChanged( const AlbumPtr &album )
{
    Q_UNUSED( album );
}

void
Observer::metadataChanged( const ComposerPtr &composer )
{
    Q_UNUSED( composer );
}

void
Observer::metadataChanged( const GenrePtr &genre )
{
    Q_UNUSED( genre );
}

void
Observer::metadataChanged( const YearPtr &year )
{
    Q_UNUSED( year );
}

void
Observer::entityDestroyed()
{
}

void
Observer::subscribeTo( Base *ptr )
{
    if( !ptr )
        return;
    QMutexLocker locker( &m_subscriptionsMutex );
    ptr->subscribe( this );
    m_subscriptions.insert( ptr );
}

void
Observer::unsubscribeFrom( Base *ptr )
{
    QMutexLocker locker( &m_subscriptionsMutex );
    if( ptr )
        ptr->unsubscribe( this );
    m_subscriptions.remove( ptr );
}

void
Observer::destroyedNotify( Base *ptr )
{
    {
        QMutexLocker locker( &m_subscriptionsMutex );
        m_subscriptions.remove( ptr );
    }
    entityDestroyed();
}
