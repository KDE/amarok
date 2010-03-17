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

#include "MetaTypeExporter.h"
#include "EngineObserver.h"

#include <QObject>
#include <QtScript>

namespace AmarokScript
{
    class AmarokEngineScript : public QObject, public EngineObserver
    {
        Q_OBJECT

        Q_PROPERTY ( bool randomMode READ randomMode WRITE setRandomMode )
        Q_PROPERTY ( bool dynamicMode READ dynamicMode WRITE setDynamicMode )
        Q_PROPERTY ( bool repeatPlaylist READ repeatPlaylist WRITE setRepeatPlaylist )
        Q_PROPERTY ( bool repeatTrack READ repeatTrack WRITE setRepeatTrack )
        Q_PROPERTY ( int volume READ volume WRITE setVolume )
        Q_PROPERTY ( int fadeoutLength READ fadeoutLength WRITE setFadeoutLength )

        public:
            AmarokEngineScript( QScriptEngine* ScriptEngine );
            ~AmarokEngineScript();

            enum PlayerStatus
            {
                Playing  = 0,
                Paused   = 1,
                Stopped  = 2,
                Error    = -1
            };

        public slots:
            void Play() const;
            void Stop( bool forceInstant = false ) const;
            void Pause() const;
            void Next() const;
            void Prev() const;
            void PlayPause() const;
            void Seek( int ms ) const;
            void SeekRelative( int ms ) const;
            void SeekForward( int ms = 10000 ) const;
            void SeekBackward( int ms = 10000 ) const;
            int  IncreaseVolume( int ticks = 100/25 );
            int  DecreaseVolume( int ticks = 100/25 );
            void Mute();
            int  trackPosition() const;
            int  trackPositionMs() const;
            int  engineState() const;
            QVariant currentTrack() const;

        signals:
            void trackFinished(); // when playback stops altogether
            void trackChanged();
            void newMetaData( const QHash<qint64, QString>&, bool );
            void trackSeeked( int ); //return relative time in million second
            void volumeChanged( int );
            void trackPlayPause( int );  //Playing: 0, Paused: 1

        private:
            void engineVolumeChanged( int value );
            void engineTrackPositionChanged( qint64 position, bool userSeek );
            void engineTrackChanged( Meta::TrackPtr track );
            void engineTrackFinished( Meta::TrackPtr track );
            void engineNewMetaData( const QHash<qint64, QString> &newData, bool trackChanged );
            void engineStateChanged( Phonon::State currentState, Phonon::State oldState );

            bool randomMode() const;
            bool dynamicMode() const;
            bool repeatPlaylist() const;
            bool repeatTrack() const;
            void setRandomMode( bool enable );
            void setDynamicMode( bool enable ); //TODO: implement
            void setRepeatPlaylist( bool enable );
            void setRepeatTrack( bool enable );
            int  volume() const;
            void setVolume( int percent );
            int  fadeoutLength() const;
            void setFadeoutLength( int length ); //TODO:implement
    };
}

#endif
