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

#ifndef AMAROK_ENGINE_SCRIPT_H
#define AMAROK_ENGINE_SCRIPT_H

#include <QObject>
#include <QtScript>

namespace Amarok
{

    class AmarokEngineScript : public QObject
    {
        Q_OBJECT

        public:
            AmarokEngineScript( QScriptEngine* ScriptEngine );
            ~AmarokEngineScript();

        public slots:
            void Play();
            void Stop( bool forceInstant = false );
            void Pause();
            void PlayPause();
            void PlayAudioCD();
            void Seek( int ms );
            void SeekRelative( int ms );
            void SeekForward( int ms = 10000 );
            void SeekBackward( int ms = 10000 );
            int  increaseVolume( int ticks = 100/25 );
            int  decreaseVolume( int ticks = 100/25 );
            int  setVolume( int percent );
            void Mute();
        signals:
            void trackFinished();
            void trackChanged();
            void trackSeeked( int ); //return relative time in million second
        private:

    };
}

#endif
