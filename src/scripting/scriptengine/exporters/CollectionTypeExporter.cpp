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
#include "scripting/scriptengine/ScriptingDefines.h"

#include <QIcon>
#include <QJSEngine>
#include <QJSValue>

using namespace AmarokScript;

using Collections::Collection;
using Collections::QueryMaker;

#define GET_COLLECTION( returnVal ) Collection *collection = m_collection.data(); if( !collection ) return returnVal;

void
CollectionPrototype::init( QJSEngine *engine )
{
    qRegisterMetaType<Collection*>();
    QMetaType::registerConverter<Collection*, QJSValue>( [=] (Collection* collection) {
        return toScriptValue<Collection*, CollectionPrototype>( engine, collection );
    } );
    QMetaType::registerConverter<QJSValue, Collection*>( [] (QJSValue jsValue) {
        Collection* collection;
        fromScriptValue<Collection*, CollectionPrototype>( jsValue, collection );
        return collection;
    } );

    qRegisterMetaType<Collections::CollectionList>();
    QMetaType::registerConverter<Collections::CollectionList,QJSValue>( [=] (Collections::CollectionList collectionList) {
        return toScriptArray<Collections::CollectionList>( engine, collectionList);

    } );
    QMetaType::registerConverter<QJSValue,Collections::CollectionList>( [] (QJSValue jsValue) {
        Collections::CollectionList collectionList;
        fromScriptArray<Collections::CollectionList>( jsValue, collectionList );
        return collectionList;
    } );

}

//script invokable

void
CollectionPrototype::copyTracks( const Meta::TrackPtr &track, Collection* targetCollection )
{
    copyTracks( Meta::TrackList() << track, targetCollection );
}

void
CollectionPrototype::copyTracks( const Meta::TrackList &tracks, Collection* targetCollection )
{
    GET_COLLECTION()
    if( !targetCollection )
        return;
    collection->location()->prepareCopy( removeInvalidTracks( tracks ), targetCollection->location() );
}

void
CollectionPrototype::queryAndCopyTracks( QueryMaker *queryMaker, Collection* targetCollection )
{
    GET_COLLECTION()
    if( !queryMaker || !targetCollection )
        return;
    collection->location()->prepareCopy( queryMaker, targetCollection->location() );
}

void
CollectionPrototype::moveTracks( const Meta::TrackPtr &track, Collection *targetCollection )
{
    moveTracks( Meta::TrackList() << track, targetCollection );
}

void
CollectionPrototype::moveTracks( const Meta::TrackList &tracks, Collection *targetCollection )
{
    GET_COLLECTION()
    if( !targetCollection )
        return;
    collection->location()->prepareMove( removeInvalidTracks( tracks ), targetCollection->location() );
}

void
CollectionPrototype::queryAndMoveTracks( QueryMaker *queryMaker, Collection *targetCollection )
{
    GET_COLLECTION()
    if( !queryMaker || !targetCollection )
        return;
    collection->location()->prepareMove( queryMaker, targetCollection->location() );
}

void
CollectionPrototype::removeTracks( const Meta::TrackList &list )
{
    GET_COLLECTION()
    collection->location()->prepareRemove( removeInvalidTracks( list ) );
}

void CollectionPrototype::removeTracks( const Meta::TrackPtr &track )
{
    removeTracks( Meta::TrackList() << track );
}

void
CollectionPrototype::queryAndRemoveTracks( QueryMaker* queryMaker )
{
    GET_COLLECTION()
    if( !queryMaker )
        return;
    collection->location()->prepareRemove( queryMaker );
}

QueryMaker*
CollectionPrototype::queryMaker()
{
    GET_COLLECTION( nullptr );
    return collection->queryMaker();
}

//private methods

QIcon
CollectionPrototype::icon() const
{
    GET_COLLECTION( QIcon() )
    return collection->icon();
}

QString
CollectionPrototype::collectionId() const
{
    GET_COLLECTION( QString() );
    return collection->collectionId();
}

bool
CollectionPrototype::isOrganizable() const
{
    GET_COLLECTION( false );
    return collection->isOrganizable();
}

bool
CollectionPrototype::isWritable() const
{
    GET_COLLECTION( false );
    return collection->isWritable();
}

QString
CollectionPrototype::toString() const
{
    GET_COLLECTION( QString() );
    return collection->prettyName();
}

float
CollectionPrototype::totalCapacity() const
{
    GET_COLLECTION( 0.0 )
    return collection->totalCapacity();
}

float
CollectionPrototype::usedCapacity() const
{
    GET_COLLECTION( 0.0 )
    return collection->usedCapacity();
}

bool
CollectionPrototype::isValid() const
{
    GET_COLLECTION( false )
    return true;
}

bool
CollectionPrototype::isQueryable() const
{
    if( CollectionManager::instance()->collectionStatus( collectionId() ) &
        CollectionManager::CollectionQueryable )
        return true;
    return false;
}

bool
CollectionPrototype::isViewable() const
{
    if( CollectionManager::instance()->collectionStatus( collectionId() ) &
        CollectionManager::CollectionViewable )
        return true;
    return false;
}

bool
CollectionPrototype::supportsTranscode() const
{
    GET_COLLECTION( false )
    if( !collection->has<Capabilities::TranscodeCapability>() )
        return false;
    Transcoding::Controller *tcC = Amarok::Components::transcodingController();
    if( tcC && !tcC->availableEncoders().isEmpty() )
        return true;
    return false;
}

//private

CollectionPrototype::CollectionPrototype( Collection *collection )
: QObject( nullptr ) //script owned
, m_collection( collection )
{
    connect( collection, &Collections::Collection::updated, this, &CollectionPrototype::updated );
    connect( collection->location(), &Collections::CollectionLocation::aborted, this, &CollectionPrototype::aborted );
    connect( collection->location(), &Collections::CollectionLocation::finishCopy, this, &CollectionPrototype::finishCopy );
    connect( collection->location(), &Collections::CollectionLocation::finishRemove, this, &CollectionPrototype::finishRemove );
    connect( collection, &Collections::Collection::remove, this, &CollectionPrototype::removed );
}

Meta::TrackList
CollectionPrototype::removeInvalidTracks(const Meta::TrackList& tracks)
{
    Meta::TrackList cleaned;
    for( const Meta::TrackPtr &track : tracks )
    {
        if( track )
            cleaned << track;
    }
    return cleaned;
}

QString
CollectionPrototype::prettyLocation() const
{
    GET_COLLECTION( QString() )
    return collection->location()->prettyLocation();
}

QStringList
CollectionPrototype::actualLocation() const
{
    GET_COLLECTION( QStringList() )
    return collection->location()->actualLocation();
}

#undef GET_COLLECTION
