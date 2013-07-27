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

#include "AmarokWindowScript.h"

#include "ActionClasses.h"
#include "amarokconfig.h"
#include "core/support/Amarok.h"
#include "App.h"
#include "core/support/Debug.h"
#include "MainWindow.h"

#include <KAction>
#include <KActionCollection>

#include <QScriptEngine>

using namespace AmarokScript;

AmarokWindowScript::AmarokWindowScript( QScriptEngine* scriptEngine )
    : QObject( scriptEngine )
    , m_toolsMenu( The::mainWindow()->ToolsMenu() )
    , m_settingsMenu( The::mainWindow()->SettingsMenu() )
    , m_scriptEngine( scriptEngine )
{
    QScriptValue windowObject = scriptEngine->newQObject( this, QScriptEngine::AutoOwnership,
                                                            QScriptEngine::ExcludeSuperClassContents );
    scriptEngine->globalObject().property( "Amarok" ).setProperty( "Window", windowObject );

    windowObject.setProperty( "ToolsMenu", scriptEngine->newObject() );
    windowObject.setProperty( "SettingsMenu", scriptEngine->newObject() );

    connect( App::instance(), SIGNAL(prepareToQuit()),
                this, SIGNAL(prepareToQuit()) );
}

bool
AmarokWindowScript::addToolsMenu( QString id, QString menuTitle, QString icon )
{
    return addMenuAction( m_toolsMenu, id, menuTitle, "ToolsMenu", icon );
}

void
AmarokWindowScript::addToolsSeparator()
{
    QAction* action = m_toolsMenu.data()->addSeparator();
    m_guiPtrList.append( action );
}

bool
AmarokWindowScript::addSettingsMenu( QString id, QString menuTitle, QString icon )
{
    return addMenuAction( m_settingsMenu, id, menuTitle, "SettingsMenu", icon );
}

void
AmarokWindowScript::addSettingsSeparator()
{
    QAction* action = m_settingsMenu.data()->addSeparator();
    m_guiPtrList.append( action );
}

bool
AmarokWindowScript::addMenuAction( QWeakPointer<KMenu> menu, QString id, QString menuTitle, QString menuProperty, QString icon )
{
    bool retVal = true;

    KActionCollection* const ac = Amarok::actionCollection();

    if ( !ac->action( id ) )
    {
        KAction *action = new KAction( KIcon( icon ), menuTitle, this );
        ac->addAction( id, action );

        // don't forget to read the shortcut settings from the config file so
        // the shortcuts for the actions are updated
        ac->readSettings();

        // add the action to the given menu
        menu.data()->addAction( ac->action( id ) );

        m_guiPtrList.append( action );

        QScriptValue newMenu = m_scriptEngine->newQObject( action );
        m_scriptEngine->globalObject().property( "Amarok" ).property( "Window" ).property( menuProperty ).setProperty( id, newMenu );
    }
    else
        retVal = false;

    return retVal;
}

void
AmarokWindowScript::showBrowser( QString browser ) const
{
    if ( browser == "collection" )
        The::mainWindow()->showBrowser( "CollectionBrowser" );
    if ( browser == "playlist" )
        The::mainWindow()->showBrowser( "PlaylistBrowser" );
    if ( browser == "internet" )
        The::mainWindow()->showBrowser( "Internet" );
    if ( browser == "file" )
        The::mainWindow()->showBrowser( "FileBrowser" );
}

void
AmarokWindowScript::showTrayIcon( bool show )
{
    AmarokConfig::setShowTrayIcon( show );
    App::instance()->applySettings();
}

QString
AmarokWindowScript::activeBrowserName()
{
    return The::mainWindow()->activeBrowserName();
}

bool
AmarokWindowScript::isTrayIconShown()
{
    return AmarokConfig::showTrayIcon();
}
