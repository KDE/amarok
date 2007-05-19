/******************************************************************************
 * copyright: (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>       *
 *                                                                            *
 *   This program is free software; you can redistribute it and/or modify     *
 *   it under the terms of the GNU General Public License version 2           *
 *   as published by the Free Software Foundation.                            *
 ******************************************************************************/

#ifndef COLLECTIONTREEVIEW_H
#define COLLECTIONTREEVIEW_H

#include "collectionsortfilterproxymodel.h"

#include <QSortFilterProxyModel>
#include <QTreeView>

class QSortFilterProxyModel;
class CollectionSortFilterProxyModel;
class CollectionTreeItemModel;

class CollectionTreeView: public QTreeView {
    Q_OBJECT
    public:
    CollectionTreeView( QWidget *parent = 0 );
    ~CollectionTreeView();

    QSortFilterProxyModel* filterModel() const;

    void setLevels( const QList<int> &levels );
    void setLevel( int level, int type );

    void setModel ( QAbstractItemModel * model );
	void contextMenuEvent(QContextMenuEvent* event);
    private:
        CollectionSortFilterProxyModel *m_filterModel;
        CollectionTreeItemModel *m_treeModel;
};

#endif
