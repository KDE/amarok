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

#include "CollectionBrowserTreeView.h"
#include "browsers/CollectionTreeItem.h"
#include "widgets/PrettyTreeDelegate.h"
#include "widgets/PrettyTreeRoles.h"

#include <QAction>
#include <QMouseEvent>
#include <QToolTip>

Q_DECLARE_METATYPE( QAction* )
Q_DECLARE_METATYPE( QList<QAction*> )

CollectionBrowserTreeView::CollectionBrowserTreeView( QWidget *parent )
    : CollectionTreeView( parent )
{
    setMouseTracking( true ); // we want to highlight some icons if the mouse moves over it.
}

CollectionBrowserTreeView::~CollectionBrowserTreeView()
{
}

void
CollectionBrowserTreeView::mouseMoveEvent( QMouseEvent *event )
{
    CollectionTreeView::mouseMoveEvent( event );

    // Make sure we repaint the item for the collection action buttons
    const QModelIndex index = indexAt( event->pos() );
    const int actionsCount = index.data( PrettyTreeRoles::DecoratorRoleCount ).toInt();
    if( actionsCount )
        update( index );
}

void
CollectionBrowserTreeView::mousePressEvent( QMouseEvent *event )
{
    const QModelIndex index = indexAt( event->pos() );

    // Only forward the press event if we aren't on an action (which gets triggered on a release)
    if( event->button() == Qt::LeftButton &&
        event->modifiers() == Qt::NoModifier &&
        decoratorActionAt( index, event->pos() ) )
        return;

    CollectionTreeView::mousePressEvent( event );
}

void
CollectionBrowserTreeView::mouseReleaseEvent( QMouseEvent *event )
{
    const QModelIndex index = indexAt( event->pos() );

    QAction *decoratorAction = decoratorActionAt( index, event->pos() );
    if( event->button() == Qt::LeftButton &&
        event->modifiers() == Qt::NoModifier &&
        decoratorAction )
    {
        decoratorAction->trigger();
        return;
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
CollectionBrowserTreeView::decoratorActionAt( const QModelIndex &index, const QPoint pos )
{
    const int actionsCount = index.data( PrettyTreeRoles::DecoratorRoleCount ).toInt();
    if( actionsCount <= 0 )
        return 0;

    PrettyTreeDelegate* ptd = qobject_cast<PrettyTreeDelegate*>( itemDelegate( index ) );
    if( !ptd )
        return 0;

    QActionList actions = index.data( PrettyTreeRoles::DecoratorRole ).value<QActionList>();
    QRect rect = visualRect( index );

    for( int i = 0; i < actions.count(); i++ )
        if( ptd->decoratorRect( rect, i ).contains( pos ) )
            return actions.at( i );

    return 0;
}
