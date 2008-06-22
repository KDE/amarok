/******************************************************************************
 * Copyright (C) 2003 Stanislav Karchebny <berk@inbox.ru>                     *
 *           (C) 2005 Ian Monroe <ian@monroe.nu>                              *
 *           (C) 2005 Seb Ruiz <ruiz@kde.org>                                 *
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

#ifndef AMAROK_SCRIPT_DBUS_HANDLER_H
#define AMAROK_SCRIPT_DBUS_HANDLER_H

#include <QObject>

namespace Amarok
{
    class amarokScriptDBusHandler : public QObject
    {
        Q_OBJECT

        public:
            amarokScriptDBusHandler();

            public /* DBus */ slots:
            virtual bool runScript(const QString&);
            virtual bool stopScript(const QString&);
            virtual QStringList listRunningScripts();
            virtual void addCustomMenuItem(QString submenu, QString itemTitle );
            virtual void removeCustomMenuItem(QString submenu, QString itemTitle );
            virtual QString readConfig(const QString& key);
            virtual QStringList readListConfig(const QString& key);
            virtual QString proxyForUrl(const QString& url);
            virtual QString proxyForProtocol(const QString& protocol);
        };
}

#endif
