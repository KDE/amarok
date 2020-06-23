/****************************************************************************************
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
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

#include "AmarokServicePluginManagerScript.h"

#include "App.h"
#include "services/ServicePluginManager.h"

#include <QJSEngine>

using namespace AmarokScript;

AmarokServicePluginManagerScript::AmarokServicePluginManagerScript( QJSEngine *engine )
    : QObject( engine )
{
    QJSValue scriptObject = engine->newQObject( this );
    engine->globalObject().property( QStringLiteral("Amarok") ).setProperty( QStringLiteral("ServicePluginManager"), scriptObject );
}

QStringList
AmarokServicePluginManagerScript::loadedServices()
{
    return ServicePluginManager::instance()->loadedServices();
}

QStringList
AmarokServicePluginManagerScript::loadedServiceNames()
{
    return ServicePluginManager::instance()->loadedServiceNames();
}

QString
AmarokServicePluginManagerScript::serviceDescription( const QString &service )
{
    return ServicePluginManager::instance()->serviceDescription( service );
}

QString
AmarokServicePluginManagerScript::serviceMessages( const QString &service )
{
    return ServicePluginManager::instance()->serviceMessages( service );
}

QString
AmarokServicePluginManagerScript::sendMessage( const QString &service, const QString &message )
{
    return ServicePluginManager::instance()->sendMessage( service, message );
}
