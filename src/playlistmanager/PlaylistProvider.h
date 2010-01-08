/****************************************************************************************
 * Copyright (c) 2007 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#ifndef AMAROK_PLAYLISTPROVIDER_H
#define AMAROK_PLAYLISTPROVIDER_H

#include "amarok_export.h"
#include "plugin/plugin.h"
#include "meta/Playlist.h"

#include <QString>

class QAction;
class KIcon;

class AMAROK_EXPORT PlaylistProvider : public QObject, public Amarok::Plugin
{
    Q_OBJECT

    public:
        virtual ~PlaylistProvider() {}

        /**
        * @returns A translated string to identify this Provider.
        */
        virtual QString prettyName() const = 0;

        virtual KIcon icon() const = 0;

        /**
         * @returns An unique integer that identifies the category of the offered playlists.
         * Use the PlaylistManager::PlaylistCategory enum.
         */
        virtual int category() const = 0;

        /** @returns the number of playlists this provider has or a negative value if it
         * can not determine that before loading them all.
         */
        virtual int playlistCount() const { return -1; }
        virtual Meta::PlaylistList playlists() = 0;

        virtual QList<QAction *> playlistActions( Meta::PlaylistPtr playlist ) = 0;
        virtual QList<QAction *> trackActions( Meta::PlaylistPtr playlist,
                                                  int trackIndex ) = 0;

    signals:
        void updated();

};

#endif // AMAROK_PLAYLISTPROVIDER_H
