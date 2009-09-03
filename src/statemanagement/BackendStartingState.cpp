/****************************************************************************************
 * Copyright (c) 2008 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#include "BackendStartingState.h"

#include "EngineController.h"
#include "CollectionManager.h"
#include "MountPointManager.h"

Amarok::BackendStartingState::BackendStartingState
    : State()
{
}

void
Amarok::BackendStartingState::activated()
{
    //this probably won't compile
    EngineController *ec = new EngineController();
    CollectionManager *cm = new CollectionManager();
    context()->setEngineController( ec );
    context()->setCollectionManager( cm );
    
    
    //TODO: move mpm into SqlCollection. keeping it in libamarok is so wrong
    MountPointManager *mpm = new MountPointManager();
    context()->setMountPointManager( mpm );
    //TODO activate dbus interface for backend subsystems
}
