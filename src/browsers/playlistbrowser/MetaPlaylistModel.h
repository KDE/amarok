/****************************************************************************************
 * Copyright (c) 2009 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#ifndef AMAROK_METAPLAYLISTMODEL_H
#define AMAROK_METAPLAYLISTMODEL_H

#include "meta/Playlist.h"
#include "playlist/PlaylistController.h"

#include <QAbstractItemModel>

class QAction;


namespace PlaylistBrowserNS {

/**
    @author Bart Cerneels <bart.cerneels@kde.org>
*/
class MetaPlaylistModel
{
    public:
        virtual ~MetaPlaylistModel() {}
        
        enum
        {
            DescriptionRole = Qt::UserRole + 1,
            //What is the name of the group this Playlist is in.
            GroupRole,
            ByLineRole
        };

        virtual QList<QAction *> actionsFor( const QModelIndexList &indexes ) = 0;

        virtual void loadItems( QModelIndexList list, Playlist::AddOptions insertMode ) = 0;
};

}
#endif //AMAROK_METAPLAYLISTMODEL_H
