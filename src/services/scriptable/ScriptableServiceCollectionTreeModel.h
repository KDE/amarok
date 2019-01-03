/****************************************************************************************
 * Copyright (c) 2013 Matěj Laitl <matej@laitl.cz>                                      *
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

#ifndef SCRIPTABLESERVICECOLLECTIONTREEMODEL_H
#define SCRIPTABLESERVICECOLLECTIONTREEMODEL_H

#include "browsers/SingleCollectionTreeItemModel.h"

/**
 * Tiny wrapper around SingleCollectionTreeItemModel that implements
 * converting service tracks that are in fact playlist to MultiTracks (for mime data,
 * with help of ScriptableServiceQueryMaker
 */
class ScriptableServiceCollectionTreeModel : public SingleCollectionTreeItemModel
{
    public:
        ScriptableServiceCollectionTreeModel( Collections::Collection *collection,
                                              const QList<CategoryId::CatMenuId> &levelType );

        using SingleCollectionTreeItemModel::mimeData; // prevent warning
        /**
         * Overridden to masquerade playlist tracks as MultiTracks
         */
        QMimeData* mimeData( const QList<CollectionTreeItem *> &items ) const override;
};

#endif // SCRIPTABLESERVICECOLLECTIONTREEMODEL_H
