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

#ifndef MPRIS1_PLAYER_HANDLER_H
#define MPRIS1_PLAYER_HANDLER_H

#include "amarok_export.h"
#include "core/meta/forward_declarations.h"

#include <QObject>
#include <QVariantMap>
#include <QDBusArgument>

namespace Mpris1 {
    class PlayerHandler;
}

namespace Mpris1
{
    struct Status
    {
        int Play; //Playing = 0, Paused = 1, Stopped = 2
        int Random; //Linearly = 0, Randomly = 1
        int Repeat; //Go_To_Next = 0, Repeat_Current = 1
        int RepeatPlaylist; //Stop_When_Finished = 0, Never_Give_Up_Playing = 1
    };
}

Q_DECLARE_METATYPE( Mpris1::Status )

// Marshall the Status data into a D-BUS argument
QDBusArgument &operator << ( QDBusArgument &argument, const Mpris1::Status &status );
// Retrieve the Status data from the D-BUS argument
const QDBusArgument &operator >> ( const QDBusArgument &argument, Mpris1::Status &status );

namespace Mpris1
{
    class AMAROK_EXPORT PlayerHandler : public QObject
    {
        Q_OBJECT
        public:
            PlayerHandler();

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

        public Q_SLOTS:
            Status GetStatus();

            void Pause();
            void Play();
            void PlayPause();
            void Stop();
            void StopAfterCurrent();
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

            void updateStatus();

        Q_SIGNALS:
            void CapsChange( int );
            void TrackChange( QVariantMap );
            void StatusChange( Mpris1::Status );

        public:
            QVariantMap GetTrackMetadata( Meta::TrackPtr track );

        private Q_SLOTS:
            void slotTrackChanged( Meta::TrackPtr track );
            void slotStateChanged();
    };

} // namespace Amarok

#endif // MPRIS1_PLAYER_HANDLER_H
