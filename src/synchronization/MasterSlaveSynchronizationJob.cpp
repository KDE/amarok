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

#include "MasterSlaveSynchronizationJob.h"

#include "core/collections/Collection.h"
#include "core/collections/CollectionLocation.h"
#include "core/support/Debug.h"

MasterSlaveSynchronizationJob::MasterSlaveSynchronizationJob()
        : SynchronizationBaseJob()
        , m_master( 0 )
        , m_slave( 0 )
{

}

MasterSlaveSynchronizationJob::~MasterSlaveSynchronizationJob()
{
}

void
MasterSlaveSynchronizationJob::setMaster( Collections::Collection *master )
{
    m_master = master;
    setCollectionA( master );
}

void
MasterSlaveSynchronizationJob::setSlave( Collections::Collection *slave )
{
    m_slave = slave;
    setCollectionB( slave );
}

void
MasterSlaveSynchronizationJob::doSynchronization( const Meta::TrackList &tracks, InSet syncDirection, Collections::Collection *collA, Collections::Collection *collB )
{
    DEBUG_BLOCK
    if( !( syncDirection == OnlyInA || syncDirection == OnlyInB ) )
    {
        debug() << "warning, received an unexpected syncDirection";
        return;
    }
    if( !( m_master == collA || m_master == collB ) || !( m_slave == collA || m_slave == collB ) )
    {
        debug() << "warning, received an unknown collection";
        return;
    }
    if( !m_slave->isWritable() )
    {
        debug() << "Error: slave collection " << m_slave->collectionId() << " is not writable";
        return;
    }
    if( ( syncDirection == OnlyInA && collA == m_master ) || ( syncDirection == OnlyInB && collB == m_master ) )
    {
        debug() << "Master " << m_master->collectionId() << " has to sync " << tracks.count() << " track(s) to " << m_slave->collectionId();
        CollectionLocation *masterLoc = m_master->location();
        CollectionLocation *slaveLoc = m_slave->location();
        masterLoc->prepareCopy( tracks, slaveLoc );
    }
    else
    {
        //we have tested for the correct synDirections and the correct collections above,
        //so these are definitely the tracks that have to be removed from the slave
        debug() << "Delete " << tracks.count() << " track(s) from slave " << m_slave->collectionId();
        //do some more stuff, and *really* show a confirmation dialog
        CollectionLocation *slaveLoc = m_slave->location();
        slaveLoc->prepareRemove( tracks );
    }
}
