/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef AMAROK_PRETTYTREEVIEW_H
#define AMAROK_PRETTYTREEVIEW_H

#include "amarok_export.h"

#include <QTreeView>

namespace Amarok
{
    /**
     * A utility QTreeView subcass that handles drawing nice, svg themed, rows and palette changes
     *
     * @author: Nikolaj Hald Nielsen <nhn@kde.org>
     */
    class AMAROK_EXPORT PrettyTreeView : public QTreeView
    {
        Q_OBJECT

        public:
            PrettyTreeView( QWidget *parent = 0 );
            virtual ~PrettyTreeView();

        public slots:
            /* There is a need to overload even this edit() variant, otherwise it hides
             * QAbstactItemView's implementation. Note that it is NOT safe to do anything
             * special in this method, as it is not virtual.
             * bool edit( const QModelIndex &index, EditTrigger trigger, QEvent *event )
             * IS virtual. */
            void edit( const QModelIndex &index );

        protected:
            bool edit( const QModelIndex &index, EditTrigger trigger, QEvent *event );
            void drawRow( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const;

        private slots:
            virtual void newPalette( const QPalette &palette );
    };
}

#endif
