/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#define DEBUG_PREFIX "CollectionBrowserTreeView"
#include "core/support/Debug.h"

#include "CollectionBrowserTreeView.h"
#include "CollectionTreeItemDelegate.h"
#include "browsers/CollectionTreeItem.h"

#include <QAction>
#include <QMouseEvent>
#include <QToolTip>

Q_DECLARE_METATYPE( QAction* )
Q_DECLARE_METATYPE( QList<QAction*> )

CollectionBrowserTreeView::CollectionBrowserTreeView( QWidget *parent )
    : CollectionTreeView( parent )
{
}

CollectionBrowserTreeView::~CollectionBrowserTreeView()
{
}

void
CollectionBrowserTreeView::mouseMoveEvent( QMouseEvent *event )
{
    CollectionTreeView::mouseMoveEvent( event );

    const QModelIndex index = indexAt( event->pos() );
    if( index.parent().isValid() ) // not a root element
        return;

    // Make sure we repaint the item for the collection action buttons
    update( index );
}

void
CollectionBrowserTreeView::mousePressEvent( QMouseEvent *event )
{
    const QModelIndex index = indexAt( event->pos() );

    if( index.parent().isValid() ) // not a root element, don't bother checking actions
    {
        CollectionTreeView::mousePressEvent( event );
        return;
    }

    // Only forward the press event if we aren't on an action (which gets triggered on a release)
    const int actionsCount = index.data( CustomRoles::DecoratorRoleCount ).toInt();
    if( actionsCount > 0 )
    {
        const QRect rect = CollectionTreeItemDelegate::decoratorRect( index );
        if( rect.contains( event->pos() ) )
            return;
    }
    CollectionTreeView::mousePressEvent( event );
}

void
CollectionBrowserTreeView::mouseReleaseEvent( QMouseEvent *event )
{
    const QModelIndex index = indexAt( event->pos() );

    if( !index.parent().isValid() )
    {
        QAction *decoratorAction = decoratorActionAt( index, event->pos() );
        if( decoratorAction )
        {
            decoratorAction->trigger();
            return;
        }
    }

    CollectionTreeView::mouseReleaseEvent( event );
}

bool
CollectionBrowserTreeView::viewportEvent( QEvent *event )
{
    if( event->type() == QEvent::ToolTip )
    {
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>( event );
        const QModelIndex &index = indexAt( helpEvent->pos() );
        if( !rootIsDecorated() && !index.parent().isValid() )
        {
            QAction *action = decoratorActionAt( index, helpEvent->pos() );
            if( action )
            {
                QToolTip::showText( helpEvent->globalPos(), action->toolTip() );
                return true;
            }
        }
    }

    return QAbstractItemView::viewportEvent( event );
}

QAction *
CollectionBrowserTreeView::decoratorActionAt( const QModelIndex &idx, const QPoint pos )
{
        const int actionsCount = idx.data( CustomRoles::DecoratorRoleCount ).toInt();
    if( actionsCount > 0 )
    {
        const QRect &rect = CollectionTreeItemDelegate::decoratorRect( idx );
        if( rect.contains( pos ) )
        {
            QActionList actions = idx.data( CustomRoles::DecoratorRole ).value<QActionList>();
            if( actions.isEmpty() )
                return 0;

            //HACK: rect height == the width of one action's area.
            int indexOfAction = ( pos.x() - rect.left() ) / rect.height();
            if( indexOfAction >= actions.count() )
                return 0;
            return actions.value( indexOfAction );
        }
    }
    return 0;
}
