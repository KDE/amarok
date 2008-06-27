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

#include "amarokEngineScript.h"

#include "App.h"
#include "EngineController.h"
#include "MainWindow.h"
#include "TheInstances.h"

#include <QtScript>

namespace Amarok
{
    amarokEngineScript::amarokEngineScript( QScriptEngine* ScriptEngine )
    : QObject( kapp )
    {

    }

    amarokEngineScript::~amarokEngineScript()
    {
    }

    void amarokEngineScript::Play()
    {
        The::engineController()->play();
    }

    void amarokEngineScript::Stop( bool forceInstant )
    {
        The::engineController()->stop( forceInstant );
    }

    void amarokEngineScript::Pause()
    {
        The::engineController()->pause();
    }

    void amarokEngineScript::PlayPause()
    {
        The::engineController()->playPause();
    }

    void amarokEngineScript::PlayAudioCD()
    {
        MainWindow::self()->playAudioCD();
    }

}

#include "amarokEngineScript.moc"
