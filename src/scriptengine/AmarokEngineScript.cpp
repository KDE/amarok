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
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "AmarokEngineScript.h"

#include "amarokconfig.h"
#include "App.h"
#include "EngineController.h"
#include "MainWindow.h"
//#include "mediabrowser.h"
#include "playlist/PlaylistActions.h"


#include <KSelectAction>

namespace AmarokScript
{
    AmarokEngineScript::AmarokEngineScript( QScriptEngine* ScriptEngine )
    : QObject( kapp )
    {
        Q_UNUSED( ScriptEngine );
        connect( The::engineController(), SIGNAL( trackChanged( Meta::TrackPtr ) ), this, SIGNAL( trackChanged() ) );
        connect( The::engineController(), SIGNAL( trackFinished() ), this, SIGNAL( trackFinished() ) );
        connect( The::engineController(), SIGNAL( trackSeeked( int ) ), this, SIGNAL( trackSeeked( int ) ) );
        connect( The::engineController(), SIGNAL( volumeChanged( int ) ), this, SIGNAL( volumeChanged( int ) ) );
        connect( The::engineController(), SIGNAL( trackPlayPause( int ) ), this, SIGNAL( trackPlayPause( int ) ) );
    }

    AmarokEngineScript::~AmarokEngineScript()
    {
    }

    void AmarokEngineScript::Play() const
    {
        The::engineController()->play();
    }

    void AmarokEngineScript::Stop( bool forceInstant ) const
    {
        The::engineController()->stop( forceInstant );
    }

    void AmarokEngineScript::Pause() const
    {
        The::engineController()->pause();
    }

    void AmarokEngineScript::Next() const
    {
        The::playlistActions()->next();
    }
    void AmarokEngineScript::Prev() const
    {
        The::playlistActions()->back();
    }

    void AmarokEngineScript::PlayPause() const
    {
        The::engineController()->playPause();
    }

    void AmarokEngineScript::Seek( int ms ) const
    {
        The::engineController()->seek( ms );
    }

    void AmarokEngineScript::SeekRelative( int ms ) const
    {
        The::engineController()->seekRelative( ms );
    }

    void AmarokEngineScript::SeekForward( int ms ) const
    {
        The::engineController()->seekForward( ms );
    }

    void AmarokEngineScript::SeekBackward( int ms ) const
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
        The::engineController()->toggleMute();
    }

    int AmarokEngineScript::trackPosition() const
    {
        return The::engineController()->trackPosition();
    }

    int AmarokEngineScript::trackPositionMs() const
    {
        return The::engineController()->trackPositionMs();
    }

    int AmarokEngineScript::engineState() const
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
            default:
            case Phonon::ErrorState:
                return -1;
        };
    }

    QVariant AmarokEngineScript::currentTrack() const
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
        Q_UNUSED( enable );
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
        Q_UNUSED( length );
    }
}

#include "AmarokEngineScript.moc"

