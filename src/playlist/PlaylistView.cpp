/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#include "PlaylistDelegate.h"
#include "PlaylistView.h"

#include <QItemDelegate>
#include <QKeyEvent>
#include <QKeySequence>
#include <QModelIndex>

using namespace PlaylistNS;

void
View::setModel( QAbstractItemModel * model )
{
    QListView::setModel( model );
    setAcceptDrops( true );
    setAlternatingRowColors( true );
    setDragDropMode( QAbstractItemView::DragDrop );
    setDragDropOverwriteMode( true );
    setDragEnabled( true );
    setDropIndicatorShown( true );
    setDropIndicatorShown( true );
    setSelectionBehavior( QAbstractItemView::SelectRows );
    setSelectionMode( QAbstractItemView::ExtendedSelection );
    setSpacing( 2 );
    //setMovement( QListView::Free );
    delete itemDelegate();
    setItemDelegate( new Delegate( this ) );
    connect( this, SIGNAL( activated( const QModelIndex& ) ), model, SLOT( play( const QModelIndex& ) ) );
}

void
View::keyPressEvent( QKeyEvent* event )
{
    if( event->matches( QKeySequence::Delete ) )
    {
        if( selectionModel()->hasSelection() )
        {
            event->accept();
            QItemSelection selection = selectionModel()->selection();
            QItemSelectionRange it;
            foreach( it, selection )
                model()->removeRows( it.top(), it.height() );
            return;
        }
    }
    QListView::keyPressEvent( event );
}
#include "PlaylistView.moc"
