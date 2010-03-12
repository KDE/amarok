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
#include "playlist/PlaylistModelStack.h"

#include <QAbstractItemModel>

class QAction;

Q_DECLARE_METATYPE( QAction* )
Q_DECLARE_METATYPE( QList<QAction*> )

namespace PlaylistBrowserNS {

/**
    @author Bart Cerneels <bart.cerneels@kde.org>
*/
class MetaPlaylistModel
{
    public:
        virtual ~MetaPlaylistModel() {}
        
        enum {
            PlaylistColumn = 0, //Data form the playlist itself
            LabelColumn, //Data from the labels. Can be used as foldernames in the view.
            ProviderColumn, //data from the PlaylistProvider
            CustomColumOffset //first column that can be used by subclasses for their own data
        };

        enum
        {
            DescriptionRole = Qt::UserRole,
            ByLineRole, //show some additional info like count or status. Displayed under description
            ActionCountRole,
            ActionRole, //list of QActions for the index
            CustomRoleOffset //first role that can be used by sublasses for their own data
        };
};

}
#endif //AMAROK_METAPLAYLISTMODEL_H
