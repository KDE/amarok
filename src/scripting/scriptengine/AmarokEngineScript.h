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

#ifndef AMAROK_ENGINE_SCRIPT_H
#define AMAROK_ENGINE_SCRIPT_H

#include "core/meta/forward_declarations.h"

#include <QHash>
#include <QObject>

class QScriptEngine;

namespace AmarokScript
{
    // SCRIPTDOX Amarok.Engine
    class AmarokEngineScript : public QObject
    {
        Q_OBJECT
        Q_PROPERTY( bool randomMode READ randomMode WRITE setRandomMode )
        Q_PROPERTY( bool dynamicMode READ dynamicMode )
        Q_PROPERTY( bool repeatPlaylist READ repeatPlaylist WRITE setRepeatPlaylist )
        Q_PROPERTY( bool repeatTrack READ repeatTrack WRITE setRepeatTrack )
        Q_PROPERTY( int volume READ volume WRITE setVolume )
        Q_PROPERTY( int fadeoutLength READ fadeoutLength WRITE setFadeoutLength )

        public:
            explicit AmarokEngineScript( QScriptEngine* scriptEngine );

            enum PlayerStatus
            {
                Playing  = 0,
                Paused   = 1,
                Stopped  = 2,
                Error    = -1 // deprecated. reason: Amarok will try to resolve the error by itself and never stay in the error state. In the worst case it will stop
            };

            Q_INVOKABLE void Play() const;
            Q_INVOKABLE void Stop( bool forceInstant = false ) const;
            Q_INVOKABLE void Pause() const;
            Q_INVOKABLE void Next() const;
            Q_INVOKABLE void Prev() const;
            Q_INVOKABLE void PlayPause() const;
            Q_INVOKABLE void Seek( int ms ) const;
            Q_INVOKABLE void SeekRelative( int ms ) const;
            Q_INVOKABLE void SeekForward( int ms = 10000 ) const;
            Q_INVOKABLE void SeekBackward( int ms = 10000 ) const;
            Q_INVOKABLE int  IncreaseVolume( int ticks = 100/25 );
            Q_INVOKABLE int  DecreaseVolume( int ticks = 100/25 );
            Q_INVOKABLE void Mute();
            /** This function returns the track position in seconds */
            Q_INVOKABLE int  trackPosition() const;
            /** This function returns the track position in milliseconds */
            Q_INVOKABLE int  trackPositionMs() const;
            /** This function returns the current engine state.
                @returns 0 when playing or buffering, 1 when paused, 2 when stopped or loading and -1 in case of an error.
            */
            Q_INVOKABLE int  engineState() const;
            /** This function returns the current track.
                The current track might even be valid when not in playing state.
            */
            Q_INVOKABLE Meta::TrackPtr currentTrack() const;

        Q_SIGNALS:
            void trackFinished(); // when playback stops altogether

            /** This signal will be emitted every time the current track changes.
                It will not be emitted if e.g. the title of the current track changes.
                For this you will need to connect to newMetaData signal.
            */
            void trackChanged();

            /** This signal will indicate newly received meta data.
                The signal will be triggered as soon as the phonon backend parses new meta data or
                the current track or album data is changed through Amarok.
                @param metaData Not longer filled. Use currentTrack to get the current meta data.
                @param newTrack Always false. Use trackChanged to find changed tracks.
            */
            void newMetaData( const QHash<qint64, QString> &metaData, bool newTrack );

            /** Will be emitted as soon as the user changes the track playback position. */
            void trackSeeked( int ); //return relative time in million second

            /** This signal will be emitted when the volume changes.
                The parameter is the relative volume between 0 (mute) and 100.
            */
            void volumeChanged( int );

            /** This signal is emitted when the engine state switches to play or pause.
                Note: You could get two trackPlayPause(1) in a row if e.g. the state
                changed to stopped in between (which you will notice if connecting to
                the trackFinished signal)
                @param state Is 0 when state changed to playing or 1 when the state switched to pause.
            */
            void trackPlayPause( int state );

        private Q_SLOTS:
            void trackPositionChanged( qint64 );
            void slotNewMetaData();
            void slotPaused();
            void slotPlaying();

        private:
            bool randomMode() const;
            bool dynamicMode() const;
            bool repeatPlaylist() const;
            bool repeatTrack() const;
            void setRandomMode( bool enable );
            void setRepeatPlaylist( bool enable );
            void setRepeatTrack( bool enable );
            int  volume() const;
            void setVolume( int percent );
            int  fadeoutLength() const;

            /**
             * Set the fadeout length. Must be >= 400.
             */
            void setFadeoutLength( int length );
    };
}

#endif
