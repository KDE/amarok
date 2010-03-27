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
#include "core/support/Amarok.h"
#include "App.h"
#include "core/support/Debug.h"
#include "MainWindow.h"

#include <KAction>
#include <KActionCollection>


namespace AmarokScript
{
    AmarokWindowScript::AmarokWindowScript( QScriptEngine* ScriptEngine, QList<QObject*>* guiPtrList )
    : QObject( kapp )
    {
        m_ToolsMenu = The::mainWindow()->ToolsMenu();
        m_SettingsMenu = The::mainWindow()->SettingsMenu();
        m_ScriptEngine = ScriptEngine;
        m_guiPtrList = guiPtrList;
    }

    AmarokWindowScript::~AmarokWindowScript()
    {
    }

    bool AmarokWindowScript::addToolsMenu( QString id, QString MenuTitle, QString icon )
    {
        DEBUG_BLOCK
        KActionCollection* const ac = Amarok::actionCollection();
        if ( !ac->action( id ) )
        {
            KAction *action = new KAction( KIcon( icon ), MenuTitle, The::mainWindow() );
            ac->addAction( id, action );
            m_ToolsMenu->addAction( ac->action( id ) );
            m_ToolsMenu->addAction( action );
            QScriptValue newMenu = m_ScriptEngine->newQObject( action );
            m_ScriptEngine->globalObject().property( "Amarok" ).property( "Window" ).property( "ToolsMenu" ).setProperty( id, newMenu );
            m_guiPtrList->append( action );
        }
        else
            return false;
        return true;
    }

    void AmarokWindowScript::addToolsSeparator()
    {
        QAction* action = m_ToolsMenu->addSeparator();
        m_guiPtrList->append( action );
    }

    bool AmarokWindowScript::addSettingsMenu( QString id, QString MenuTitle, QString icon )
    {
        DEBUG_BLOCK
        KActionCollection* const ac = Amarok::actionCollection();
        if ( !ac->action( id ) )
        {
            KAction *action = new KAction( KIcon( icon ), MenuTitle, The::mainWindow() );
            ac->addAction( id, action );
            m_SettingsMenu->addAction( ac->action( id ) );
            m_SettingsMenu->addAction( action );
            QScriptValue newMenu = m_ScriptEngine->newQObject( action );
            m_ScriptEngine->globalObject().property( "Amarok" ).property( "Window" ).property( "SettingsMenu" ).setProperty( id, newMenu );
            m_guiPtrList->append( action );
        }
        else
            return false;
        return true;
    }

    void AmarokWindowScript::addSettingsSeparator()
    {
        QAction* action = m_SettingsMenu->addSeparator();
        m_guiPtrList->append( action );
    }

    void AmarokWindowScript::showBrowser( QString browser ) const
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
}

#include "AmarokWindowScript.moc"

