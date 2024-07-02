/****************************************************************************************
 * Copyright (c) 2010 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include "OpmlDirectoryView.h"

#include "OpmlDirectoryModel.h"

#include "core/support/Debug.h"

#include <QMenu>

#include <QContextMenuEvent>

OpmlDirectoryView::OpmlDirectoryView( QWidget *parent ) :
    Amarok::PrettyTreeView(parent)
{
}

void
OpmlDirectoryView::contextMenuEvent( QContextMenuEvent *event )
{
    DEBUG_BLOCK

    QModelIndex idx = indexAt( event->pos() );

    debug() << idx;

    event->accept();

    QVariant data = model()->data( idx, OpmlDirectoryModel::ActionRole );
    QActionList actions = data.value<QActionList>();

    if( actions.isEmpty() )
    {
        warning() << "no actions for index:" << idx;
        return;
    }

    QMenu menu;
    for( QAction *action : actions )
    {
        if( action )
            menu.addAction( action );
    }

    menu.exec( mapToGlobal( event->pos() ) );

    //We keep the items that the actions need to be applied to in the actions private data.
    //Clear the data from all actions now that the context menu has executed.
    for( QAction *action : actions )
        action->setData( QVariant() );
}

void
OpmlDirectoryView::keyPressEvent( QKeyEvent *event )
{
    switch( event->key() )
    {
        case Qt::Key_Delete:
        {
            for( const QItemSelectionRange &range : selectionModel()->selection() )
                model()->removeRows( range.top(), range.height(), range.parent() );
            event->accept();
            return;
        }
     }
     Amarok::PrettyTreeView::keyPressEvent( event );
}

QItemSelectionModel::SelectionFlags
OpmlDirectoryView::selectionCommand ( const QModelIndex &index, const QEvent *event ) const
{
    if( model()->hasChildren( index ) )
        return QItemSelectionModel::ClearAndSelect;

    return Amarok::PrettyTreeView::selectionCommand( index, event );
}
