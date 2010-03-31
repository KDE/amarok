/******************************************************************************
 * Copyright (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>        *
 * Copyright (c) 2009 Soren Harward <stharward@gmail.com>                     *
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

/* A stripped-down version of CollectionTreeViewSimple that doesn't support fancy
 * filtering, sorting, selection, popupdropper, or actions. -- sth */

#ifndef COLLECTIONTREEVIEW_SIMPLE_H
#define COLLECTIONTREEVIEW_SIMPLE_H

#include "widgets/PrettyTreeView.h"

#include <QSet>

class CollectionTreeItem;
class CollectionTreeItemModelBase;
class QAbstractItemModel;

class CollectionTreeViewSimple: public Amarok::PrettyTreeView
{
    Q_OBJECT

    public:
        CollectionTreeViewSimple( QWidget* parent = 0 );
        ~CollectionTreeViewSimple();

        void setModel( QAbstractItemModel* model );

    protected slots:
        virtual void selectionChanged ( const QItemSelection&, const QItemSelection& );
        void slotExpand( const QModelIndex& );
        void slotCollapsed( const QModelIndex& );
        void slotExpanded( const QModelIndex& );

    private:
        CollectionTreeItemModelBase* m_model;
        QSet<CollectionTreeItem*> m_currentItems;

    signals:
        void itemSelected( CollectionTreeItem* );
};

#endif
