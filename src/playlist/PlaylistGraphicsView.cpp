/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#include "amarok.h"
#include "debug.h"
#include "PlaylistModel.h"
#include "PlaylistGraphicsItem.h"
#include "PlaylistGraphicsView.h"
#include "PlaylistGraphicsScene.h"
#include "PlaylistDropVis.h"
#include "TheInstances.h"

#include <KAction>
#include <KMenu>

#include <QModelIndex>
#include <QKeyEvent>
#include <QVariant>

Playlist::GraphicsView::GraphicsView( QWidget* parent, Playlist::Model* model )
    : QGraphicsView( parent )
    , m_model( model )
{
    DEBUG_BLOCK
    setScene( new Playlist::GraphicsScene() );
    scene()->addItem( Playlist::DropVis::instance() );

    rowsInserted( QModelIndex(), 0, m_model->rowCount() - 1);

    connect( m_model, SIGNAL( modelReset() ), this, SLOT( modelReset() ) );
    connect( m_model, SIGNAL( rowsInserted( const QModelIndex&, int, int ) ), this, SLOT( rowsInserted( const QModelIndex &, int, int ) ) );
    connect( m_model, SIGNAL( rowsRemoved( const QModelIndex&, int, int ) ), this, SLOT( rowsRemoved( const QModelIndex&, int, int ) ) );
    connect( m_model, SIGNAL( dataChanged( const QModelIndex&, const QModelIndex& ) ), this, SLOT( dataChanged( const QModelIndex& ) ) );
    show();
}

void
Playlist::GraphicsView::contextMenuEvent( QContextMenuEvent *event )
{
    QPointF sceneClickPos = mapToScene( event->pos() );
    QGraphicsItem *topItem = scene()->itemAt( sceneClickPos );
    if( !topItem )
        return;

    
    Playlist::GraphicsItem *item = dynamic_cast<Playlist::GraphicsItem*>( topItem );
    if( !item )
        item = dynamic_cast<Playlist::GraphicsItem*>( topItem->parentItem() );
    if( !item ) // we've clicked on empty space
        return;
           
    event->accept();

    KAction *playAction = new KAction( KIcon( Amarok::icon( "play" ) ), i18n( "&Play" ), this );
    playAction->setData( QVariant( sceneClickPos ) );
    connect( playAction, SIGNAL( triggered() ), this, SLOT( playTrack() ) );

    KMenu *menu = new KMenu( this );
    menu->addAction( playAction );
    menu->addSeparator();
    menu->addAction( i18n( "Remove From Playlist" ), this, SLOT( removeSelection() ) );
    menu->exec( event->globalPos() );
}

void
Playlist::GraphicsView::keyPressEvent( QKeyEvent* event )
{
    DEBUG_BLOCK
    debug() << "Pressed: " << event;
    if( event->matches( QKeySequence::Delete ) )
    {
        if( not scene()->selectedItems().isEmpty() )
        {
            event->accept();
            removeSelection();
            return;
        }
    }
    QGraphicsView::keyPressEvent( event );
}

void
Playlist::GraphicsView::playTrack()
{
    QAction *playAction = dynamic_cast<QAction*>( sender() );
    if( !playAction )
        return;

    QPointF sceneClickPos = playAction->data().toPointF();
    Playlist::GraphicsItem *item = dynamic_cast<Playlist::GraphicsItem*>( scene()->itemAt( sceneClickPos )->parentItem() );
    if( !item )
        return;
    item->play();
}

void
Playlist::GraphicsView::removeSelection()
{
    QList<QGraphicsItem*> selection = scene()->selectedItems();
    foreach( QGraphicsItem *i, selection )
    {
        int index = m_tracks.indexOf( static_cast<Playlist::GraphicsItem*>(i) );
        m_model->removeRows( index, 1 );
    }
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
    shuffleTracks( end );
}

void
Playlist::GraphicsView::rowsRemoved(const QModelIndex& parent, int start, int end )
{
    Q_UNUSED( parent );
    for( int i = end; i >= start; i-- )
        delete m_tracks.takeAt( i );

    shuffleTracks( start );
}

void
Playlist::GraphicsView::moveItem( Playlist::GraphicsItem *moveMe, Playlist::GraphicsItem *above )
{
    int moveMeIndex = m_tracks.indexOf( moveMe );
    int aboveIndex  = m_tracks.indexOf( above  );
    m_tracks.move( moveMeIndex, aboveIndex );
    shuffleTracks( moveMeIndex );
}

void
Playlist::GraphicsView::shuffleTracks( int startPosition )
{
    if ( startPosition < 0 )
        return;

    for( int i = startPosition; i < m_tracks.size(); ++i )
    {
        Playlist::GraphicsItem* item = m_tracks.at( i );
        item->setPos( 0.0, Playlist::GraphicsItem::height() * i );
    }
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
         m_tracks.at( index.row() )->refresh();
}

#include "PlaylistGraphicsView.moc"
