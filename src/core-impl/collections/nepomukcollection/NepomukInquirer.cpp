/****************************************************************************************
 * Copyright (c) 2013 Edward Toroshchin <amarok@hades.name>
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

#include "NepomukInquirer.h"

#include "NepomukParser.h"

#include "core/support/Debug.h"

#include <Nepomuk2/ResourceManager>
#include <Soprano/Model>

namespace Collections {


NepomukInquirer::NepomukInquirer( QString query, std::auto_ptr<NepomukParser> parser )
    : m_query( query )
{
    Q_ASSERT( parser.get() );

    m_parser = parser.release();
    m_parser->setParent( this );
}

NepomukInquirer::~NepomukInquirer()
{
}

void
NepomukInquirer::run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread)
{
    Q_UNUSED(self);
    Q_UNUSED(thread);
    DEBUG_BLOCK

    Soprano::Model *model = Nepomuk2::ResourceManager::instance()->mainModel();
    Soprano::QueryResultIterator it = model->executeQuery( m_query, Soprano::Query::QueryLanguageSparql );

    if( !it.isValid() )
    {
        error() << "nepomuk query failed!";
        error() << "Soprano message:" << it.lastError().message();
        return;
    }

    m_parser->parse( it );
}

void
NepomukInquirer::defaultBegin(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread)
{
    Q_EMIT started(self);
    ThreadWeaver::Job::defaultBegin(self, thread);
}

void
NepomukInquirer::defaultEnd(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread)
{
    ThreadWeaver::Job::defaultEnd(self, thread);
    if (!self->success()) {
        Q_EMIT failed(self);
    }
    Q_EMIT done(self);
}

}
