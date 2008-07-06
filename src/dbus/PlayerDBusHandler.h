/******************************************************************************
 * Copyright (C) 2008 Ian Monroe <ian@monroe.nu>                              *
 *           (C) 2008 Peter ZHOU <peterzhoulei@gmail.com>                     *
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

#ifndef PLAYER_DBUS_HANDLER_H
#define PLAYER_DBUS_HANDLER_H

#include <QObject>
#include <QVariantMap>

namespace Amarok
{

    class PlayerDBusHandler : public QObject
    {
        Q_OBJECT
        public:
            PlayerDBusHandler();

            enum DBusStatus { Playing = 0, Paused = 1, Stopped = 2 };
            //http://wiki.xmms2.xmms.se/index.php/MPRIS#GetCaps
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
            int GetStatus();
            void Pause();
            void Play();
            void PlayPause();
            int PositionGet();
            void PositionSet(int in0);
            void Stop();
            int VolumeGet();
            void VolumeSet(int in0);
            int GetCaps();
            QVariantMap GetMetadata();
        signals:
            void CapsChange( int );
        private slots:
            void capsChangeSlot();
    };

} // namespace Amarok

#endif
