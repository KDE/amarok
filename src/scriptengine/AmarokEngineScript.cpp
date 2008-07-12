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

#include "AmarokEngineScript.h"

#include "App.h"
#include "EngineController.h"
#include "MainWindow.h"

#include <QtScript>

namespace Amarok
{
    AmarokEngineScript::AmarokEngineScript( QScriptEngine* ScriptEngine )
    : QObject( kapp )
    {
        Q_UNUSED( ScriptEngine );
        connect( The::engineController(), SIGNAL( trackChanged( Meta::TrackPtr ) ), this, SIGNAL( trackChanged() ) );
        connect( The::engineController(), SIGNAL( trackFinished() ), this, SIGNAL( trackFinished() ) );
        connect( The::engineController(), SIGNAL( trackSeeked( int ) ), this, SIGNAL( trackSeeked( int ) ) );
    }

    AmarokEngineScript::~AmarokEngineScript()
    {
    }

    void AmarokEngineScript::Play()
    {
        The::engineController()->play();
    }

    void AmarokEngineScript::Stop( bool forceInstant )
    {
        The::engineController()->stop( forceInstant );
    }

    void AmarokEngineScript::Pause()
    {
        The::engineController()->pause();
    }

    void AmarokEngineScript::PlayPause()
    {
        The::engineController()->playPause();
    }

    void AmarokEngineScript::PlayAudioCD()
    {
        MainWindow::self()->playAudioCD();
    }

    void AmarokEngineScript::Seek( int ms )
    {
        The::engineController()->seek( ms );
    }

    void AmarokEngineScript::SeekRelative( int ms )
    {
        The::engineController()->seekRelative( ms );
    }

    void AmarokEngineScript::SeekForward( int ms )
    {
        The::engineController()->seekForward( ms );
    }

    void AmarokEngineScript::SeekBackward( int ms )
    {
        The::engineController()->seekBackward( ms );
    }

    int AmarokEngineScript::increaseVolume( int ticks )
    {
        return The::engineController()->increaseVolume( ticks );
    }

    int AmarokEngineScript::decreaseVolume( int ticks )
    {
        return The::engineController()->decreaseVolume( ticks );
    }

    int AmarokEngineScript::setVolume( int percent )
    {
        return The::engineController()->setVolume( percent );
    }

    void AmarokEngineScript::Mute()
    {
        The::engineController()->mute();
    }
}

#include "AmarokEngineScript.moc"
