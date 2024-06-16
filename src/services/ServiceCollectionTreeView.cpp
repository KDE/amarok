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

#include "ServiceCollectionTreeView.h"

#include "browsers/CollectionTreeItem.h"
#include "core/capabilities/ActionsCapability.h"
#include "core/meta/Meta.h"
#include "core/support/Debug.h"

#include <QMenu>

#include <QAction>
#include <QContextMenuEvent>
#include <QSortFilterProxyModel>

ServiceCollectionTreeView::ServiceCollectionTreeView( QWidget *parent )
    : CollectionTreeView( parent )
    , m_playableTracks( true ) //per default, act just like a normal CollectionTreeView
{
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
            for( const QModelIndex &idx : indices )
            {
                tmp.append( filterModel()->mapToSource( idx ) );
            }
            indices = tmp;
        }

        if( !indices.isEmpty() )
        {
            QMenu menu;
            if( indices.count() == 1 )
            {
                if( indices.first().isValid() && indices.first().internalPointer() )
                {
                    Meta::DataPtr data = static_cast<CollectionTreeItem*>( indices.first().internalPointer() )->data();
                    if( data )
                    {
                        QScopedPointer< Capabilities::ActionsCapability > ac( data->create<Capabilities::ActionsCapability>() );
                        if( ac )
                        {
                            QList<QAction*> actions = ac->actions();
                            if( !actions.isEmpty() )
                                menu.addSeparator();
                            for( QAction *action : actions )
                            {
                                if( !action->parent() )
                                    action->setParent( &menu );
                                menu.addAction( action );
                            }
                        }
                    }
                }
            }

            if( menu.actions().count() > 0 )
            {
                (void)menu.exec( event->globalPos() );
                QSet<CollectionTreeItem*> items;
                for( const QModelIndex &index : indices )
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

