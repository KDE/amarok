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

#ifndef ONEWAYSYNCHRONIZATIONJOB_H
#define ONEWAYSYNCHRONIZATIONJOB_H

#include "SynchronizationBaseJob.h"

namespace Amarok
{
    class Collection;
}
/**
 * @class OneWaySynchronizationJob
 * Ensures that all tracks that are in the
 * source collection are present in the target collection as well, i.e.
 * it adds tracks that are in source but not in target to target. It does
 * not remove any tracks from target. It is one half of UnionJob.
*/
class OneWaySynchronizationJob : public SynchronizationBaseJob
{
    Q_OBJECT
public:
    OneWaySynchronizationJob();
    ~OneWaySynchronizationJob();

    //source/target are not settable in the ctor
    //to make explicit which collection is the source and which is the target
    //for this synchronization
    void setSource( Amarok::Collection *source );
    void setTarget( Amarok::Collection *target );

protected:
    void doSynchronization( const Meta::TrackList &tracks, InSet syncDirection, Amarok::Collection *collA, Amarok::Collection *collB );

private:
    Amarok::Collection *m_source;
    Amarok::Collection *m_target;
};

#endif // ONEWAYSYNCHRONIZATIONJOB_H
