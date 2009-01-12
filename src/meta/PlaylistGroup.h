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
    class PlaylistGroup : public virtual QSharedData
    {
        public:
            virtual ~PlaylistGroup() {};

            virtual QString name() const = 0;
            virtual QString description() const = 0;

            virtual Meta::PlaylistGroupPtr parent() const = 0;

            virtual void setName( const QString &name ) = 0;
            virtual void setDescription( const QString &description ) = 0;
            virtual void setParent( Meta::PlaylistGroupPtr parent ) = 0;
    };
}

Q_DECLARE_METATYPE( Meta::PlaylistGroupPtr )
Q_DECLARE_METATYPE( Meta::PlaylistGroupList )

#endif
