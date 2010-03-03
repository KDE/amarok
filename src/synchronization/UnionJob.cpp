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

#include "UnionJob.h"

#include "collection/Collection.h"
#include "collection/CollectionLocation.h"
#include "Debug.h"

UnionJob::UnionJob( Amarok::Collection *collA, Amarok::Collection *collB )
        : SynchronizationBaseJob()
{
    DEBUG_BLOCK
    setCollectionA( collA );
    setCollectionB( collB );
}

UnionJob::~UnionJob()
{
    //nothing to do
}

void
UnionJob::doSynchronization( const Meta::TrackList &tracks, InSet syncDirection, Amarok::Collection *collA, Amarok::Collection *collB )
{
    DEBUG_BLOCK
    if( !( syncDirection == OnlyInA || syncDirection == OnlyInB ) )
    {
        debug() << "warning, received an unexpected syncDirection";
        return;
    }
    Amarok::Collection *from = ( syncDirection == OnlyInA ? collA : collB );
    Amarok::Collection *to = ( syncDirection == OnlyInA ? collB : collA );

    debug() << "Collection " << from->collectionId() << " has to sync " << tracks.count() << " track(s) to " << to->collectionId();
    //show confirmation dialog, actually do stuff
    CollectionLocation *fromLoc = from->location();
    CollectionLocation *toLoc = to->location();

    if( !toLoc->isWritable() )
    {
        debug() << "Collection " << to->collectionId() << " is not writable";
        fromLoc->deleteLater();
        toLoc->deleteLater();
    }
    else
    {
        fromLoc->prepareCopy( tracks, toLoc );
    }
}
