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

#ifndef MASTERSLAVESYNCHRONIZATIONJOB_H
#define MASTERSLAVESYNCHRONIZATIONJOB_H

#include "SynchronizationBaseJob.h"

namespace Amarok
{
    class Collection;
}

class MasterSlaveSynchronizationJob : public SynchronizationBaseJob
{
    Q_OBJECT
    public:
        MasterSlaveSynchronizationJob();
        ~MasterSlaveSynchronizationJob();

        //master/slave are not settable in the ctor
        //to make explicit which collection is the master and which is the slave
        //for this synchronization
        void setMaster( Amarok::Collection *master );
        void setSlave( Amarok::Collection *slave );

    protected:
        void doSynchronization( const Meta::TrackList &tracks, InSet syncDirection, Amarok::Collection *collA, Amarok::Collection *collB );

    private:
        Amarok::Collection *m_master;
        Amarok::Collection *m_slave;
};

#endif
