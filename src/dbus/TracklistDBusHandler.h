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

#ifndef TRACKLIST_DBUS_HANDLER_H
#define TRACKLIST_DBUS_HANDLER_H

#include <QObject>
#include <QVariantMap>

namespace Amarok
{
    class TracklistDBusHandler : public QObject
    {
        Q_OBJECT
        Q_CLASSINFO("D-Bus Interface", "org.freedesktop.MediaPlayer.tracklist")

        public:
            TracklistDBusHandler();

            int AddTrack( const QString &, bool );
            void DelTrack( int index );
            int GetCurrentTrack();
            int GetLength();
            QVariantMap GetMetadata( int );
            void SetLoop( bool enable );
            void SetRandom( bool enable );

            // NB: Amarok extensions, not part of the mpris spec
            void PlayTrack( int index );

        signals:
            void TrackListChange( int );

        private slots:
            void slotTrackListChange();
    };
}

#endif
