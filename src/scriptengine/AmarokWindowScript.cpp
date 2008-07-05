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
        m_ToolMenu = MainWindow::self()->ToolMenu();
        m_ScriptEngine = ScriptEngine;
        m_guiPtrList = guiPtrList;
    }

    AmarokWindowScript::~AmarokWindowScript()
    {
    }

    bool AmarokWindowScript::addToolMenu( QString MenuTitle )
    {
        DEBUG_BLOCK
        KActionCollection* const ac = actionCollection();
        if ( !ac->action( MenuTitle ) )
        {
            KAction *action = new KAction( KIcon( "amarok" ), MenuTitle, MainWindow::self() );
            ac->addAction( MenuTitle, action );
            m_ToolMenu->addAction( ac->action( MenuTitle ) );
            m_ToolMenu->addAction( action );
            QScriptValue newMenu = m_ScriptEngine->newQObject( action );
            m_ScriptEngine->globalObject().property( "Amarok" ).property( "Window" ).property( "ToolMenu" ).setProperty( MenuTitle, newMenu );
            m_guiPtrList->append( action );
        }
        else
            return false;
        return true;
    }

    void AmarokWindowScript::addToolSeparator()
    {
        QAction* action = m_ToolMenu->addSeparator();
        m_guiPtrList->append( action );
    }

}

#include "AmarokWindowScript.moc"
