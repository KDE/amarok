/****************************************************************************************
 * Copyright (c) 2010 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#include "core/support/Components.h"
#include <QObject>

class ComponentsPrivate
{
public:
    ComponentsPrivate()
        : collectionManager( nullptr )
        , engineController( nullptr )
        , sqlStorage( nullptr )
        , applicationController( nullptr )
        , collectionLocationDelegate( nullptr )
        , transcodingController( nullptr )
        , statSyncingController( nullptr )
    {}

    CollectionManager *collectionManager;
    EngineController *engineController;
    SqlStorage *sqlStorage;
    Amarok::ApplicationController *applicationController;
    Collections::CollectionLocationDelegate *collectionLocationDelegate;
    Transcoding::Controller *transcodingController;
    StatSyncing::Controller *statSyncingController;
};

//using a static variable is ok in this case as ComponentsPrivate does nothing on destruction
//in particular it does not delete any objects
Q_GLOBAL_STATIC( ComponentsPrivate, d )

//a define might be helpful for these getter/setters

#define COMPONENT_ACCESSORS( Type, getter, setter ) \
Type \
Amarok::Components::getter() \
{ \
    return d->getter ; \
} \
Type \
Amarok::Components::setter( Type type ) \
{ \
    Type old = d->getter; \
    d->getter = type; \
    return old; \
}

COMPONENT_ACCESSORS( CollectionManager*, collectionManager, setCollectionManager )

COMPONENT_ACCESSORS( EngineController*, engineController, setEngineController )

COMPONENT_ACCESSORS( SqlStorage*, sqlStorage, setSqlStorage )

COMPONENT_ACCESSORS( Amarok::ApplicationController*, applicationController, setApplicationController )

COMPONENT_ACCESSORS( Collections::CollectionLocationDelegate*, collectionLocationDelegate, setCollectionLocationDelegate )

COMPONENT_ACCESSORS( Transcoding::Controller*, transcodingController, setTranscodingController )

COMPONENT_ACCESSORS( StatSyncing::Controller*, statSyncingController, setStatSyncingController )
