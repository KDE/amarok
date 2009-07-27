/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#include "CollectionBrowserTreeView.h"
#include "CollectionTreeItemDelegate.h"

#include "Debug.h"

#include <QMouseEvent>

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
    QAction *action = CollectionTreeItemDelegate::actionUnderPoint( event->pos() );
    if( !action )
        CollectionTreeView::mousePressEvent( event );
}

void
CollectionBrowserTreeView::mouseReleaseEvent( QMouseEvent *event )
{
    const QModelIndex index = indexAt( event->pos() );

    if( index.parent().isValid() ) // not a root element, don't bother checking actions
    {
        CollectionTreeView::mouseReleaseEvent( event );
        return;
    }

    QAction *action = CollectionTreeItemDelegate::actionUnderPoint( event->pos() );
    if( action )
        action->trigger();
    else
        CollectionTreeView::mouseReleaseEvent( event );
}
