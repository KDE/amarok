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

#ifndef AMAROK_COMPONENTS_H
#define AMAROK_COMPONENTS_H

#include "shared/amarok_export.h"

namespace Amarok
{
    class ApplicationController;
    class Logger;
}

namespace Collections
{
    class CollectionLocationDelegate;
}

class CollectionManager;
class EngineController;
class SqlStorage;

namespace Amarok
{
    namespace Components
    {
        AMAROK_EXPORT CollectionManager* collectionManager();
        AMAROK_EXPORT CollectionManager* setCollectionManager( CollectionManager *mgr );

        AMAROK_EXPORT EngineController* engineController();
        AMAROK_EXPORT EngineController* setEngineController( EngineController *controller );

        AMAROK_EXPORT SqlStorage* sqlStorage();
        AMAROK_EXPORT SqlStorage* setSqlStorage( SqlStorage *storage );

        AMAROK_EXPORT Amarok::Logger* logger();
        AMAROK_EXPORT Amarok::Logger* setLogger( Amarok::Logger *logger );

        AMAROK_EXPORT Amarok::ApplicationController* applicationController();
        AMAROK_EXPORT Amarok::ApplicationController* setApplicationController( Amarok::ApplicationController *controller );

        AMAROK_EXPORT Collections::CollectionLocationDelegate* collectionLocationDelegate();
        AMAROK_EXPORT Collections::CollectionLocationDelegate* setCollectionLocationDelegate( Collections::CollectionLocationDelegate *delegate );
    }
}

#endif
