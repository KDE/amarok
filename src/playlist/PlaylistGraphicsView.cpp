/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#include "debug.h"
#include "PlaylistModel.h"
#include "PlaylistGraphicsItem.h"
#include "PlaylistGraphicsView.h"

#include <QModelIndex>
#include <QGraphicsScene>

Playlist::GraphicsView::GraphicsView( QWidget* parent, Playlist::Model* model )
    : QGraphicsView( parent )
    , m_model( model )
{
    DEBUG_BLOCK
    setScene( new QGraphicsScene() );
    rowsInserted( QModelIndex(), 0, m_model->rowCount() - 1);
    connect( m_model, SIGNAL( modelReset() ), this, SLOT( modelReset() ) );
    connect( m_model, SIGNAL( rowsInserted( const QModelIndex&, int, int ) ), this, SLOT( rowsInserted( const QModelIndex &, int, int ) ) );
    connect( m_model, SIGNAL( rowsRemoved( const QModelIndex&, int, int ) ), this, SLOT( rowsRemoved( const QModelIndex&, int, int ) ) );
    connect( m_model, SIGNAL( dataChanged( const QModelIndex&, const QModelIndex& ) ), this, SLOT( dataChanged( const QModelIndex& ) ) );
    show();
}

void
Playlist::GraphicsView::rowsInserted( const QModelIndex& parent, int start, int end )
{
    Q_UNUSED( parent );
    for( int i = start; i <= end; i++ )
    {
        Playlist::GraphicsItem* item = new Playlist::GraphicsItem();
        item->setPos( 0.0, Playlist::GraphicsItem::height() * i );
        scene()->addItem( item );
        m_tracks.insert( i, item  );
    }
}

void
Playlist::GraphicsView::rowsRemoved(const QModelIndex& parent, int start, int end )
{
    Q_UNUSED( parent );
    for( int i = end; i >= start; i-- )
        delete m_tracks.takeAt( i );
}

void 
Playlist::GraphicsView::modelReset()
{
    foreach( Playlist::GraphicsItem* it, m_tracks )
    {
        delete it;
    }
    m_tracks.clear();
}


void 
Playlist::GraphicsView::dataChanged(const QModelIndex & index)
{
     DEBUG_BLOCK
     if ( !index.isValid() )
        return;
   
     if ( m_tracks.count() > index.row() )
         m_tracks[ index.row() ]->refresh();
}

#include "PlaylistGraphicsView.moc"
