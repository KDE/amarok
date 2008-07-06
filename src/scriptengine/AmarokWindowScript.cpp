/******************************************************************************
 * Copyright (C) 2008 Peter ZHOU <peterzhoulei@gmail.com>                     *
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

#include "AmarokWindowScript.h"

#include "ActionClasses.h"
#include "Amarok.h"
#include "App.h"
#include "Debug.h"
#include "MainWindow.h"

#include <KAction>
#include <KActionCollection>

#include <QtScript>

namespace Amarok
{
    AmarokWindowScript::AmarokWindowScript( QScriptEngine* ScriptEngine, QList<QObject*>* guiPtrList  )
    : QObject( kapp )
    {
        m_ToolsMenu = MainWindow::self()->ToolsMenu();
        m_SettingsMenu = MainWindow::self()->SettingsMenu();
        m_ScriptEngine = ScriptEngine;
        m_guiPtrList = guiPtrList;
    }

    AmarokWindowScript::~AmarokWindowScript()
    {
    }

    bool AmarokWindowScript::addToolsMenu( QString MenuTitle )
    {
        DEBUG_BLOCK
        QString title = "Tools_" + MenuTitle;
        KActionCollection* const ac = actionCollection();
        if ( !ac->action( title ) )
        {
            KAction *action = new KAction( KIcon( "amarok" ), MenuTitle, MainWindow::self() );
            ac->addAction( title, action );
            m_ToolsMenu->addAction( ac->action( title ) );
            m_ToolsMenu->addAction( action );
            QScriptValue newMenu = m_ScriptEngine->newQObject( action );
            m_ScriptEngine->globalObject().property( "Amarok" ).property( "Window" ).property( "ToolsMenu" ).setProperty( MenuTitle, newMenu );
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

    bool AmarokWindowScript::addSettingsMenu( QString MenuTitle )
    {
        DEBUG_BLOCK
        QString title = "Settings_" + MenuTitle;
        KActionCollection* const ac = actionCollection();
        if ( !ac->action( title ) )
        {
            KAction *action = new KAction( KIcon( "amarok" ), MenuTitle, MainWindow::self() );
            ac->addAction( title, action );
            m_SettingsMenu->addAction( ac->action( title ) );
            m_SettingsMenu->addAction( action );
            QScriptValue newMenu = m_ScriptEngine->newQObject( action );
            m_ScriptEngine->globalObject().property( "Amarok" ).property( "Window" ).property( "SettingsMenu" ).setProperty( MenuTitle, newMenu );
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
}

#include "AmarokWindowScript.moc"
