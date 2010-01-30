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

#include "OneWaySynchronizationJob.h"

#include "collection/Collection.h"
#include "collection/CollectionLocation.h"
#include "Debug.h"
#include "meta/Meta.h"

OneWaySynchronizationJob::OneWaySynchronizationJob()
        : SynchronizationBaseJob()
        , m_source( 0 )
        , m_target( 0 )
{
}

OneWaySynchronizationJob::~OneWaySynchronizationJob()
{
    //nothing to do
}

void
OneWaySynchronizationJob::setSource( Amarok::Collection *source )
{
    m_source = source;
    setCollectionA( source );
}

void
OneWaySynchronizationJob::setTarget( Amarok::Collection *target )
{
    m_target = target;
    //this will be slightly inefficient as SynchronizationBaseJob will figure out
    //the tracks that are in target but not in source, even though we do not care about
    //those.
    setCollectionB( target );
}

void
OneWaySynchronizationJob::doSynchronization( const Meta::TrackList &tracks, InSet syncDirection, Amarok::Collection *collA, Amarok::Collection* collB )
{
    DEBUG_BLOCK
    if( !( syncDirection == OnlyInA || syncDirection == OnlyInB ) )
    {
        debug() << "warning, received an unexpected syncDirection";
        deleteLater();
        return;
    }
    if( !( m_source == collA || m_source == collB ) || !( m_target == collA || m_target == collB ) )
    {
        debug() << "warning, received an unknown collection";
        deleteLater();
        return;
    }
    if( ( syncDirection == OnlyInA && collA == m_source ) || ( syncDirection == OnlyInB && collB == m_source ) )
    {
        debug() << "Master " << m_source->collectionId() << " has to sync " << tracks.count() << " track(s) to " << m_target->collectionId();
        //show confirmation dialog, actually do stuff
        CollectionLocation *locSource = m_source->location();
        CollectionLocation *locTarget = m_target->location();
        if( !locTarget->isWritable() )
        {
            debug() << "target collection " << m_target->collectionId() << " is not writable, what am I doing here?";
            locSource->deleteLater();
            locTarget->deleteLater();
            deleteLater();
            return;
        }
        //might be nice to ask the user if the tracks should really be copied here...

        //although not named particularly well, this method actually starts the workflow too
        locSource->prepareCopy( tracks, locTarget );
        //the collection locations will take care of the remaining work flow, we are done
        deleteLater();
    }
}
