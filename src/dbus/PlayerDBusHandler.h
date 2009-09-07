/****************************************************************************************
 * Copyright (c) 2008 Ian Monroe <ian@monroe.nu>                                        *
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

#ifndef PLAYER_DBUS_HANDLER_H
#define PLAYER_DBUS_HANDLER_H

#include "meta/Meta.h"
#include "EngineObserver.h"

#include <QObject>
#include <QVariantMap>
#include <QDBusArgument>

namespace Amarok {
    class PlayerDBusHandler;
}

namespace The {
    AMAROK_EXPORT Amarok::PlayerDBusHandler* playerDBusHandler();
}

struct DBusStatus
{
    int Play; //Playing = 0, Paused = 1, Stopped = 2
    int Random; //Linearly = 0, Randomly = 1
    int Repeat; //Go_To_Next = 0, Repeat_Current = 1
    int RepeatPlaylist; //Stop_When_Finished = 0, Never_Give_Up_Playing = 1
};

Q_DECLARE_METATYPE( DBusStatus )

// Marshall the DBusStatus data into a D-BUS argument
QDBusArgument &operator << ( QDBusArgument &argument, const DBusStatus &status );
// Retrieve the DBusStatus data from the D-BUS argument
const QDBusArgument &operator >> ( const QDBusArgument &argument, DBusStatus &status );

namespace Amarok
{
    class AMAROK_EXPORT PlayerDBusHandler : public QObject, public EngineObserver
    {
        friend Amarok::PlayerDBusHandler* The::playerDBusHandler();

        Q_OBJECT
        public:
            PlayerDBusHandler();

            enum DBusCaps {
                 NONE                  = 0,
                 CAN_GO_NEXT           = 1 << 0,
                 CAN_GO_PREV           = 1 << 1,
                 CAN_PAUSE             = 1 << 2,
                 CAN_PLAY              = 1 << 3,
                 CAN_SEEK              = 1 << 4,
                 CAN_PROVIDE_METADATA  = 1 << 5,
                 CAN_HAS_TRACKLIST     = 1 << 6
             };

        public slots:
            DBusStatus GetStatus();

            void Pause();
            void Play();
            void PlayPause();
            void Stop();
            void Prev();
            void Next();
            void Repeat( bool on );

            int  PositionGet();
            void PositionSet( int time );

            int  VolumeGet();
            void VolumeSet( int vol );

            int GetCaps();
            QVariantMap GetMetadata();

            // NB: Amarok extensions, not part of the mpris spec
            void VolumeUp( int step ) const;
            void VolumeDown( int step ) const;
            void Mute() const;
            void ShowOSD() const;

            void LoadThemeFile( const QString &path ) const;

            void Forward( int time );
            void Backward( int time );

        signals:
            void CapsChange( int );
            void TrackChange( QVariantMap );
            void StatusChange( DBusStatus );

        private slots:
            void updateStatus();

        public:
            QVariantMap GetTrackMetadata( Meta::TrackPtr track );

        private:
            void engineTrackChanged( Meta::TrackPtr track );
            void engineStateChanged( Phonon::State currentState, Phonon::State oldState );

            static PlayerDBusHandler* s_instance;
    };

} // namespace Amarok

#endif
