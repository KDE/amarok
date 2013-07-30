/****************************************************************************************
 * Copyright (c) 2013 Anmol Ahuja <darthcodus@gmail.com>                                *
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

#include "CollectionTypeExporter.h"

#include "core/meta/Meta.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core/collections/Collection.h"
#include "core/collections/CollectionLocation.h"
#include "core/capabilities/TranscodeCapability.h"
#include "core/transcoding/TranscodingController.h"
#include "scriptengine/ScriptingDefines.h"

#include <QIcon>
#include <QScriptEngine>
#include <QScriptValue>

using namespace AmarokScript;

using Collections::Collection;
using Collections::QueryMaker;

#define CHECK_COLLECTION( returnVal ) if( !m_collection ) return returnVal;

void
CollectionPrototype::init( QScriptEngine *engine )
{
    qScriptRegisterMetaType<Collection*>( engine, toScriptValue, fromScriptValue );
    qScriptRegisterMetaType<Collections::CollectionList>( engine, toScriptArray, fromScriptArray );
}

QScriptValue
CollectionPrototype::toScriptValue( QScriptEngine *engine, Collection* const &collection )
{
    CollectionPrototype *collectionProto = new CollectionPrototype( engine, collection );
    QScriptValue val = engine->newQObject( collectionProto, QScriptEngine::AutoOwnership);
    return val;
}

void
CollectionPrototype::fromScriptValue( const QScriptValue &obj, Collection* &collection )
{
    CollectionPrototype *collectionProto = dynamic_cast<CollectionPrototype*>( obj.toQObject() );
    if( !collectionProto )
        collection = 0;
    else
        collection = collectionProto->m_collection;
}

//script invokable

void
CollectionPrototype::copyTrack( const Meta::TrackPtr track, Collection* targetCollection, Configuration tc )
{
    copyTracks( Meta::TrackList() << track, targetCollection, tc );
}

void
CollectionPrototype::copyTracks( const Meta::TrackList &tracks, Collection* targetCollection, Configuration tc )
{
    CHECK_COLLECTION()
    if( !targetCollection )
        return;
    m_collection->location()->prepareTranscodeAndCopy( tracks, targetCollection->location(), tc );
}

void
CollectionPrototype::queryAndcopyTracks( QueryMaker *queryMaker, Collection* targetCollection, Configuration tc )
{
    Q_UNUSED( queryMaker )
    Q_UNUSED( targetCollection )
    Q_UNUSED( tc )
}

void
CollectionPrototype::moveTrack( const Meta::TrackPtr track, Collection *targetCollection, Configuration tc )
{
    moveTracks( Meta::TrackList() << track, targetCollection, tc );
}

void
CollectionPrototype::moveTracks( const Meta::TrackList &tracks, Collection *targetCollection, Configuration tc )
{
    if( !targetCollection )
        return;
    m_collection->location()->prepareTranscodeAndMove( tracks, targetCollection->location(), tc );
}

void
CollectionPrototype::queryAndmoveTracks( QueryMaker *queryMaker, Collection *targetCollection, Configuration tc )
{
    Q_UNUSED( queryMaker )
    Q_UNUSED( targetCollection )
    Q_UNUSED( tc )
}

void
CollectionPrototype::removeTracks( const Meta::TrackList &list )
{
    Q_UNUSED( list )
}

void CollectionPrototype::removeTrack( const Meta::TrackPtr track )
{
    removeTracks( Meta::TrackList() << track );
}

void
CollectionPrototype::queryAndRemoveTracks( QueryMaker* queryMaker )
{
    Q_UNUSED( queryMaker )
}

QueryMaker*
CollectionPrototype::queryMaker()
{
    CHECK_COLLECTION( 0 );
    return m_collection->queryMaker();
}

//private methods

QIcon
CollectionPrototype::icon() const
{
    CHECK_COLLECTION( QIcon() )
    return m_collection->icon();
}

QString
CollectionPrototype::collectionId() const
{
    CHECK_COLLECTION( QString() );
    return m_collection->collectionId();
}

bool
CollectionPrototype::isOrganizable() const
{
    CHECK_COLLECTION( false );
    return m_collection->isOrganizable();
}

bool
CollectionPrototype::isWritable() const
{
    CHECK_COLLECTION( false );
    return m_collection->isWritable();
}

QString
CollectionPrototype::prettyName() const
{
    CHECK_COLLECTION( QString() );
    return m_collection->prettyName();
}

float
CollectionPrototype::totalCapacity() const
{
    CHECK_COLLECTION( 0.0 )
    return m_collection->totalCapacity();
}

float
CollectionPrototype::usedCapacity() const
{
    CHECK_COLLECTION( 0.0 )
    return m_collection->usedCapacity();
}

bool
CollectionPrototype::isValid() const
{
    CHECK_COLLECTION( false )
    return true;
}

bool
CollectionPrototype::isQueryable()
{
    if( CollectionManager::instance()->collectionStatus( collectionId() ) &
        CollectionManager::CollectionQueryable )
        return true;
    return false;
}

bool
CollectionPrototype::isViewable()
{
    if( CollectionManager::instance()->collectionStatus( collectionId() ) &
        CollectionManager::CollectionViewable )
        return true;
    return false;
}

bool
CollectionPrototype::supportsTranscode()
{
    CHECK_COLLECTION( false )
    QScopedPointer<Capabilities::TranscodeCapability> tc(
        m_collection->create<Capabilities::TranscodeCapability>() );
    if( !tc )
        return false;
    Transcoding::Controller *tcC = Amarok::Components::transcodingController();
    if( tcC && !tcC->availableEncoders().isEmpty() )
        return true;
    return false;
}

CollectionPrototype::CollectionPrototype( QScriptEngine *engine, Collection *collection )
: QObject( engine )
, m_collection( collection )
{
    connect( collection, SIGNAL(updated()), SIGNAL(updated()) );
    connect( collection->location(), SIGNAL(aborted()), SIGNAL(aborted()) );
    connect( collection->location(), SIGNAL(finishCopy()), SIGNAL(finishCopy()) );
    connect( collection->location(), SIGNAL(finishRemove()), SIGNAL(finishRemove()) );
    //I kan haz constructorz?;
    connect( collection, SIGNAL(destroyed(QObject*)), SLOT(slotCollectionDestroyed()) );
}

void
CollectionPrototype::slotCollectionDestroyed()
{
    m_collection = 0;
}

#undef CHECK_COLLECTION
