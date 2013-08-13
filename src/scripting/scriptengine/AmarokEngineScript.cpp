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

#include "AmarokEqualizerScript.h"
#include "App.h"
#include "EngineController.h"
#include "amarokconfig.h"
#include "core/meta/Meta.h"
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistModel.h"

#include <QScriptEngine>

using namespace AmarokScript;

AmarokEngineScript::AmarokEngineScript( QScriptEngine* scriptEngine )
    : QObject( scriptEngine )
{
    QScriptValue scriptObject = scriptEngine->newQObject( this, QScriptEngine::AutoOwnership
                                                        , QScriptEngine::ExcludeSuperClassContents );
    scriptEngine->globalObject().property( "Amarok" ).setProperty( "Engine", scriptObject );

    EngineController *engine = The::engineController();
    connect( engine, SIGNAL(trackPositionChanged(qint64,bool)),
             this, SLOT(trackPositionChanged(qint64)) );
    connect( engine, SIGNAL(trackChanged(Meta::TrackPtr)), this, SIGNAL(trackChanged()) );
    connect( engine, SIGNAL(paused()), this, SLOT(slotPaused()) );
    connect( engine, SIGNAL(trackPlaying(Meta::TrackPtr)), this, SLOT(slotPlaying()) );
    connect( engine, SIGNAL(stopped(qint64,qint64)), this, SIGNAL(trackFinished()) );
    connect( engine, SIGNAL(currentMetadataChanged(QVariantMap)),
             this, SLOT(slotNewMetaData()) );
    connect( engine, SIGNAL(trackMetadataChanged(Meta::TrackPtr)),
             this, SLOT(slotNewMetaData()) );
    connect( engine, SIGNAL(albumMetadataChanged(Meta::AlbumPtr)),
             this, SLOT(slotNewMetaData()) );
    connect( engine, SIGNAL(volumeChanged(int)), this, SIGNAL(volumeChanged(int)) );

    new AmarokEqualizerScript( scriptEngine );
}

void
AmarokEngineScript::Play() const
{
    The::engineController()->play();
}

void
AmarokEngineScript::Stop( bool forceInstant ) const
{
    The::engineController()->stop( forceInstant );
}

void
AmarokEngineScript::Pause() const
{
    The::engineController()->pause();
}

void
AmarokEngineScript::Next() const
{
    The::playlistActions()->next();
}
void
AmarokEngineScript::Prev() const
{
    The::playlistActions()->back();
}

void
AmarokEngineScript::PlayPause() const
{
    The::engineController()->playPause();
}

void
AmarokEngineScript::Seek( int ms ) const
{
    The::engineController()->seekTo( ms );
}

void
AmarokEngineScript::SeekRelative( int ms ) const
{
    The::engineController()->seekBy( ms );
}

void
AmarokEngineScript::SeekForward( int ms ) const
{
    The::engineController()->seekBy( ms );
}

void
AmarokEngineScript::SeekBackward( int ms ) const
{
    The::engineController()->seekBy( -ms );
}

int
AmarokEngineScript::IncreaseVolume( int ticks )
{
    return The::engineController()->increaseVolume( ticks );
}

int
AmarokEngineScript::DecreaseVolume( int ticks )
{
    return The::engineController()->decreaseVolume( ticks );
}

void
AmarokEngineScript::Mute()
{
    The::engineController()->toggleMute();
}

int
AmarokEngineScript::trackPosition() const
{
    return The::engineController()->trackPosition();
}

int
AmarokEngineScript::trackPositionMs() const
{
    return The::engineController()->trackPositionMs();
}

int
AmarokEngineScript::engineState() const
{

    if( The::engineController()->isPlaying() )
        return Playing;
    else if( The::engineController()->isPaused() )
        return Paused;
    else
        return Stopped;
}

Meta::TrackPtr
AmarokEngineScript::currentTrack() const
{
    Meta::TrackPtr track = The::engineController()->currentTrack();
    return track;
}

void
AmarokEngineScript::trackPositionChanged( qint64 pos )
{
    emit trackSeeked( pos );
}

void
AmarokEngineScript::slotNewMetaData()
{
    emit newMetaData( QHash<qint64, QString>(), false );
}

void
AmarokEngineScript::slotPaused()
{
    emit trackPlayPause( Paused );
}

void
AmarokEngineScript::slotPlaying()
{
    emit trackPlayPause( Playing );
}

bool
AmarokEngineScript::randomMode() const
{
    return AmarokConfig::trackProgression() == AmarokConfig::EnumTrackProgression::RandomTrack ||
            AmarokConfig::trackProgression() == AmarokConfig::EnumTrackProgression::RandomAlbum;
}

bool
AmarokEngineScript::dynamicMode() const
{
    return AmarokConfig::dynamicMode();
}

bool
AmarokEngineScript::repeatPlaylist() const
{
    return AmarokConfig::trackProgression() == AmarokConfig::EnumTrackProgression::RepeatPlaylist;
}

bool
AmarokEngineScript::repeatTrack() const
{
    return AmarokConfig::trackProgression() == AmarokConfig::EnumTrackProgression::RepeatTrack;
}

void
AmarokEngineScript::setRandomMode( bool enable )
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

void
AmarokEngineScript::setDynamicMode( bool enable )
{
    Q_UNUSED( enable );
}

void
AmarokEngineScript::setRepeatPlaylist( bool enable )
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

void
AmarokEngineScript::setRepeatTrack( bool enable )
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

int
AmarokEngineScript::volume() const
{
    return The::engineController()->volume();
}

void
AmarokEngineScript::setVolume( int percent )
{
    The::engineController()->setVolume( percent );
}

int
AmarokEngineScript::fadeoutLength() const
{
    return AmarokConfig::fadeoutLength();
}

void
AmarokEngineScript::setFadeoutLength( int length )
{
    if( length < 400 )
        debug() << "Fadeout length must be >= 400";
    else
        AmarokConfig::setFadeoutLength( length );
}
