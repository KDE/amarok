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

#include "amarok_export.h"

namespace Amarok
{
    class ApplicationController;
}
class CollectionLocationDelegate;
class CollectionManager;
class EngineController;
class SqlStorage;

namespace Amarok
{
    namespace Components
    {
        CollectionManager* collectionManager();
        CollectionManager* setCollectionManager( CollectionManager *mgr );

        EngineController* engineController();
        EngineController* setEngineController( EngineController *controller );

        SqlStorage* sqlStorage();
        SqlStorage* setSqlStorage( SqlStorage *storage );

        Amarok::ApplicationController* applicationController();
        Amarok::ApplicationController* setApplicationController( Amarok::ApplicationController *controller );

        CollectionLocationDelegate* collectionLocationDelegate();
        CollectionLocationDelegate* setCollectionLocationDelegate( CollectionLocationDelegate *delegate );
    }
}

#endif
