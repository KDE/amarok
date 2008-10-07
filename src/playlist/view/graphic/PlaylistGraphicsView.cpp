/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>
 * copyright            : (C) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
 * copyright            : (C) 2008 Seb Ruiz <ruiz@kde.org> 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

#define DEBUG_PREFIX "Playlist::GraphicsView"

#include "PlaylistGraphicsView.h"

#include "Amarok.h"
#include "App.h" // application palette
#include "Debug.h"

#include "PlaylistDropVis.h"
#include "PlaylistGraphicsItem.h"
#include "PlaylistGraphicsScene.h"
#include "SvgTinter.h"
#include "WidgetBackgroundPainter.h"
#include "meta/CurrentTrackActionsCapability.h"
#include "playlist/GroupingProxy.h"
#include "playlist/PlaylistController.h"
#include "playlist/PlaylistModel.h"

#include <KAction>
#include <KMenu>

//#include <QClipboard>
#include <QGraphicsItemAnimation>
#include <QKeyEvent>
#include <QModelIndex>
#include <QPixmapCache>
#include <QScrollBar>
#include <QTimeLine>
#include <QVariant>

#include <typeinfo>

Playlist::GraphicsView::GraphicsView( QWidget *parent )
    : QGraphicsView( parent )
{
    DEBUG_BLOCK
    setAcceptDrops( true );
    setAlignment( Qt::AlignLeft | Qt::AlignTop );
    setTransformationAnchor( QGraphicsView::AnchorUnderMouse );

    setScene( new GraphicsScene(this) );
    scene()->addItem( DropVis::instance(this) );

    setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

    //make background transparent
    QPalette p = palette();
    QColor c = p.color( QPalette::Base );
    c.setAlpha( 0 );
    p.setColor( QPalette::Base, c );
    setPalette( p );

    setBackgroundBrush ( QBrush( Qt::transparent ) );
    setAutoFillBackground ( true );

    setObjectName( "PlaylistGraphicsView" );
}

Playlist::GraphicsView::~GraphicsView()
{
    DropVis::destroy(this);
    GroupingProxy::destroy();
}

void
Playlist::GraphicsView::setModel()
{
    // TODO: merge this into the constructor, if possible
    GroupingProxy* model = GroupingProxy::instance();

    rowsInserted( QModelIndex(), 0, model->rowCount() - 1);

    connect( model, SIGNAL( modelReset() ), this, SLOT( modelReset() ) );
    connect( model, SIGNAL( rowsInserted( const QModelIndex&, int, int ) ), this, SLOT( rowsInserted( const QModelIndex &, int, int ) ) );
    connect( model, SIGNAL( rowsRemoved( const QModelIndex&, int, int ) ), this, SLOT( rowsRemoved( const QModelIndex&, int, int ) ) );
    connect( model, SIGNAL( dataChanged( const QModelIndex&, const QModelIndex& ) ), this, SLOT( dataChanged( const QModelIndex& ) ) );
    connect( model, SIGNAL( layoutChanged() ), this, SLOT( groupingChanged() ) );
}

void
Playlist::GraphicsView::dragEnterEvent( QDragEnterEvent *event )
{
    event->accept();
    foreach( const QString &mime, The::playlistModel()->mimeTypes() )
    {
        if( event->mimeData()->hasFormat( mime ) )
        {
            showDropVisEvent( event->pos() );
            
            QGraphicsView::dragEnterEvent( event );
            event->acceptProposedAction();

            return;
        }
    }
    QGraphicsView::dragEnterEvent( event );
}

void
Playlist::GraphicsView::dragMoveEvent( QDragMoveEvent *event )
{
    foreach( const QString &mime, The::playlistModel()->mimeTypes() )
    {
        if( event->mimeData()->hasFormat( mime ) )
        {
            showDropVisEvent( event->pos() );
            
            QGraphicsView::dragMoveEvent( event );
            event->acceptProposedAction();

            return;
        }
    }
    QGraphicsView::dragMoveEvent( event );
}

void
Playlist::GraphicsView::showDropVisEvent( QPoint pos )
{
    GraphicsItem* hoverItem = 0;
    QList<QGraphicsItem*> hoverItems = items( pos );
    if( !hoverItems.isEmpty() )
        hoverItem = dynamic_cast<GraphicsItem*>( hoverItems.last() );

    if( hoverItem )
    {
        bool showBelow = false;
        if( hoverItem == m_tracks.last() )
        {
            QPointF halfPos = hoverItem->mapToScene( QPointF(0,0) );
            halfPos.setY( halfPos.y() + ( hoverItem->boundingRect().height() / 2 ) );
            QPointF scenePos = mapToScene( pos );

            if( scenePos.y() >= halfPos.y() )
                showBelow = true;
        }
        DropVis::instance(this)->show( hoverItem, showBelow );
    }
    else
        DropVis::instance(this)->show();
}

void
Playlist::GraphicsView::dragLeaveEvent( QDragLeaveEvent *event )
{
    DropVis::instance(this)->hide();
    QGraphicsView::dragLeaveEvent( event );
}

void
Playlist::GraphicsView::dropEvent( QDropEvent *event )
{
    DEBUG_BLOCK
    event->accept();

    QList<QGraphicsItem*> aboveItems = items( event->pos() );
    int row;
    if( aboveItems.isEmpty() )
        row = -1;
    else
    {
        GraphicsItem *aboveItem = dynamic_cast<GraphicsItem*>( aboveItems.last() );
        
        row = m_tracks.indexOf( aboveItem );
        
        if( aboveItem == m_tracks.last() )
        {
            QPointF halfPos = aboveItem->mapToScene( QPointF(0,0) );
            halfPos.setY( halfPos.y() + ( aboveItem->boundingRect().height() / 2 ) );
            QPointF scenePos = mapToScene( event->pos() );

            if( scenePos.y() >= halfPos.y() ) // we want to drop it below
                row = -1; // append
        }
    }

    The::playlistModel()->dropMimeData( event->mimeData(), Qt::CopyAction, row, 0, QModelIndex() );
    DropVis::instance(this)->hide();
}

void
Playlist::GraphicsView::playTrack()
{
    DEBUG_BLOCK

    DEBUG_BLOCK

    QAction *playAction = dynamic_cast<QAction*>( sender() );
    if( !playAction ) {
        debug() << "Action is 0. Aborting.";
        return;
    }

    m_model->play( playAction->data().toInt() );
}

void
Playlist::GraphicsView::removeSelection()
{
    QList<QGraphicsItem*> selection = scene()->selectedItems();

    if( selection.isEmpty() )
        return; // our job here is done.

    int firstIndex = m_tracks.indexOf( static_cast<GraphicsItem*>( selection.first() ) );
    if( firstIndex > 0 )
        firstIndex -= 1;

    QList<int> removals;
    foreach( QGraphicsItem *i, selection )
        removals.append(m_tracks.indexOf( static_cast<GraphicsItem*>(i) ));

    Controller::instance()->removeRows( removals );

    for( int i = firstIndex ; i < m_tracks.count(); i++ )
        m_tracks.at( i )->setRow( i );
}

void
Playlist::GraphicsView::rowsInserted( const QModelIndex& parent, int start, int end )
{
    DEBUG_BLOCK
    Q_UNUSED( parent );

    //call setRow on track imidiately preceding the insertion as this might have to change its
    // look and height if it has been grouped by the model.
    if ( start > 0 )
        m_tracks[ start-1 ]->setRow( start-1 );

    double cumulativeHeight = 0;
    for ( int j = 0; j < start; j++ )
        cumulativeHeight += m_tracks.at( j )->boundingRect().height();

    for( int i = start; i <= end; i++ )
    {

        GraphicsItem* item = new GraphicsItem(this);
        item->setRow( i );
        item->setPos( 0.0, cumulativeHeight );
        cumulativeHeight += item->boundingRect().height();
        scene()->addItem( item );
        m_tracks.insert( i, item );
    }

    // make sure all following tracks has their colors updated correctly
    for ( int i = end + 1 ; i < m_tracks.count(); i++ )
        m_tracks.at( i )->setRow( i );

    shuffleTracks( end + 1 );
}

    void
Playlist::GraphicsView::rowsRemoved(const QModelIndex& parent, int start, int end )
{
    DEBUG_BLOCK
    Q_UNUSED( parent );

    for( int i = end; i >= start; i-- )
    {
        QGraphicsItem* item = m_tracks[i];

        // kill the animator if three is one
        if( m_animatorsByItem.contains( item ) )
        {
            QGraphicsItemAnimation* oldAnimator = m_animatorsByItem[item];
            m_animatorsByTimeline.remove( oldAnimator->timeLine(), oldAnimator );
            delete oldAnimator;
            m_animatorsByItem.remove( item );
        }

        delete m_tracks.takeAt( i );
    }

    for ( int i = start; i < m_tracks.count(); i++ )
        m_tracks.at( i )->setRow( i );


    shuffleTracks( start, -1 );
}

void
Playlist::GraphicsView::moveItem( GraphicsItem *moveMe, GraphicsItem *above )
{
    DEBUG_BLOCK
    if ( moveMe->groupMode() != Head ) {

        debug() << "normal item";
        
        int moveMeIndex = m_tracks.indexOf( moveMe );
        int aboveIndex;
        if ( above )
            aboveIndex  = m_tracks.indexOf( above  );
        else
            aboveIndex = m_tracks.count();

        //call set row on all items below the first one potentially modified to
        //make sure that all items have correct background color and group info

        int firstIndex = qMin( aboveIndex, moveMeIndex ) -1;
        if ( firstIndex < 0 ) firstIndex = 0;

        if( moveMeIndex < aboveIndex )
            Controller::instance()->moveRow( moveMeIndex, aboveIndex -1 );
        else
            Controller::instance()->moveRow( moveMeIndex, aboveIndex );
    }
    else
    {
        //we are moving a head item and all its children....

        int moveMeIndex = m_tracks.indexOf( moveMe );

        QList<int> albumTracks;
        albumTracks << moveMeIndex;

        int i = moveMeIndex + 1;

        // get the index of the body elements
        while( ( i < m_tracks.count() ) && ( m_tracks[i]->groupMode() != Head ) ) {
            albumTracks << i;
            i++;
        }
        
        int aboveIndex;
        if ( above )
            aboveIndex  = m_tracks.indexOf( above  );
        else
            aboveIndex = m_tracks.count();

        Controller::instance()->moveRows( albumTracks, aboveIndex );
    }
}

void
Playlist::GraphicsView::shuffleTracks( int startPosition, int stopPosition, bool animate )
{
    if( startPosition < 0 )
        return;

    if( stopPosition < 0 || stopPosition > m_tracks.size() )
        stopPosition = m_tracks.size();

    QTimeLine* timeline = 0;

    double cumulativeHeight = 0;

    for ( int j = 0; j < startPosition; j++ )
        cumulativeHeight += m_tracks.at( j )->boundingRect().height();

    for( int i = startPosition; i < stopPosition; ++i )
    {
        GraphicsItem *item = m_tracks.at( i );
        qreal currentY = item->pos().y();

        qreal desiredY = cumulativeHeight;

        double itemHeight = item->boundingRect().height();
        cumulativeHeight += itemHeight;

        //it turns out that not repaiting stuff that is being moved, but technically
        //already is where it needs to be, is a really bad idea.
        //if( currentY == desiredY )
        //    continue;

        // If this item already being animated, stop that animation.
        if( m_animatorsByItem.contains( item ) )
        {
            QGraphicsItemAnimation* oldAnimator = m_animatorsByItem[item];
            m_animatorsByTimeline.remove( oldAnimator->timeLine(), oldAnimator );
            delete oldAnimator;
            m_animatorsByItem.remove( item );
        }

        double visibleTop = mapToScene( 0,0 ).y();
        double visibleBottom = mapToScene( 0, height() ).y();

        // Animate the repositioning of the item if it is within the viewable area and this playlist is visible...
        if ( animate &&
            (visibleTop <= desiredY && desiredY <= visibleBottom || 
            visibleTop <= currentY && currentY <= visibleBottom) &&
            itemHeight != 0 && isVisible() )
        {
            if ( timeline == 0 )
            {
                timeline = new QTimeLine( 300 ); // 0.3 second duration
                timeline->setCurveShape( QTimeLine::EaseInCurve );
                timeline->setUpdateInterval( 30 ); // make sure that there is no leftover time
                //that results in items not moving all the way
                connect( timeline, SIGNAL( finished () ), this, SLOT( animationComplete() ) );
            }


            QGraphicsItemAnimation *animator = new QGraphicsItemAnimation;
            animator->setItem( item );
            animator->setTimeLine( timeline );
            m_animatorsByTimeline.insert( timeline, animator );
            m_animatorsByItem.insert( item, animator );

            qreal distanceMoved = qAbs( desiredY - currentY );
            bool moveUp = desiredY > currentY ? true : false;

            for( qreal i = 0; i <= distanceMoved; ++i )
            {
                qreal newY = moveUp ? ( currentY + i ) : ( currentY - i );
                animator->setPosAt( i / distanceMoved, QPointF( 0.0, newY ) );
            }
        }
        else
        {
            //don't animate items if both currentY and desiredY are outside the visible area!
            //We still do need to update their position though
            m_tracks.at( i )->setPos( 0.0, desiredY );
        }

    }

    if( timeline )
        timeline->start();

    // if there are animators, it will get updated when they are finished
    if( m_animatorsByItem.isEmpty() )
        scene()->setSceneRect( scene()->itemsBoundingRect() );
}

void
Playlist::GraphicsView::modelReset()
{
    DEBUG_BLOCK
    foreach( GraphicsItem* it, m_tracks )
    {
        delete it;
    }
    m_tracks.clear();
}

void
Playlist::GraphicsView::dataChanged(const QModelIndex & index)
{
    DEBUG_BLOCK
    if( !index.isValid() )
        return;

    if( m_tracks.count() > index.row() )
    {
        m_tracks.at( index.row() )->dataChanged();
    }
}

void
Playlist::GraphicsView::groupingChanged()
{
    DEBUG_BLOCK
    // ouch!!! this is expensive!!

    GroupingProxy* model = GroupingProxy::instance();

    int i;
    for ( i = 0; i < m_tracks.count(); i++ ) {
        if (i < model->rowCount()) {
            m_tracks.at( i )->setRow( i );
        }
    }

    shuffleTracks( 0, -1 );
}

void
Playlist::GraphicsView::animationComplete()
{
    QTimeLine* timeline = dynamic_cast<QTimeLine*>( sender() );
    if( timeline )
    {
        foreach( QGraphicsItemAnimation* animator, m_animatorsByTimeline.values( timeline ) )
        {
            m_animatorsByItem.remove( animator->item() );
            delete animator;
        }

        m_animatorsByTimeline.remove( timeline );

        timeline->deleteLater();
    }

    if( m_animatorsByItem.isEmpty() )
        scene()->setSceneRect( scene()->itemsBoundingRect() );
}

void
Playlist::GraphicsView::paletteChange(const QPalette & oldPalette)
{
    Q_UNUSED( oldPalette );

    QPalette p = palette();
    QColor c = p.color( QPalette::Base );
    c.setAlpha( 0 );
    p.setColor( QPalette::Base, c );
    setPalette( p );

    //setBackgroundBrush( App::instance()->palette().window().color() );
    foreach( GraphicsItem* it, m_tracks )
    {
        it->paletteChange();
    }
}
