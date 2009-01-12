/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *   Copyright (c) 2008 Bart Cerneels <bart.cerneels@kde.org>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef PLAYLISTGROUP_H
#define PLAYLISTGROUP_H

#include "meta/Meta.h"
#include "PlaylistViewItem.h"
#include "meta/Playlist.h"

#include <QString>
#include <QStringList>

#include <KSharedPtr>

namespace Meta
{
    class PlaylistGroup;
    typedef KSharedPtr<PlaylistGroup> PlaylistGroupPtr;
    typedef QList<PlaylistGroupPtr> PlaylistGroupList;

    /**
        A class for allowing a "folder structure" in the playlist browser.

        @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
    */
    class PlaylistGroup : public PlaylistViewItem
    {
        public:
            virtual ~PlaylistGroup();

            virtual QString name() const = 0;
            virtual QString description() const = 0;

            virtual int childCount() const;

            virtual PlaylistGroupPtr parent() const = 0;

            virtual void rename( const QString &name ) = 0;

            virtual PlaylistGroupList childGroups() const = 0;
            virtual Meta::PlaylistList childPlaylists() const = 0;

            virtual void reparent( PlaylistGroupPtr parent ) = 0;

            virtual void clear();

            virtual void addChildPlaylist( Meta::PlaylistPtr playlist );
            virtual void removeChildPlaylist( Meta::PlaylistPtr playlist );

            virtual void addChildGroup( Meta::PlaylistGroup group );
            virtual void removeChildGroup( Meta::PlaylistGroup group );
    };
}

#endif
