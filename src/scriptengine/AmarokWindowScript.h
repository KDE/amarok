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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_WINDOW_SCRIPT_H
#define AMAROK_WINDOW_SCRIPT_H

#include <KMenu>

#include <QList>
#include <QObject>
#include <QtScript>

namespace AmarokScript
{

    class AmarokWindowScript : public QObject
    {
        Q_OBJECT

        public:
            AmarokWindowScript( QScriptEngine* ScriptEngine, QList<QObject*>* guiPtrList );
            ~AmarokWindowScript();

        public slots:
            bool addToolsMenu( QString id, QString MenuTitle, QString icon = "amarok" );
            void addToolsSeparator();
            bool addSettingsMenu( QString id, QString MenuTitle, QString icon = "amarok" );
            void addSettingsSeparator();
            void showBrowser( QString browser ) const;
//TODO: show Tray Icon, Show Splash Screen
        private:
            KMenu*           m_ToolsMenu;
            KMenu*           m_SettingsMenu;
            QScriptEngine*   m_ScriptEngine;
            QList<QObject*>* m_guiPtrList;

    };
}

#endif
