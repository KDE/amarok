/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#include "PlaylistDelegate.h"
#include "PlaylistModel.h"
#include "PlaylistView.h"

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QItemDelegate>
#include <QKeyEvent>
#include <QKeySequence>
#include <QModelIndex>
#include <QMouseEvent>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QScrollBar>
#include <QTimerEvent>

#include <KApplication>

using namespace PlaylistNS;

View::View( QWidget* parent )
  : QListView( parent )
{
    new Animator( this );
}

View::~View()
{
    delete Animator::instance();
}

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

void 
View::mouseDoubleClickEvent(QMouseEvent *event)
{
//method based on QGraphicsView::mouseDoubleClickEvent, (c) 2007 Trolltech ASA, GPL v2
    QModelIndex index = indexAt( event->pos() );
    QGraphicsScene* scene = index.data( ItemRole ).value< PlaylistNS::Item* >()->scene();

    QPoint pointInt = event->pos();
    pointInt.rx() += horizontalScrollBar()->value();
    pointInt.ry() += verticalScrollBar()->value();
    QRect viewportRect = visualRect( index );
    pointInt.rx() -= viewportRect.x();
    pointInt.ry() -= viewportRect.y();
    QPointF pointScene = pointInt;


/*    d->storeMouseEvent(event);
    d->mousePressViewPoint = event->pos();
    d->mousePressScenePoint = mapToScene(d->mousePressViewPoint);
    d->mousePressScreenPoint = event->globalPos();
    d->lastMouseMoveScenePoint = d->mousePressScenePoint;
    d->lastMouseMoveScreenPoint = d->mousePressScreenPoint;
    d->mousePressButton = event->button(); */

    QGraphicsSceneMouseEvent mouseEvent(QEvent::GraphicsSceneMouseDoubleClick);
    mouseEvent.setWidget( viewport() );
    mouseEvent.setButtonDownScenePos(event->button(), pointScene);
    mouseEvent.setButtonDownScreenPos( event->button(), event->globalPos() );
    mouseEvent.setScenePos( pointScene );
    mouseEvent.setScreenPos( event->globalPos() );
    mouseEvent.setLastScenePos( pointScene );
    mouseEvent.setLastScreenPos(  event->globalPos() );
    mouseEvent.setButtons( event->buttons() );
    mouseEvent.setButtons( event->buttons() );
    mouseEvent.setAccepted( false );
    mouseEvent.setButton( event->button() );
    mouseEvent.setModifiers( event->modifiers() ); 
    KApplication::sendEvent( scene, &mouseEvent );
    Animator::instance()->startAnimation( index );
}
///////////////////////////////////////////////////////////////////////////////
///Animator
///////////////////////////////////////////////////////////////////////////////

Animator* PlaylistNS::Animator::s_instance = 0;

void
PlaylistNS::Animator::startAnimation( const QModelIndex& animatedRow )
{
    m_view->update( animatedRow );
    m_rows.append( new QPersistentModelIndex( animatedRow ) );
    if( not m_timer.isActive() )
        m_timer.start( 40, this );
}

void
PlaylistNS::Animator::stopAnimation( const QModelIndex& dullRow )
{
    for( int i = 0; i < m_rows.size(); i++ )
        if( *( m_rows.at( i ) ) == dullRow )
            delete m_rows.takeAt( i );
    if( m_rows.isEmpty() )
        m_timer.stop();
}

void
PlaylistNS::Animator::timerEvent( QTimerEvent *event )
{
    if( event->timerId() == m_timer.timerId() )
        foreach( QPersistentModelIndex* index, m_rows )
        {
            if( index->isValid() )
                m_view->update( *index );
        }
    else
        QObject::timerEvent( event ); //probably not needed
}

#include "PlaylistView.moc"
