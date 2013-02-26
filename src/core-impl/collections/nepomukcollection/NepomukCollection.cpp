/****************************************************************************************
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
#include "core/meta/Meta.h"
#include "core/meta/support/MetaKeys.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/MemoryCollection.h"
#include "core-impl/collections/support/MemoryMeta.h"
#include "core-impl/collections/support/MemoryQueryMaker.h"

#include <Nepomuk2/Resource>
#include <Nepomuk2/ResourceManager>
#include <Nepomuk2/Vocabulary/NFO>

#include <KIcon>
#include <ThreadWeaver/Weaver>

using namespace MemoryMeta;
using namespace Collections;

NepomukCollection::NepomukCollection()
    : Collection()
    , m_mc( new MemoryCollection() )
{
    buildCollection();
}

NepomukCollection::~NepomukCollection()
{
}

QueryMaker*
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
    return i18n( "Nepomuk Collection" );
}

KIcon
NepomukCollection::icon() const
{
    return KIcon( "nepomuk" );
}

bool
NepomukCollection::isWritable() const
{
    // right now NepomukCollectionLocation isn't implemented, which deals with moving
    // and copying tracks into collection. So false until that is implemented.

    return false;
}

void
NepomukCollection::metadataChanged( Meta::TrackPtr track )
{
    // reflect change to ouside world:
    bool mapsChanged = MapChanger( m_mc.data() ).trackChanged( track );
    if( mapsChanged )
        // while docs say somehting different,
        // collection browser doesn't update unless we emit updated()
        emit updated();
}

bool
NepomukCollection::possiblyContainsTrack( const KUrl &url ) const
{
    // if a resource of type audio is created successfully return true
    Nepomuk2::Resource res( url );
    if( res.exists() && res.hasType( Nepomuk2::Vocabulary::NFO::Audio() ) )
        return true;
    else
        return false;
}

Meta::TrackPtr
NepomukCollection::trackForUrl( const KUrl &url )
{
    Nepomuk2::Resource fileRes( url );
    QString uidUrl = fileRes.uri().toString();
    return trackForUidUrl( uidUrl );
}

Meta::TrackPtr
NepomukCollection::trackForUidUrl( const QString &uidUrl )
{
    m_mc->acquireReadLock();
    Meta::TrackPtr ret = m_mc->trackMap().value( uidUrl, Meta::TrackPtr() );
    m_mc->releaseLock();
    return ret;
}

void
NepomukCollection::buildCollection()
{
    NepomukConstructMetaJob *job = new NepomukConstructMetaJob( this );
    connect( job, SIGNAL( done( ThreadWeaver::Job* ) ), job, SLOT( deleteLater() ) );
    connect( job, SIGNAL( updated() ), this , SIGNAL( updated() ) );
    ThreadWeaver::Weaver::instance()->enqueue( job );
}
