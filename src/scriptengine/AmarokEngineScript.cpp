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

#include "AmarokEngineScript.h"

#include "amarokconfig.h"
#include "App.h"
#include "EngineController.h"
#include "MainWindow.h"
//#include "mediabrowser.h"
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistModel.h"


#include <KSelectAction>

namespace AmarokScript
{
    AmarokEngineScript::AmarokEngineScript( QScriptEngine* ScriptEngine )
        : QObject( kapp )
        , EngineObserver( The::engineController() )
    {
        Q_UNUSED( ScriptEngine );
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
                return Playing;
            case Phonon::PausedState:
                return Paused;
            case Phonon::LoadingState:
            case Phonon::StoppedState:
                return Stopped;
            default:
            case Phonon::ErrorState:
                return Error;
        };
    }

    QVariant AmarokEngineScript::currentTrack() const
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return QVariant::fromValue( track );
    }

    void AmarokEngineScript::engineVolumeChanged( int newVolume )
    {
        emit volumeChanged( newVolume );
    }
    void AmarokEngineScript::engineTrackPositionChanged( qint64 position, bool userSeek )
    {
        Q_UNUSED( userSeek )
        emit trackSeeked( position );
    }
    void AmarokEngineScript::engineTrackChanged( Meta::TrackPtr track )
    {
        emit trackChanged();
        if ( track.isNull() )
            emit trackFinished();
    }
    void AmarokEngineScript::engineNewMetaData( const QHash<qint64, QString> &newMetaData, bool trackChanged )
    {
        emit( newMetaData, trackChanged );
    }
    void AmarokEngineScript::engineStateChanged( Phonon::State newState, Phonon::State oldState )
    {
        Q_UNUSED( oldState );

        if( newState == Phonon::PlayingState )
            emit trackPlayPause( Playing );
        else if( newState == Phonon::PausedState )
            emit trackPlayPause( Paused );
    }

    bool AmarokEngineScript::randomMode() const
    {
        return AmarokConfig::trackProgression() == AmarokConfig::EnumTrackProgression::RandomTrack ||
               AmarokConfig::trackProgression() == AmarokConfig::EnumTrackProgression::RandomAlbum;
    }

    bool AmarokEngineScript::dynamicMode() const
    {
        return AmarokConfig::dynamicMode();
    }

    bool AmarokEngineScript::repeatPlaylist() const
    {
        return AmarokConfig::trackProgression() == AmarokConfig::EnumTrackProgression::RepeatPlaylist;
    }

    bool AmarokEngineScript::repeatTrack() const
    {
        return AmarokConfig::trackProgression() == AmarokConfig::EnumTrackProgression::RepeatTrack;
    }

    void AmarokEngineScript::setRandomMode( bool enable )
    {
        if( enable )
        {
            AmarokConfig::setTrackProgression( AmarokConfig::EnumTrackProgression::RandomTrack );
            The::playlistActions()->playlistModeChanged();
        }
        else if( AmarokConfig::trackProgression() == AmarokConfig::EnumTrackProgression::RandomTrack )
        {
             AmarokConfig::setTrackProgression( AmarokConfig::EnumTrackProgression::Normal );
            The::playlistActions()->playlistModeChanged();
        }  
    }

    void AmarokEngineScript::setDynamicMode( bool enable )
    {
        Q_UNUSED( enable );
    }

    void AmarokEngineScript::setRepeatPlaylist( bool enable )
    {
        if( enable )
        {
            AmarokConfig::setTrackProgression( AmarokConfig::EnumTrackProgression::RepeatPlaylist );
            The::playlistActions()->playlistModeChanged();
        }
        else if( AmarokConfig::trackProgression() == AmarokConfig::EnumTrackProgression::RepeatPlaylist )
        {
             AmarokConfig::setTrackProgression( AmarokConfig::EnumTrackProgression::Normal );
            The::playlistActions()->playlistModeChanged();
        }  
    }

    void AmarokEngineScript::setRepeatTrack( bool enable )
    {
       if( enable )
        {
            AmarokConfig::setTrackProgression( AmarokConfig::EnumTrackProgression::RepeatTrack );
            The::playlistActions()->playlistModeChanged();
        }
        else if( AmarokConfig::trackProgression() == AmarokConfig::EnumTrackProgression::RepeatTrack )
        {
             AmarokConfig::setTrackProgression( AmarokConfig::EnumTrackProgression::Normal );
            The::playlistActions()->playlistModeChanged();
        }  
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

