/****************************************************************************************
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
 * Copyright (c) 2013-2014 Anmol Ahuja <darthcodus@gmail.com>                           *
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

#include <QFont>
#include <QMap>
#include <QMetaType>
#include <QObject>
#include <QPalette>
#include <QPointer>
#include <QString>
#include <QMainWindow>

class QMenu;

namespace AmarokScript
{
    class AmarokScriptEngine;
    // SCRIPTDOX: Amarok.Window
    class AmarokWindowScript : public QObject
    {
        Q_OBJECT

        Q_PROPERTY( bool isTrayIconShown READ isTrayIconShown )
        Q_PROPERTY( QString activeBrowserName READ activeBrowserName )
        Q_PROPERTY( QMainWindow* mainWindow READ mainWindow )
        /**
         * Convenience method for mainWindow.styleSheet and setStyleSheet
         */
        Q_PROPERTY( QString styleSheet READ styleSheet WRITE setStyleSheet )
        Q_PROPERTY( QFont font READ font WRITE setFont )
        Q_PROPERTY( QPalette palette READ palette WRITE setPalette )

        public:
            explicit AmarokWindowScript( AmarokScriptEngine* scriptEngine );

            Q_INVOKABLE void addToolsMenu( QMenu *menu );
            Q_INVOKABLE void addSettingsMenu( QMenu *menu );
            Q_INVOKABLE bool addToolsMenu( const QString &id, const QString &menuTitle, const QString &icon = QStringLiteral("amarok") );
            Q_INVOKABLE void addToolsSeparator();
            Q_INVOKABLE bool addSettingsMenu( const QString &id, const QString &actionName, const QString &icon = QStringLiteral("amarok") );
            Q_INVOKABLE bool addCustomAction( const QString &menuName, const QString &id, const QString &actionName, const QString &icon = QStringLiteral("amarok") );
            Q_INVOKABLE void addSettingsSeparator();
            Q_INVOKABLE void showTrayIcon( bool show );

            /**
             * Show tooltips with the widget's name on mouseover.
             *
             * Must restart Amarok after invoking this function to revert tooltips
             * to normal.
             */
            Q_INVOKABLE void showToolTip();

        Q_SIGNALS:
            void prepareToQuit();
            void newPalette( QPalette );

        private:
            /**
              * adds an action with the given ID and title to the given menu
              *
              * @param menu the menu to which the action will be added
              * @param id the ID of the action
              * @param actionName the title of the action
              * @param menuProperty the name of the menu property for the script engine
              * @param icon the icon for the action
              *
              * @return true if adding the action was successful, false otherwise
              */
            bool addMenuAction( QMenu *menu, const QString &id, const QString &actionName, const QString &menuProperty, const QString &icon );

            QString activeBrowserName();
            bool isTrayIconShown();
            QMainWindow* mainWindow();
            void setStyleSheet( const QString &styleSheet );
            QString styleSheet() const;
            QFont font() const;
            void setFont( const QFont &font );
            QPalette palette() const;
            void setPalette( const QPalette & palette );

            QMap<QString, QMenu*> m_customMenus;
            QPointer<QMenu> m_toolsMenu;
            QPointer<QMenu> m_settingsMenu;
            AmarokScriptEngine*   m_scriptEngine;
    };
}

Q_DECLARE_METATYPE( QMainWindow* )
Q_DECLARE_METATYPE( QPalette )

#endif
