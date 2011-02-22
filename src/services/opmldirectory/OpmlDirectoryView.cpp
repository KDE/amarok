#include "OpmlDirectoryView.h"

#include "OpmlDirectoryModel.h"

#include "core/support/Debug.h"

#include <KMenu>

#include <QContextMenuEvent>

OpmlDirectoryView::OpmlDirectoryView( QWidget *parent ) :
    Amarok::PrettyTreeView(parent)
{
}

void
OpmlDirectoryView::contextMenuEvent( QContextMenuEvent *event )
{
    QModelIndex idx = indexAt( event->pos() );

    debug() << idx;

    event->accept();

    QVariant data = model()->data( idx, OpmlDirectoryModel::ActionRole );
    QActionList actions = data.value<QActionList>();

    if( actions.isEmpty() )
    {
        return;
    }

    KMenu menu;
    foreach( QAction *action, actions )
    {
        if( action )
            menu.addAction( action );
    }

    menu.exec( mapToGlobal( event->pos() ) );

    //We keep the items that the actions need to be applied to in the actions private data.
    //Clear the data from all actions now that the context menu has executed.
    foreach( QAction *action, actions )
        action->setData( QVariant() );
}

void
OpmlDirectoryView::keyPressEvent( QKeyEvent *event )
{
    switch( event->key() )
    {
        case Qt::Key_Delete:
        {
            foreach( QItemSelectionRange range, selectionModel()->selection() )
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
