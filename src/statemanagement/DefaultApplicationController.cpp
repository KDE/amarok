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

#include "DefaultApplicationController.h"

#include "Components.h"
#include "EngineController.h"

#include "collection/CollectionLocationDelegateImpl.h"

#include <QMetaObject>

using namespace Amarok;

DefaultApplicationController::DefaultApplicationController()
    : ApplicationController()
{
    //there can be only one applicationcontroller
    ApplicationController *oldController = Components::setApplicationController( this );
    Q_ASSERT( !oldController );
}

DefaultApplicationController::~DefaultApplicationController()
{
    Components::setApplicationController( 0 );
}

void
DefaultApplicationController::start()
{
    //construct all central components
    initCoreServiceProxies();
    initSqlStorage();
    initEngineController();
    initCollectionManager();
    initCollectionLocationDelegate();

}

void
DefaultApplicationController::shutdown()
{
    //destroy all central components instead of letting them be
    //destroyed in a random order as static objects


    //do more stuff

    delete Components::setEngineController( 0 );
    delete Components::setCollectionLocationDelegate( 0 );
}

void
DefaultApplicationController::initCoreServiceProxies()
{
    //we have core services that (e.g. StatusBar) that
    //basically all other components could sensibly use
    //unfortunately some of these components are GUI
    //components and can only be constructed quite late
    //therefore we provide proxies here that won't do anything, but forward
    //to the real implementation as soon as possible

}

void
DefaultApplicationController::initSqlStorage()
{
    //SqlStorage should really be moved out of SqlCollection and into a separate plugin that
    //can be loaded independently of the collection
}

void
DefaultApplicationController::initEngineController()
{
    EngineController *controller = new EngineController();
    Components::setEngineController( controller );
    bool invoked = QMetaObject::invokeMethod( controller, "initializePhonon", Qt::DirectConnection );
    Q_ASSERT( invoked );
}

void
DefaultApplicationController::initCollectionManager()
{

}

void
DefaultApplicationController::initCollectionLocationDelegate()
{
    Components::setCollectionLocationDelegate( new CollectionLocationDelegateImpl() );
}
