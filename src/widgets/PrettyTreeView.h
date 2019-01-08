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
     * A utility QTreeView subcass that handles:
     * - drawing nice (svg themed) rows
     * - palette changes
     * - nicer expanding/collapsing interaction even when single click is used
     * - decorator actions for root level items when isRootDecorated() is false
     *
     * If you use decorator actions, don't forget to set mouseTracking to true as
     * PrettyTreeView doesn't do it automatically as it would be too costly for models
     * that don't use the actions.
     *
     * @author: Nikolaj Hald Nielsen <nhn@kde.org>
     */
    class AMAROK_EXPORT PrettyTreeView : public QTreeView
    {
        Q_OBJECT

        public:
            explicit PrettyTreeView( QWidget *parent = nullptr );
            ~PrettyTreeView() override;
        /**
         * Return pointer to decorator action which was most recently mouse-pressed
         * or null it mouse button was released since then. Used by PrettyTreeDelegate.
         */
        QAction *pressedDecoratorAction() const;

        public Q_SLOTS:
            /* There is a need to overload even this edit() variant, otherwise it hides
             * QAbstractItemView's implementation. Note that it is NOT safe to do anything
             * special in this method, as it is not virtual.
             * bool edit( const QModelIndex &index, EditTrigger trigger, QEvent *event )
             * IS virtual. */
            void edit( const QModelIndex &index );


        protected:
            bool edit( const QModelIndex &index, EditTrigger trigger, QEvent *event ) override;
            void drawRow( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const override;

            /**
             * Reimplemented to trigger item redraw in case mouse is over an item which
             * has decorator actions.
             */
            void mouseMoveEvent( QMouseEvent *event ) override;

            /**
             * Reimplemented to handle expanding with single-click mouse setting event
             * when it is clicked outside the arrow and for consistency with
             * mouseReleaseEvent() in case of decorator actions.
             */
            void mousePressEvent( QMouseEvent *event ) override;

            /**
             * Reimplemented to handle expanding with single-click mouse setting event
             * when it is clicked outside the arrow and to handle clicking on decorator
             * actions */
            void mouseReleaseEvent( QMouseEvent *event ) override;

            /**
             * Reimplemented to show proper tooltips for decorator actions.
             */
            bool viewportEvent( QEvent *event ) override;

            /**
             * Get dectorator action (little action icon as seen for example in collection
             * items in collection browser) of index @p idx under mouse position @p pos.
             */
            QAction *decoratorActionAt( const QModelIndex &idx, const QPoint &pos );

        protected Q_SLOTS:
            virtual void newPalette( const QPalette &palette );

        private:
            /**
             * Position (relative to this widget) where the mouse button was pressed to
             * trigger expand/collapse, or null pointer where expand/collapse shouldn't
             * be handled in mouseReleaseEvent()
             */
            QScopedPointer<QPoint> m_expandCollapsePressedAt;

            /**
             * Pointer to decorator action which was pressed in mousePressEvent() or null
             * pointer if no action was pressed in the most recent mouse press
             */
            QAction *m_decoratorActionPressed;
    };
}

#endif
