/****************************************************************************************
 * Copyright (c) 2008-2010 Soren Harward <stharward@gmail.com>                          *
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

#define DEBUG_PREFIX "TrackSelectWidget"

#include "TrackSelectWidget.h"
#include "browsers/CollectionTreeItem.h"
#include "browsers/CollectionTreeItemModel.h"
#include "browsers/CollectionTreeViewSimple.h"
#include "browsers/collectionbrowser/CollectionTreeItemDelegate.h"
#include "core/support/Debug.h"

#include <KVBox>

#include <QList>

TrackSelectWidget::TrackSelectWidget( QWidget* parent = 0 ) : KVBox( parent )
{
    DEBUG_BLOCK
    m_view = new CollectionTreeViewSimple( this );
    m_view->setRootIsDecorated( false );
    m_view->setFrameShape( QFrame::NoFrame );

    CollectionTreeItemDelegate *delegate = new CollectionTreeItemDelegate( m_view );
    m_view->setItemDelegate( delegate );

    // TODO: use the same levels that are shown in the collection browser?
    QList<int> levels;
    levels << CategoryId::Artist << CategoryId::Album;
    m_model = new CollectionTreeItemModel( levels );
    m_view->setModel( m_model );

    connect( m_view->selectionModel(), SIGNAL( currentChanged( const QModelIndex&, const QModelIndex& ) ),
             this, SLOT( recvNewSelection( const QModelIndex& ) ) );
}

TrackSelectWidget::~TrackSelectWidget() {}

void
TrackSelectWidget::recvNewSelection( const QModelIndex& idx )
{
    if ( !idx.isValid() )
        return;

    CollectionTreeItem *item = static_cast<CollectionTreeItem*>( idx.internalPointer() );

    if ( item && item->isDataItem() ) {
        Meta::DataPtr data = item->data();
        if ( data != Meta::DataPtr() ) {
            emit selectionChanged( data );
        }
    }
}
