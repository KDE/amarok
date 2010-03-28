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

#ifndef UNIONJOB_H
#define UNIONJOB_H

#include "SynchronizationBaseJob.h"

#include "core/meta/Meta.h"

#include <QMap>
#include <QPair>
#include <QSet>

namespace Amarok
{
    class Collection;
}

/**
 * @class UnionJob
 * As the result of this job both collections contain the union
 * of the tracks in both collections, i.e. it adds tracks that are in
 * collection A but not in collection B to B and tracks that are in B but
 * not in A to A. It does not remove tracks from either A or B.
 */
class UnionJob : public SynchronizationBaseJob
{
    Q_OBJECT
    public:
        UnionJob( Collections::Collection *collA, Collections::Collection *collB );
        virtual ~UnionJob();

    protected:
        void doSynchronization( const Meta::TrackList &tracks, InSet syncDirection, Collections::Collection *collA, Collections::Collection *collB );
};


#endif
