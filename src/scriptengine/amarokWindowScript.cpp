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

#include "amarokWindowScript.h"

#include "ActionClasses.h"
#include "Amarok.h"
#include "App.h"
#include "Debug.h"
#include "MainWindow.h"

#include <KAction>
#include <KActionCollection>
#include <KLocale>

#include <QtScript>

namespace Amarok
{
    amarokWindowScript::amarokWindowScript( QScriptEngine *ScriptEngine )
    : QObject( kapp )
    {
        m_ToolMenu = MainWindow::self()->ToolMenu();
    }

    amarokWindowScript::~amarokWindowScript()
    {
    }

    void amarokWindowScript::addMenu( QString MenuTitle )
    {
        DEBUG_BLOCK

        KActionCollection* const ac = actionCollection();
        KAction *action = new KAction( KIcon( "preferences-plugin-script-amarok" ), MenuTitle, MainWindow::self() );
        ac->addAction( "customized_menu", action );
        new Amarok::MenuAction( ac );
        m_ToolMenu->addAction( actionCollection()->action( "customized_menu" ) );
    }

}

#include "amarokWindowScript.moc"
