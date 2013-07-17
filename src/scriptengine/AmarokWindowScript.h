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

#ifndef AMAROK_WINDOW_SCRIPT_H
#define AMAROK_WINDOW_SCRIPT_H

#include <KMenu>

#include <QObject>
#include <QSharedPointer>

class QScriptEngine;

namespace AmarokScript
{
    // SCRIPTDOX: Amarok.Window
    class AmarokWindowScript : public QObject
    {
        Q_OBJECT

        public:
            AmarokWindowScript( QScriptEngine* scriptEngine );

        public slots:
            bool addToolsMenu( QString id, QString menuTitle, QString icon = "amarok" );
            void addToolsSeparator();
            bool addSettingsMenu( QString id, QString menuTitle, QString icon = "amarok" );
            void addSettingsSeparator();
            void showBrowser( QString browser ) const;
            //TODO: show Tray Icon

        signals:
            void prepareToQuit();

        private:
            /**
              * adds an action with the given ID and title to the given menu
              *
              * @param menu the menu to which the action will be added
              * @param id the ID of the action
              * @param menuTitle the title of the action
              * @param menuProperty the name of the menu property for the script engine
              * @param icon the icon for the action
              *
              * @return true if adding the action was successful, false otherwise
              */
            bool addMenuAction( QWeakPointer<KMenu> menu, QString id, QString menuTitle, QString menuProperty, QString icon );

            QWeakPointer<KMenu> m_toolsMenu;
            QWeakPointer<KMenu> m_settingsMenu;
            QScriptEngine*   m_scriptEngine;
            QList<QObject*> m_guiPtrList;
    };
}

#endif
