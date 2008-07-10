/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>
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

#include "PlaylistGraphicsView.h"

#include "Amarok.h"
#include "App.h" // application palette
#include "Debug.h"

#include "PlaylistDropVis.h"
#include "PlaylistGraphicsItem.h"
#include "PlaylistGraphicsScene.h"
#include "PlaylistModel.h"
#include "PlaylistViewCommon.h"
#include "SvgTinter.h"
#include "WidgetBackgroundPainter.h"
#include "meta/CurrentTrackActionsCapability.h"

#include <KAction>
#include <KMenu>

#include <QGraphicsItemAnimation>
#include <QModelIndex>
#include <QKeyEvent>
#include <QPixmapCache>
#include <QTimeLine>
#include <QVariant>
#include <QScrollBar>

Playlist::GraphicsView *Playlist::GraphicsView::s_instance = 0;

Playlist::GraphicsView::GraphicsView( QWidget *parent )
    : QGraphicsView( parent )
    , m_model( 0 )
    , m_contextMenuItem( 0 )
    , m_isAnimating( false )
{
    setAcceptDrops( true );
    setAlignment( Qt::AlignLeft | Qt::AlignTop );
    setTransformationAnchor( QGraphicsView::AnchorUnderMouse );
    //setBackgroundBrush( App::instance()->palette().window().color() );

    setScene( new Playlist::GraphicsScene() );
    scene()->addItem( Playlist::DropVis::instance() );

    setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );


    //make background transparent
    QPalette p = palette();
    QColor c = p.color( QPalette::Base );
    c.setAlpha( 0 );
    p.setColor( QPalette::Base, c );
    setPalette( p );

    setBackgroundBrush ( QBrush( Qt::transparent ) );
    setAutoFillBackground ( true );

    m_timer = 0;

    setObjectName( "PlaylistGraphicsView" );
}

void
Playlist::GraphicsView::setModel( Playlist::Model *model )
{
//     DEBUG_BLOCK

    m_model = model;

    rowsInserted( QModelIndex(), 0, m_model->rowCount() - 1);

    connect( m_model, SIGNAL( modelReset() ), this, SLOT( modelReset() ) );
    connect( m_model, SIGNAL( rowsInserted( const QModelIndex&, int, int ) ), this, SLOT( rowsInserted( const QModelIndex &, int, int ) ) );
    connect( m_model, SIGNAL( rowsRemoved( const QModelIndex&, int, int ) ), this, SLOT( rowsRemoved( const QModelIndex&, int, int ) ) );
    connect( m_model, SIGNAL( dataChanged( const QModelIndex&, const QModelIndex& ) ), this, SLOT( dataChanged( const QModelIndex& ) ) );
    connect( m_model, SIGNAL( playlistGroupingChanged( ) ), this, SLOT( groupingChanged() ) );
    connect( m_model, SIGNAL( rowsChanged( int ) ), this, SLOT( rowsChanged( int ) ) );

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

    item->setSelected( true );
    event->accept();

    m_contextMenuItem = item;

    QPointF itemClickPos = item->mapFromScene( sceneClickPos );
    int row = m_tracks.indexOf( static_cast<Playlist::GraphicsItem*>(item) );
    const QModelIndex index = The::playlistModel()->index( row, 0 );
    ViewCommon::trackMenu(this, &index ,event->globalPos(), item->groupMode() < Playlist::Body && item->imageLocation().contains( itemClickPos ));

}

void
Playlist::GraphicsView::showItemImage()
{
    if( !m_contextMenuItem )
        return;
    m_contextMenuItem->showImage();
    m_contextMenuItem = 0;
}

void
Playlist::GraphicsView::fetchItemImage()
{
    if( !m_contextMenuItem )
        return;
    m_contextMenuItem->fetchImage();
    m_contextMenuItem = 0;
}

void
Playlist::GraphicsView::unsetItemImage()
{
    if( !m_contextMenuItem )
        return;
    m_contextMenuItem->unsetImage();
    m_contextMenuItem = 0;
}

void
Playlist::GraphicsView::editTrackInformation()
{
    if( !m_contextMenuItem )
    {
        return;
    }
    m_contextMenuItem->editTrackInformation();
    m_contextMenuItem = 0;
}

void
Playlist::GraphicsView::dragEnterEvent( QDragEnterEvent *event )
{
//     DEBUG_BLOCK
    event->accept();
    foreach( const QString &mime, The::playlistModel()->mimeTypes() )
    {
        if( event->mimeData()->hasFormat( mime ) )
        {
            QGraphicsView::dragEnterEvent( event );
            event->acceptProposedAction();
            Playlist::DropVis::instance()->show( event->pos().y() );
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
            QGraphicsView::dragMoveEvent( event );
            Playlist::DropVis::instance()->show( event->pos().y() );
            event->acceptProposedAction();
            return;
        }
    }
    QGraphicsView::dragMoveEvent( event );
}
void
Playlist::GraphicsView::dragLeaveEvent( QDragLeaveEvent *event )
{
    Playlist::DropVis::instance()->hide();
    QGraphicsView::dragLeaveEvent( event );
}

void
Playlist::GraphicsView::dropEvent( QDropEvent *event )
{
    DEBUG_BLOCK
    event->accept();
    The::playlistModel()->dropMimeData( event->mimeData(), Qt::CopyAction, -1, 0, QModelIndex() );
    Playlist::DropVis::instance()->hide();
}

void
Playlist::GraphicsView::keyPressEvent( QKeyEvent* event )
{
    if( event->matches( QKeySequence::Delete ) )
    {
        if( !scene()->selectedItems().isEmpty() )
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

    int firstIndex = m_tracks.indexOf( static_cast<Playlist::GraphicsItem*>( selection.first() ) );
    if ( firstIndex > 0) firstIndex -= 1;

    foreach( QGraphicsItem *i, selection )
    {
        int count = 1;
        int index = m_tracks.indexOf( static_cast<Playlist::GraphicsItem*>(i) );
        QModelIndex modelIndex = The::playlistModel()->index( index, 0 );
        QModelIndex nextIndex = The::playlistModel()->index( index + 1 , 0 );
        if( modelIndex.data( GroupRole ).toInt() == Head && nextIndex.data( GroupRole ).toInt() != Head ) // If a selected item is the head of a group, and the item after it is not the head of a group, then we remove all of the group.
        {
            QModelIndex in = modelIndex;
            int i = index;
            while( in.data( GroupRole ).toInt() != End )
            {
                 ++count;
                 in = The::playlistModel()->index( i++, 0 );
            }
        }
        count = modelIndex.data( GroupRole ).toInt() == Head ? count - 1 : count;
        m_model->removeRows( index, count );
    }

    for ( int i = firstIndex ; i < m_tracks.count(); i++ )
        m_tracks.at( i )->setRow( i );
}

void
Playlist::GraphicsView::rowsInserted( const QModelIndex& parent, int start, int end )
{
    Q_UNUSED( parent );

     //call setRow on track imidiately preceding the insertion as this might have to change its
    // look and height if it has been grouped by the model.
    if ( start > 0 )
        m_tracks[ start-1 ]->setRow( start-1 );

    double cumulativeHeight = 0;
    for ( int j = 0; j < start; j++ )
        cumulativeHeight += m_tracks.at( j )->boundingRect().height();

//     debug() << "start: " << start << " ,end: " << end;
    for( int i = start; i <= end; i++ )
    {

        Playlist::GraphicsItem* item = new Playlist::GraphicsItem();
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
        delete m_tracks.takeAt( i );

    // make sure all following tracks has their colors updated correctly
    for ( int i = start; i < m_tracks.count(); i++ )
        m_tracks.at( i )->setRow( i );

    shuffleTracks( start, -1, false );
    scene()->setSceneRect( scene()->itemsBoundingRect() );

    //update();
}

void
Playlist::GraphicsView::moveItem( Playlist::GraphicsItem *moveMe, Playlist::GraphicsItem *above )
{
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
    {
        m_model->moveRow( moveMeIndex, aboveIndex -1 );
        m_tracks.move( moveMeIndex, aboveIndex - 1 );


        int i;
        for ( i = firstIndex; i < m_tracks.count(); i++ )
            m_tracks.at( i )->setRow( i );


        //shuffleTracks( moveMeIndex, aboveIndex );
        shuffleTracks( 0 );
    }
    else
    {
        m_model->moveRow( moveMeIndex, aboveIndex );
        m_tracks.move( moveMeIndex, aboveIndex );

//         debug() << "First index: " << firstIndex;

        for ( int i = firstIndex; i < m_tracks.count(); i++ )
            m_tracks.at( i )->setRow( i );

        //shuffleTracks( aboveIndex, moveMeIndex + 1);
        shuffleTracks( 0 );
    }

}


void
Playlist::GraphicsView::shuffleTracks( int startPosition, int stopPosition, bool animate )
{
     DEBUG_BLOCK

     debug() << "number if items: " << m_tracks.count();
    if( startPosition < 0 )
        return;

    if( stopPosition < 0 || stopPosition > m_tracks.size() )
        stopPosition = m_tracks.size();

    if ( m_timer == 0 ) {
        m_timer = new QTimeLine( 300 ); // 0.3 second duration
        m_timer->setCurveShape( QTimeLine::EaseInCurve );
        m_timer->setUpdateInterval( 30 ); // make sure that there is no leftover time
                                    //that results in items not moving all the way
        connect( m_timer, SIGNAL( finished () ), this, SLOT( animationComplete() ) );

    }

    double cumulativeHeight = 0;

    for ( int j = 0; j < startPosition; j++ )
        cumulativeHeight += m_tracks.at( j )->boundingRect().height();

    for( int i = startPosition; i < stopPosition; ++i )
    {
        Playlist::GraphicsItem *item = m_tracks.at( i );
        qreal currentY = item->pos().y();

        qreal desiredY = cumulativeHeight;

        double itemHeight = item->boundingRect().height();
        cumulativeHeight += itemHeight;

        double visibleTop = mapToScene( 0,0 ).y();
        double visibleBottom = mapToScene( 0, height() ).y();

        // Animate the repositioning of the item if it is within the viewable area and this playlist is visible...
       if ( animate && !( ( desiredY < visibleTop ) || ( desiredY > visibleBottom ) ) &&
              ( ( currentY >= visibleTop ) && ( currentY <= visibleBottom ) ) &&
               ( itemHeight != 0 ) && isVisible() )
        {

            m_isAnimating = true;
            bool moveUp = false;
            if( desiredY > currentY )
                moveUp = true;

            qreal distanceMoved = moveUp ? ( desiredY - currentY ) : ( currentY - desiredY );

            QGraphicsItemAnimation *animator = new QGraphicsItemAnimation;
            animator->setItem( item );
            animator->setTimeLine( m_timer );

            // if distanceMoved is negative, then we are moving the object towards the bottom of the screen
            for( qreal i = 0; i < distanceMoved; ++i )
            {
                qreal newY = moveUp ? ( currentY + i ) : ( currentY - i );
                animator->setPosAt( i / distanceMoved, QPointF( 0.0, newY ) );
            }
            animator->setPosAt( 1, QPointF( 0.0, desiredY ) );

            m_animators.append( animator );
        }
        else
        {
            //don't animate items if both currentY and desiredY are outside the visible area!
            //We still do need to update their position though
            m_tracks.at( i )->setPos( 0.0, desiredY );
        }

    }
    m_timer->start();
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
     if( !index.isValid() )
        return;

     if( m_tracks.count() > index.row() )
     {
        m_tracks.at( index.row() )->dataChanged();
     }
}

void Playlist::GraphicsView::groupingChanged()
{
    // ouch!!! this is expensive!!
    DEBUG_BLOCK

    int i;
    for ( i = 0; i < m_tracks.count(); i++ )
        m_tracks.at( i )->setRow( i );


    shuffleTracks( 0, -1, false );
    //update();
}

void Playlist::GraphicsView::animationComplete()
{
    while( !m_animators.empty () ) {
        delete m_animators.takeFirst();
    }
    m_isAnimating = false;
}

void Playlist::GraphicsView::rowsChanged( int start )
{
    DEBUG_BLOCK
    for ( int i = start; i < m_tracks.count(); i++ )
        m_tracks.at( i )->setRow( i );
}

void Playlist::GraphicsView::paletteChange(const QPalette & oldPalette)
{
    Q_UNUSED( oldPalette );

    QPalette p = palette();
    QColor c = p.color( QPalette::Base );
    c.setAlpha( 0 );
    p.setColor( QPalette::Base, c );
    setPalette( p );

    //setBackgroundBrush( App::instance()->palette().window().color() );
    foreach( Playlist::GraphicsItem* it, m_tracks )
    {
        it->paletteChange();
    }
}

namespace The {
    Playlist::GraphicsView* playlistView()
    {
        if( !Playlist::GraphicsView::s_instance )
            Playlist::GraphicsView::s_instance = new Playlist::GraphicsView(Amarok::mainWindow());
        return Playlist::GraphicsView::s_instance;
    }
}

#include "PlaylistGraphicsView.moc"


