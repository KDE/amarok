/****************************************************************************************
* Copyright (c) 2010 Nathan Sala <sala.nathan@gmail.com>                               *
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

#include "TestDataEngine.h"
#include <KServiceTypeTrader>

TestDataEngine::TestDataEngine(QString identifier)
{
    Plasma::DataEngine* engine = 0;
    // load the engine, add it to the engines
    QString constraint = QString("[X-KDE-PluginInfo-Name] == '"+ identifier +"'").arg(identifier);
    KService::List offers = KServiceTypeTrader::self()->query("Plasma/DataEngine", constraint);
    QString error;

    if (!offers.isEmpty()) {
        QVariantList allArgs;
        allArgs << offers.first()->storageId();
        QString api = offers.first()->property("X-Plasma-API").toString();
        if (api.isEmpty()) {
            if (offers.first()) {
                KPluginLoader plugin(*offers.first());
                if (Plasma::isPluginVersionCompatible(plugin.pluginVersion())) {
                   engine = offers.first()->createInstance<Plasma::DataEngine>(0, allArgs, &error);
               }
            }
        } else {
            engine = new Plasma::DataEngine(0, offers.first());
        }
    }
    
    m_engine = engine;
}