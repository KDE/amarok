/******************************************************************************
 * Copyright (C) 2003 Stanislav Karchebny <berk@inbox.ru>                     *
 *           (C) 2004 Christian Muehlhaeuser <chris@chris.de>                 *
 *           (C) 2005 Ian Monroe <ian@monroe.nu>                              *
 *           (C) 2005 Seb Ruiz <ruiz@kde.org>                                 *
 *           (C) 2006 Alexandre Pereira de Oliveira <aleprj@gmail.com>        *
 *           (C) 2006 2007 Leonardo Franchi <lfranchi@gmail.com>              *
 *           (C) 2008 Peter ZHOU <peterzhoulei@gmail.com>                     *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/

#include "amarokScriptDBusHandler.h"

#include "Amarok.h"
#include "amarokconfig.h"
#include "App.h"
#include "ScriptManager.h"

#include "amarokScriptAdaptor.h"

namespace Amarok
{
    
    amarokScriptDBusHandler::amarokScriptDBusHandler()
    : QObject( kapp )
    {
        new amarokScriptAdaptor(this);
        QDBusConnection::sessionBus().registerObject("/Script", this);
    }
    
    bool amarokScriptDBusHandler::runScript(const QString& name)
    {
        return ScriptManager::instance()->runScript(name);
    }
    
    bool amarokScriptDBusHandler::stopScript(const QString& name)
    {
        return ScriptManager::instance()->stopScript(name);
    }
    
    QStringList amarokScriptDBusHandler::listRunningScripts()
    {
        return ScriptManager::instance()->listRunningScripts();
    }
    
    void amarokScriptDBusHandler::addCustomMenuItem(QString submenu, QString itemTitle )
    {
        Q_UNUSED( submenu ); Q_UNUSED( itemTitle );
        //PORT 2.0
        //         Playlist::instance()->addCustomMenuItem( submenu, itemTitle );
    }
    
    void amarokScriptDBusHandler::removeCustomMenuItem(QString submenu, QString itemTitle )
    {
        Q_UNUSED( submenu ); Q_UNUSED( itemTitle );
        //PORT 2.0
        //         Playlist::instance()->removeCustomMenuItem( submenu, itemTitle );
    }
    
    QString amarokScriptDBusHandler::readConfig(const QString& key)
    {
        QString cleanKey = key;
        KConfigSkeletonItem* configItem = AmarokConfig::self()->findItem(cleanKey.remove(' '));
        if (configItem)
            return configItem->property().toString();
        else
            return QString();
    }
    
    QStringList amarokScriptDBusHandler::readListConfig(const QString& key)
    {
        QString cleanKey = key;
        KConfigSkeletonItem* configItem = AmarokConfig::self()->findItem(cleanKey.remove(' '));
        QStringList stringList;
        if(configItem)
        {
            QVariantList variantList = configItem->property().toList();
            QVariantList::Iterator it = variantList.begin();
            while(it != variantList.end())
            {
                stringList << (*it).toString();
                ++it;
            }
        }
        return stringList;
    }
    
    QString amarokScriptDBusHandler::proxyForUrl(const QString& url)
    {
        return Amarok::proxyForUrl( url );
    }
    
    QString amarokScriptDBusHandler::proxyForProtocol(const QString& protocol)
    {
        return Amarok::proxyForProtocol( protocol );
    }
}

#include "amarokScriptDBusHandler.moc"
