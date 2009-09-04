/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#include "ServiceCollectionTreeView.h"

#include "browsers/CollectionTreeItem.h"
#include "Debug.h"
#include "../meta/capabilities/CustomActionsCapability.h"

#include <KMenu>

#include <QAction>
#include <QContextMenuEvent>



ServiceCollectionTreeView::ServiceCollectionTreeView( QWidget *parent )
    : CollectionTreeView( parent )
    , m_playableTracks( true ) //per default, act just like a normal CollectionTreeView
{
    setVerticalScrollMode( QAbstractItemView::ScrollPerPixel ); // Enable smooth scrolling 
    setHorizontalScrollMode( QAbstractItemView::ScrollPerPixel ); // Enable smooth scrolling 
}

ServiceCollectionTreeView::~ServiceCollectionTreeView()
{}

void
ServiceCollectionTreeView::mouseDoubleClickEvent( QMouseEvent* event )
{
    if ( m_playableTracks )
        CollectionTreeView::mouseDoubleClickEvent( event );
}

void
ServiceCollectionTreeView::contextMenuEvent( QContextMenuEvent * event )
{
    if ( m_playableTracks )
        CollectionTreeView::contextMenuEvent( event );
    else
    {
        QModelIndexList indices = selectedIndexes();
        if( filterModel() )
        {
            QModelIndexList tmp;
            foreach( const QModelIndex &idx, indices )
            {
                tmp.append( filterModel()->mapToSource( idx ) );
            }
            indices = tmp;
        }

        if( !indices.isEmpty() )
        {
            KMenu menu;
            if( indices.count() == 1 )
            {
                if( indices.first().isValid() && indices.first().internalPointer() )
                {
                    Meta::DataPtr data = static_cast<CollectionTreeItem*>( indices.first().internalPointer() )->data();
                    if( data )
                    {
                        Meta::CustomActionsCapability *cac = data->create<Meta::CustomActionsCapability>();
                        if( cac )
                        {
                            QList<QAction*> actions = cac->customActions();
                            if( actions.count() )
                                menu.addSeparator();
                            foreach( QAction *action, actions )
                                menu.addAction( action );
                            delete cac;
                        }
                    }
                }
            }

            if( menu.actions().count() > 0 )
            {
                (void)menu.exec( event->globalPos() );
                QSet<CollectionTreeItem*> items;
                foreach( const QModelIndex &index, indices )
                {
                    if( index.isValid() && index.internalPointer() )
                        items.insert( static_cast<CollectionTreeItem*>( index.internalPointer() ) );
                }
            }
        }
        else
            debug() << "invalid index or null internalPointer";
    }
}

bool
ServiceCollectionTreeView::playableTracks() const
{
    return m_playableTracks;
}


void
ServiceCollectionTreeView::setPlayableTracks( bool playable )
{
    m_playableTracks = playable;
}

