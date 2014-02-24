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

#include <QObject>
#include <QMetaType>
#include <QWeakPointer>

class EventFilter;
class KMenu;
class QFont;
class QMainWindow;
class QMenu;
class QPalette;
class QScriptEngine;

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
            AmarokWindowScript( AmarokScriptEngine* scriptEngine );

        public slots:
            void addToolsMenu( QMenu *menu );
            void addSettingsMenu( QMenu *menu );
            bool addToolsMenu( QString id, QString menuTitle, QString icon = "amarok" );
            void addToolsSeparator();
            bool addSettingsMenu( QString id, QString menuTitle, QString icon = "amarok" );
            void addSettingsSeparator();
            void showTrayIcon( bool show );

            /**
             * Show tooltips with the widget's name on mouseover.
             * @param showDelay the duration after which toolTip appears.
             * @param copyDelay the duration after
             *
             * Must restart Amarok afetr invoking this function to revert tooltips
             * to normal.
             */
            void showToolTip();

        signals:
            void prepareToQuit();
            void newPalette( QPalette );

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

            QString activeBrowserName();
            bool isTrayIconShown();
            QMainWindow* mainWindow();
            void setStyleSheet( const QString &styleSheet );
            QString styleSheet() const;
            QFont font() const;
            void setFont( const QFont &font );
            QPalette palette() const;
            void setPalette( const QPalette & palette );

            QWeakPointer<KMenu> m_toolsMenu;
            QWeakPointer<KMenu> m_settingsMenu;
            AmarokScriptEngine*   m_scriptEngine;
    };
}

Q_DECLARE_METATYPE( QMainWindow* )
Q_DECLARE_METATYPE( QPalette )

#endif
