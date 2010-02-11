/****************************************************************************************
 * Copyright (c) 2009 Simon Esneault <simon.esneault@gmail.com>                         *
 *               2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "PhotosScrollWidget.h"
#include "DragPixmapItem.h"

// Amarok
#include "Debug.h"
#include "SvgHandler.h"

// QT
#include <QGraphicsItem>
#include <QGraphicsSceneHoverEvent>
#include <QList>
#include <QPixmap>
#include <QTimer>
#include <QPropertyAnimation>

#define DEBUG_PREFIX "PhotosScrollWidget"

PhotosScrollWidget::PhotosScrollWidget( QGraphicsItem* parent )
    : QGraphicsWidget( parent )
    , m_speed( 1. )
    , m_margin( 5 )
    , m_scrollmax( 0 )
    , m_actualpos( 0 )
    , m_currentPix( 0 )
    , m_lastPix( 0 )
    , m_interval( 3500 )
    , m_mode( PHOTOS_MODE_INTERACTIVE )
    , m_delta( 0 )
    , m_animation( new QPropertyAnimation( this, "animValue" ) )
{

    setAcceptHoverEvents( true );
    setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);

    // prepare the timer for the fading effect
    m_timer =  new QTimer( this );
    m_timer->setSingleShot( true );
    connect(m_timer, SIGNAL( timeout() ), this, SLOT( automaticAnimBegin() ) );

    m_animation->setEasingCurve( QEasingCurve::Linear );
    m_animation->setStartValue( 0.0 );
    m_animation->setEndValue( 1.0 );

    // connect the end of the animation
    connect( m_animation, SIGNAL(finished()), this, SLOT(automaticAnimEnd()) );
}

PhotosScrollWidget::~PhotosScrollWidget()
{
    DEBUG_BLOCK
    clear();

}


void PhotosScrollWidget::clear()
{
    DEBUG_BLOCK

    if( m_animation->state() == QAbstractAnimation::Running )
        m_animation->stop();

    // stop the timer for animation
    if ( m_timer->isActive() )
        m_timer->stop();

    //delete!!!
    debug() << "Going to delete " << m_pixmaplist.count() << " items";

    qDeleteAll( m_pixmaplist );

    m_pixmaplist.clear();
    m_currentlist.clear();
    m_scrollmax = 0;
    m_actualpos = 0;
    m_currentPix = 0;
    m_lastPix = 0;
}


void PhotosScrollWidget::setMode( int mode )
{
    DEBUG_BLOCK
    m_mode = mode;
    QList < PhotosInfo * > tmp = m_currentlist;
    clear();
    setPixmapList( tmp );
    tmp.clear();
}

void PhotosScrollWidget::setPixmapList (QList < PhotosInfo * > list)
{
    DEBUG_BLOCK
    // if the list is the same, nothing happen.
    if ( list == m_currentlist )
        return;

  //  debug() << "adding " << list.count() << "new pics";
    // If a new one arrived, we change.
    foreach( PhotosInfo *item, list )
    {
        if ( !m_currentlist.contains( item ) )
        {
            if ( !item->photo->isNull() )
            {
                switch ( m_mode )
                {
                    case PHOTOS_MODE_INTERACTIVE :
                    {
                        if( m_animation->state() == QAbstractAnimation::Running ) // careful we're animating
                            m_animation->stop();

                        DragPixmapItem *dragpix = new DragPixmapItem( this );
                        dragpix->setPixmap( The::svgHandler()->addBordersToPixmap(
                        item->photo->scaledToHeight( (int) size().height() - 4 * m_margin,  Qt::SmoothTransformation ), 5, "", true ) );
                        dragpix->setPos( m_actualpos, 0 );
                        dragpix->SetClickableUrl( item->urlpage );
                        dragpix->show();

                        m_pixmaplist << dragpix;

                        int delta = dragpix->boundingRect().width() + m_margin;
                        m_scrollmax += delta;
                        m_actualpos += delta;

                        break;
                    }
                    case PHOTOS_MODE_AUTOMATIC :
                    {
                        DragPixmapItem *dragpix = new DragPixmapItem( this );
                        dragpix->setPixmap( The::svgHandler()->addBordersToPixmap(
                            item->photo->scaledToHeight( (int) size().height() - 4 * m_margin,  Qt::SmoothTransformation ), 5, "", true ) );
                        dragpix->SetClickableUrl( item->urlpage );

                        // only pos and show if no animation, otherwise it will be set at the end automatically
                        if ( m_animation->state() != QAbstractAnimation::Running )
                        {
                            if ( ! m_pixmaplist.empty() )
                            {
                                dragpix->setPos( m_pixmaplist.last()->boundingRect().width() + m_pixmaplist.last()->pos().x() + m_margin , 0 ) ;
                                dragpix->show();
                            }
                            else
                            {
                                m_actualpos = 0;
                                dragpix->setPos( m_actualpos, 0 ) ;
                                dragpix->show();
                            }
                        }

                        m_pixmaplist << dragpix;

                        // set a timer after and launch
                        QTimer::singleShot( m_interval, this, SLOT( automaticAnimBegin() ) );

                        break;
                    }
                    case PHOTOS_MODE_FADING :
                    {

                        DragPixmapItem *dragpix = new DragPixmapItem( this );
                        dragpix->setPixmap( The::svgHandler()->addBordersToPixmap(
                        item->photo->scaledToHeight( (int) size().height() - 4 * m_margin,  Qt::SmoothTransformation ), 5, "", true ) );
                        dragpix->setPos( ( size().width() - dragpix->boundingRect().width() ) / 2, 0 );
                        dragpix->SetClickableUrl( item->urlpage );
                        dragpix->hide();
                        m_pixmaplist << dragpix;
                        if ( m_pixmaplist.size() == 1 )
                        {
                            dragpix->show();
                            m_timer->start( m_interval );
                        }

                        break;
                    }
                }
            }
        }
    }
    m_currentlist = list;
  //  debug() << "total count: " << m_pixmaplist.count();
}

void PhotosScrollWidget::hoverEnterEvent(QGraphicsSceneHoverEvent*)
{
//    DEBUG_BLOCK
    switch ( m_mode )
    {
        case PHOTOS_MODE_AUTOMATIC :
        {
            if( m_animation->state() == QAbstractAnimation::Running )
            {
                m_animation->stop();
                if ( m_currentPix != 0 )
                    m_currentPix--;
            }
            break;
        }
    }
}

void PhotosScrollWidget::hoverLeaveEvent(QGraphicsSceneHoverEvent*)
{
//    DEBUG_BLOCK
    switch ( m_mode )
    {
        case PHOTOS_MODE_INTERACTIVE :
        {
            if( m_animation->state() == QAbstractAnimation::Running )
               m_animation->stop();
            break;
        }

        case PHOTOS_MODE_AUTOMATIC :
        {
            if( m_animation->state() == QAbstractAnimation::Running )
                QTimer::singleShot( 0, this, SLOT( automaticAnimBegin() ) );
            break;
        }
    }
}

void PhotosScrollWidget::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
//    DEBUG_BLOCK
    switch ( m_mode )
    {
        case PHOTOS_MODE_INTERACTIVE :
        {
            m_speed = ( event->pos().x() - ( size().width() / 2 ) ) / size().width();
            m_speed *= 20;

            if( m_animation->state() == QAbstractAnimation::Running )
            {
                m_animation->pause();
                m_animation->setDuration( m_scrollmax*10 );
                m_animation->resume();
            } else {
                m_animation->setDuration( m_scrollmax*10 );
                m_animation->start();
            }
        }
        default:
            break;
    }
}

void PhotosScrollWidget::resize(qreal wid, qreal hei)
{

    switch( m_mode )
    {
        case PHOTOS_MODE_FADING:
        {
            foreach (DragPixmapItem *item, m_pixmaplist)
            {
                if ( !item->pixmap().isNull() )
                {
                    if ( size().height() != hei )
                        item->setPixmap( item->pixmap().scaledToHeight( (int) hei - 4 * m_margin,  Qt::SmoothTransformation ) );
                    if ( size().width() != wid )
                        item->setPos( ( wid - item->boundingRect().width() ) / 2, 0 );
                }
            }
            break;
        }
    }

    QGraphicsWidget::resize( wid, hei );
}


void
PhotosScrollWidget::automaticAnimBegin()
{
    if ( m_pixmaplist.size() > 1 && m_animation->state() != QAbstractAnimation::Running )  // only start if m_pixmaplist >= 2
    {
        m_lastPix = m_currentPix;
        m_currentPix = ( m_currentPix + 1 ) % ( m_pixmaplist.count() );

        switch( m_mode )
        {
            case PHOTOS_MODE_AUTOMATIC:
            {
                m_delta = m_pixmaplist.at( m_currentPix )->boundingRect().width() + m_margin;
                if( m_animation->state() == QAbstractAnimation::Running )
                    m_animation->stop();

                m_animation->setDuration( m_delta*20 );
                m_animation->start();
                break;
            }

            case PHOTOS_MODE_FADING:
            {
                if( m_animation->state() == QAbstractAnimation::Running )
                    m_animation->stop();

                m_animation->setDuration( 1200 );
                m_animation->start();
                break;
            }
            default:
                break;
        }
    }
}

void
PhotosScrollWidget::automaticAnimEnd()
{
    switch( m_mode )
    {
        case PHOTOS_MODE_AUTOMATIC:
        {
            // DEBUG_BLOCK

            /*if ( !m_pixmaplist.empty() && m_currentPix != 0 )
            {

                DragPixmapItem * orgCurrentPix = m_pixmaplist.at( m_currentPix );

                m_pixmaplist << m_pixmaplist.takeAt( m_lastPix );

                //update index of current pic
                m_currentPix = m_pixmaplist.indexOf( orgCurrentPix );
                m_lastPix = m_pixmaplist.count() - 1; //update to point at same pic at new position at the end of the list
            }*/

            QTimer::singleShot( m_interval, this, SLOT( automaticAnimBegin() ) );
            break;
        }
        case PHOTOS_MODE_FADING:
        {
            //   DEBUG_BLOCK;
            if ( !m_pixmaplist.empty() && m_currentPix != 0 )
            {
                m_pixmaplist.at( m_lastPix )->hide();
            }

            m_timer->start( m_interval );
            break;
        }
        default :
            break;
    }
}

qreal PhotosScrollWidget::animValue() const
{
    // Just a stub
    return m_delta;
}

void PhotosScrollWidget::animate( qreal anim )
{
    // DEBUG_BLOCK
    switch ( m_mode )
    {
        case PHOTOS_MODE_INTERACTIVE :
        {
            // If we're are near the border and still asking to go higher !
            if ( !childItems().isEmpty() && ( ( childItems().first()->pos().x() + childItems().first()->boundingRect().width() + 10 ) > boundingRect().width() ) && ( m_speed < 0 ) )
            {
                if( m_animation->state() == QAbstractAnimation::Running )
                    m_animation->stop();
                return;
            }
            // If we're are near the border and still asking to go down
            if ( !childItems().isEmpty() && ( ( childItems().last()->pos().x() - 10 ) < 0 ) && ( m_speed > 0 ) )
            {
                if( m_animation->state() == QAbstractAnimation::Running )
                    m_animation->stop();
                return;
            }

            int right = 0;
            foreach( QGraphicsItem *it, this->childItems() )
            {
                qreal x = it->pos().x() - m_speed;
                it->setPos( x, it->pos().y() );
                it->update();
                if ( x > right )
                    right = x + it->boundingRect().width() + m_margin;
            }
            m_actualpos = right;
            break;
        }
        case PHOTOS_MODE_AUTOMATIC :
        {
            if ( !m_pixmaplist.empty() ) // just for prevention, this should never appears
            {

                if ( ( m_pixmaplist.at( m_currentPix )->pos().x() ) <= ( m_margin / 2 - 1) )
                {
                    m_actualpos = m_margin / 2 - 1;
                    automaticAnimEnd();
                    return;
                }

                m_actualpos--;

                //this is not totally obvious, but we already made the number two visual image the current one,
                //so if we draw this as the first one, there will be no animation...
                int a = m_lastPix;

                int last = a - 1;
                if( last < 0 ) last = m_pixmaplist.count() - 1;
                bool first = true;
                int previousIndex = -1;

                while( true )
                {
                    int offset = m_margin;
                    if( first )
                    {
                        //we just need to move the very first image and the rest will fall in line!
                        offset += m_actualpos;
                        first = false;
                    }
                    else
                    {
                        offset += m_pixmaplist.at( previousIndex )->pos().x() + m_pixmaplist.at( previousIndex )->boundingRect().width();
                    }

                    m_pixmaplist.at( a )->setPos( offset,  m_pixmaplist.at( a )->pos().y() );
                    m_pixmaplist.at( a )->show();

                    if( a == last )
                        break;

                    previousIndex = a;
                    a = ( a + 1 ) % ( m_pixmaplist.size() );

                }
            }

            break;
        }

// setOpacity is a 4.5 method
#if QT_VERSION >= 0x040500
        case PHOTOS_MODE_FADING :
        {
            if ( !m_pixmaplist.empty() ) // just for prevention, this should never appears
            {
                m_pixmaplist.at( m_lastPix )->setOpacity( 1 - anim );
                m_pixmaplist.at( m_currentPix )->setOpacity( anim );
                m_pixmaplist.at( m_currentPix )->show();
            }

            break;
        }
#endif
    }
}

#include "PhotosScrollWidget.moc"

