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
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_COLLECTION_TREE_ITEM_DELEGATE_H 
#define AMAROK_COLLECTION_TREE_ITEM_DELEGATE_H 

#include <QStyledItemDelegate>
#include <QFont>
#include <QTreeView>

class ComparableRect; // Implemented in .cpp

class CollectionTreeItemDelegate : public QStyledItemDelegate
{
    public:
        CollectionTreeItemDelegate( QTreeView *view );
        ~CollectionTreeItemDelegate();

        void paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const;
        QSize sizeHint( const QStyleOptionViewItem &option, const QModelIndex &index ) const;

        /**
         * Determines if a particular CollectionAction is underneath a particular point, and
         * returns it. This is used for triggering the action on a click which is handled by the
         * view.
         * Note: This method is static because we do not have access to an instance of the
         * delegate from the view.
         * @param pos the position of the mouse cursor
         * @return the QAction under <param>pos</param>, or 0 if there is none
         */
        static QAction* actionUnderPoint( const QPoint pos );

    private:
        /**
         * A static map which holds a map between hit targets for actions and the corresponding
         * action. @see actionUnderPoint
         */
        static QMap<ComparableRect, QAction*> s_hitTargets;

        QTreeView *m_view;
        QFont m_bigFont;
        QFont m_smallFont;
};

#endif
