/* 
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include "memoryquerymaker.h"

#include <threadweaver/Job.h>
#include <threadweaver/ThreadWeaver.h>

class QueryJob : public ThreadWeaver::Job
{
    public:
        QueryJob( MemoryQueryMaker *qm )
            : ThreadWeaver::Job()
            , m_queryMaker( qm )
        {
            //nothing to do
        }

    protected:
        void run()
        {
            m_queryMaker->runQuery();
            setFinished( true );
        }

    private:
        MemoryQueryMaker *m_queryMaker;
};

struct MemoryQueryMaker::Private {
};

MemoryQueryMaker::MemoryQueryMaker( MemoryCollection *mc )
    : QueryMaker()
    , m_memCollection( mc )
    ,d( new Private )
{
    //nothing to do
}

MemoryQueryMaker::~MemoryQueryMaker()
{
    delete d;
}

QueryMaker*
MemoryQueryMaker::reset()
{
    return this;
}

void
MemoryQueryMaker::run()
{
}

void
MemoryQueryMaker::abortQuery()
{
}

void
MemoryQueryMaker::runQuery()
{
    m_memCollection->acquireReadLock();
    m_memCollection->releaseLock();
}
