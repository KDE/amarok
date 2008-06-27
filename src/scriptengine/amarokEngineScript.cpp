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

    void amarokEngineScript::Seek( int ms )
    {
        The::engineController()->seek( ms );
    }

    void amarokEngineScript::SeekRelative( int ms )
    {
        The::engineController()->seekRelative( ms );
    }

    void amarokEngineScript::SeekForward( int ms )
    {
        The::engineController()->seekForward( ms );
    }

    void amarokEngineScript::SeekBackward( int ms )
    {
        The::engineController()->seekBackward( ms );
    }

    int amarokEngineScript::increaseVolume( int ticks )
    {
        return The::engineController()->increaseVolume( ticks );
    }

    int amarokEngineScript::decreaseVolume( int ticks )
    {
        return The::engineController()->decreaseVolume( ticks );
    }

    int amarokEngineScript::setVolume( int percent )
    {
        return The::engineController()->setVolume( percent );
    }

    void amarokEngineScript::Mute()
    {
        The::engineController()->mute();
    }

}

#include "amarokEngineScript.moc"
