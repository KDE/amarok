/****************************************************************************************
 * Copyright (c) 2008 Daniel Winter <dw@danielwinter.de>                                *
 * Copyright (c) 2012 Phalgun Guduthur <me@phalgun.in>                                  *
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

#define DEBUG_PREFIX "NepomukCollection"

#include "NepomukCollection.h"
#include "NepomukConstructMetaJob.h"

#include "core/collections/QueryMaker.h"
#include "core-impl/collections/support/MemoryCollection.h"
#include "core-impl/collections/support/MemoryMeta.h"
#include "core-impl/collections/support/MemoryQueryMaker.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaKeys.h"
#include "core/support/Debug.h"

#include <Nepomuk/Resource>
#include <Nepomuk/ResourceManager>

#include <KIcon>
#include <ThreadWeaver/Weaver>

using namespace MemoryMeta;
using namespace Collections;

NepomukCollection::NepomukCollection()
    : Collection()
    , m_mc( new Collections::MemoryCollection() )
{
    // check if Nepomuk is available, if yes, initialize and build up collection.
    if( Nepomuk::ResourceManager::instance()->initialized() )
    {
        m_nepomukCollectionReady = true;
        buildCollection();
    }

    else
    {
        m_nepomukCollectionReady = false;
        warning() << "Couldn't initialize Nepomuk Collection. Check status of Nepomuk. Nepomuk Plugin won't be loaded";
    }

    emit collectionUpdated();
}

NepomukCollection::~NepomukCollection()
{
}

Collections::QueryMaker*
NepomukCollection::queryMaker()
{
    return new MemoryQueryMaker( m_mc.toWeakRef(), collectionId() );
}

QString
NepomukCollection::uidUrlProtocol() const
{
    static const QString uid( "amarok-nepomuk" );
    return uid;
}

QString
NepomukCollection::collectionId() const
{
    return QString( "%1://" ).arg( uidUrlProtocol() );
}

QString
NepomukCollection::prettyName() const
{
    return QString( "Nepomuk Collection" );
}

KIcon
NepomukCollection::icon() const
{
    return KIcon( "nepomuk" );
}

bool
NepomukCollection::isWritable() const
{
    // Nepomuk if initialized is always writable
    // A check for nepomuk initialized will suffice
    //return m_nepomukCollectionReady;
    return true;
}

void
NepomukCollection::buildCollection()
{
    NepomukConstructMetaJob *job = new NepomukConstructMetaJob( this );
    m_constructMetaJob = job;
    connect( job, SIGNAL( done( ThreadWeaver::Job* ) ), job, SLOT( deleteLater() ) );
    ThreadWeaver::Weaver::instance()->enqueue( job );
}
