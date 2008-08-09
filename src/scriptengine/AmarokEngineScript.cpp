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

#include "amarokconfig.h"
#include "App.h"
#include "EngineController.h"
#include "MainWindow.h"
//#include "mediabrowser.h"
#include "playlist/PlaylistModel.h"

#include <QtScript>

#include <KSelectAction>

namespace AmarokScript
{
    AmarokEngineScript::AmarokEngineScript( QScriptEngine* ScriptEngine, QList<QObject*>* wrapperList )
    : QObject( kapp )
    {
        Q_UNUSED( ScriptEngine );
        m_wrapperList = wrapperList;
        connect( The::engineController(), SIGNAL( trackChanged( Meta::TrackPtr ) ), this, SIGNAL( trackChanged() ) );
        connect( The::engineController(), SIGNAL( trackFinished() ), this, SIGNAL( trackFinished() ) );
        connect( The::engineController(), SIGNAL( trackSeeked( int ) ), this, SIGNAL( trackSeeked( int ) ) );
        connect( The::engineController(), SIGNAL( volumeChanged( int ) ), this, SIGNAL( volumeChanged( int ) ) );
        connect( The::engineController(), SIGNAL( trackPlayPause( int ) ), this, SIGNAL( trackPlayPause( int ) ) );
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

    void AmarokEngineScript::Next()
    {
        The::playlistModel()->next();
    }
    void AmarokEngineScript::Prev()
    {
        The::playlistModel()->back();
    }

    void AmarokEngineScript::PlayPause()
    {
        The::engineController()->playPause();
    }

    void AmarokEngineScript::PlayAudioCD()
    {
        The::mainWindow()->playAudioCD();
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

    int AmarokEngineScript::IncreaseVolume( int ticks )
    {
        return The::engineController()->increaseVolume( ticks );
    }

    int AmarokEngineScript::DecreaseVolume( int ticks )
    {
        return The::engineController()->decreaseVolume( ticks );
    }

    void AmarokEngineScript::Mute()
    {
        The::engineController()->mute();
    }

    int AmarokEngineScript::trackPosition()
    {
        return The::engineController()->trackPosition();
    }

    int AmarokEngineScript::engineState()
    {
        switch( The::engineController()->state() )
        {
            case Phonon::PlayingState:
            case Phonon::BufferingState:
                return 0; //Playing
            case Phonon::PausedState:
                return 1; //Paused
            case Phonon::LoadingState:
            case Phonon::StoppedState:
                return 2; //Stopped
            case Phonon::ErrorState:
                return -1;
        };
    }

    QVariant AmarokEngineScript::currentTrack()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return QVariant::fromValue( track );
    }

    bool AmarokEngineScript::randomMode() const
    {
        return AmarokConfig::randomMode();
    }

    bool AmarokEngineScript::dynamicMode() const
    {
        return AmarokConfig::dynamicMode();
    }

    bool AmarokEngineScript::repeatPlaylist() const
    {
        return Amarok::repeatPlaylist();
    }

    bool AmarokEngineScript::repeatTrack() const
    {
        return Amarok::repeatTrack();
    }

    void AmarokEngineScript::setRandomMode( bool enable )
    {
        static_cast<KSelectAction*>(Amarok::actionCollection()->action( "random_mode" ) )->setCurrentItem( enable ? AmarokConfig::EnumRandomMode::Tracks : AmarokConfig::EnumRandomMode::Off );
    }

    void AmarokEngineScript::setDynamicMode( bool enable )
    {
        
    }

    void AmarokEngineScript::setRepeatPlaylist( bool enable )
    {
        static_cast<KSelectAction*>( Amarok::actionCollection()->action( "repeat" ) )->setCurrentItem( enable ? AmarokConfig::EnumRepeat::Playlist : AmarokConfig::EnumRepeat::Off );
    }

    void AmarokEngineScript::setRepeatTrack( bool enable )
    {
        static_cast<KSelectAction*>( Amarok::actionCollection()->action( "repeat" ) )->setCurrentItem( enable ? AmarokConfig::EnumRepeat::Track : AmarokConfig::EnumRepeat::Off );
    }

    int AmarokEngineScript::volume() const
    {
        return The::engineController()->volume();
    }

    void AmarokEngineScript::setVolume( int percent )
    {
        The::engineController()->setVolume( percent );
    }

    int AmarokEngineScript::fadeoutLength() const
    {
        return AmarokConfig::fadeoutLength();
    }

    void AmarokEngineScript::setFadeoutLength( int length )
    {
        
    }
}

#include "AmarokEngineScript.moc"
