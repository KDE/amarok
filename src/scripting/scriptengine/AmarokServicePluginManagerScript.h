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

#ifndef AMAROK_SERVICE_PLUGIN_MANAGER_SCRIPT_H
#define AMAROK_SERVICE_PLUGIN_MANAGER_SCRIPT_H

#include <QObject>
#include <QStringList>


class QJSEngine;

namespace AmarokScript
{
    // SCRIPTDOX: Amarok.ServicePluginManager
    class AmarokServicePluginManagerScript : public QObject
    {
        Q_OBJECT

        public:
            explicit AmarokServicePluginManagerScript( QJSEngine* scriptEngine );

            Q_INVOKABLE QStringList loadedServices();
            Q_INVOKABLE QStringList loadedServiceNames();
            Q_INVOKABLE QString serviceDescription( const QString &service );
            Q_INVOKABLE QString serviceMessages( const QString &service );
            Q_INVOKABLE QString sendMessage( const QString &service, const QString &message );
    };
}

#endif
