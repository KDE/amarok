/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2009 Seb Ruiz <ruiz@kde.org>                                           *
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

#ifndef AMAROK_COLLECTION_TREE_ITEM_DELEGATE_H
#define AMAROK_COLLECTION_TREE_ITEM_DELEGATE_H


#include <QAction>
#include <QStyledItemDelegate>
#include <QFont>
#include <QTreeView>

//Please leave the include style of this item as it is, as the "modern" inlcude style breaks
//build on *ubunt KDE 4.2.x.
#include <kwidgetitemdelegate.h>

class CollectionTreeItemDelegate : public KWidgetItemDelegate
{
    public:
        CollectionTreeItemDelegate( QTreeView *view );
        ~CollectionTreeItemDelegate();

        void paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const;
        QSize sizeHint( const QStyleOptionViewItem &option, const QModelIndex &index ) const;

    protected:
        virtual QList<QWidget*> createItemWidgets() const;
        virtual void updateItemWidgets( const QList<QWidget*> widgets,
                                        const QStyleOptionViewItem &option,
                                        const QPersistentModelIndex &index ) const;

    private:
        QStyledItemDelegate m_styledDelegate;

        QTreeView *m_view;
        QFont m_bigFont;
        QFont m_smallFont;
};

#endif
