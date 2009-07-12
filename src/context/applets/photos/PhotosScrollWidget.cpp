/****************************************************************************************
 * Copyright (c) 2009 Simon Esneault <simon.esneault@gmail.com>                         *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "PhotosScrollWidget.h"
#include "DragPixmapItem.h"

// Amarok
#include "Debug.h"
#include "SvgHandler.h"

// KDE
#include <Plasma/Animator>

// QT
#include <QGraphicsPixmapItem>
#include <QGraphicsItem>
#include <QGraphicsSceneHoverEvent>
#include <QList>
#include <QPixmap>
#include <QTimer>

#define DEBUG_PREFIX "PhotosScrollWidget"

PhotosScrollWidget::PhotosScrollWidget( QGraphicsItem* parent )
    : QGraphicsWidget( parent )
    , m_id( 0 )
    , m_animating( false )
    , m_speed( 1. )
    , m_margin( 5 )
    , m_scrollmax( 0 )
    , m_actualpos( 0 )
    , m_currentPix( 0 )
    , m_timer( 1800 )
    , m_mode( PHOTOS_MODE_INTERACTIVE )
    , m_delta( 0 )
{
    setAcceptHoverEvents( true );
    setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);
}

PhotosScrollWidget::~PhotosScrollWidget()
{
    DEBUG_BLOCK
    clear();
}


void PhotosScrollWidget::clear()
{
    if ( !m_id )
    {
        Plasma::Animator::self()->stopCustomAnimation( m_id );
        m_id = 0;
    }
    
    foreach( QGraphicsItem *it, this->childItems() )
        delete it;

    m_pixmaplist.clear();
    m_currentlist.clear();
    m_scrollmax = 0;
    m_actualpos = 0;
    m_currentPix = 0;
}


void PhotosScrollWidget::setMode( int mode )
{
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

    // If a new one arrived, we change.
    foreach( PhotosInfo *item, list )
    {
        switch ( m_mode )
        {
            case PHOTOS_MODE_INTERACTIVE :
            {
                if ( !m_currentlist.contains( item ) )
                {
                    if ( !m_id ) // carefull we're animating
                    {
                        Plasma::Animator::self()->stopCustomAnimation( m_id );
                        m_id = 0;
                    }
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

  
                }
                break;
            }
            case PHOTOS_MODE_AUTOMATIC :
            {
                if ( !m_currentlist.contains( item ) )
                {
                    DragPixmapItem *dragpix = new DragPixmapItem( this );
                    dragpix->setPixmap( The::svgHandler()->addBordersToPixmap(
                        item->photo->scaledToHeight( (int) size().height() - 4 * m_margin,  Qt::SmoothTransformation ), 5, "", true ) );
                    dragpix->SetClickableUrl( item->urlpage );

                    if ( m_id == 0 ) // only pos and show if
                    {
                        if ( ! m_pixmaplist.empty() )
                        {
                            dragpix->setPos( m_pixmaplist.last()->boundingRect().width() + m_pixmaplist.last()->pos().x() + m_margin, 0 ) ;
                            dragpix->show();
                        }
                        else
                        {
                            dragpix->setPos( 0, 0 ) ;
                            dragpix->show();
                        }
                    }
                    
                    m_pixmaplist << dragpix;

                    // connect the end of the animation
                    connect( Plasma::Animator::self(), SIGNAL( customAnimationFinished( int ) ), SLOT( automaticAnimEnd( int ) ) );
                    // set a timer after and launch
                    QTimer::singleShot( m_timer, this, SLOT( automaticAnimBegin() ) );

                    
                }
                break;
            }
            case PHOTOS_MODE_FADING :
            {
                if ( !m_currentlist.contains( item ) )
                {
                    if ( !m_id ) // carefull we're animating
                    {
                        Plasma::Animator::self()->stopCustomAnimation( m_id );
                        m_id = 0;
                    }
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
                }
                break;
            }
        }
    }
    m_currentlist = list;
}


void PhotosScrollWidget::hoverEnterEvent(QGraphicsSceneHoverEvent*)
{
    DEBUG_BLOCK
    switch ( m_mode )
    {
        case PHOTOS_MODE_INTERACTIVE :
        {
            break;
        }
        
        case PHOTOS_MODE_AUTOMATIC :
        {
            Plasma::Animator::self()->stopCustomAnimation( m_id );
            m_id = 0;
            break;
        }
        
        case PHOTOS_MODE_FADING :
        {            
            break;
        }
    }
}


void PhotosScrollWidget::hoverLeaveEvent(QGraphicsSceneHoverEvent*)
{
    DEBUG_BLOCK
    switch ( m_mode )
    {
        case PHOTOS_MODE_INTERACTIVE :
        {
            Plasma::Animator::self()->stopCustomAnimation( m_id );
            m_id = 0;
            break;
        }
        
        case PHOTOS_MODE_AUTOMATIC :
        {
            QTimer::singleShot( m_timer, this, SLOT( automaticAnimBegin() ) );
            break;
        }
        
        case PHOTOS_MODE_FADING :
        {
            break;
        }
    }
}

void PhotosScrollWidget::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    DEBUG_BLOCK
    switch ( m_mode )
    {                
        case PHOTOS_MODE_INTERACTIVE :
        {
            m_speed = ( event->pos().x() - ( size().width() / 2 ) ) / size().width();
            m_speed*=20;
            // if m_id = 0, we don't have an animator yet, let's start the animation
            if ( !m_id )
                m_id = Plasma::Animator::self()->customAnimation( m_scrollmax / 2, m_scrollmax*10, Plasma::Animator::LinearCurve, this, "animate" );
        }
    }
}

void
PhotosScrollWidget::automaticAnimBegin()
{
    if ( m_id == 0 )
    {
        DEBUG_BLOCK
        if ( m_currentPix < m_pixmaplist.size() - 1 )
        {
            m_delta = m_pixmaplist.at( m_currentPix )->boundingRect().width() + m_margin;
            m_id = Plasma::Animator::self()->customAnimation( m_delta * 10, m_delta*20, Plasma::Animator::LinearCurve, this, "animate" );
            m_currentPix++;
        }
        else if ( m_currentPix == m_pixmaplist.size() - 1 )
        {
            m_currentPix = 0;
            m_actualpos = 0;
            m_delta = m_pixmaplist.at( m_currentPix )->boundingRect().width() + m_margin;
            m_id = Plasma::Animator::self()->customAnimation( m_delta , m_delta*20, Plasma::Animator::LinearCurve, this, "animate" );
        }
    }
}

void
PhotosScrollWidget::automaticAnimEnd( int id )
{
    if ( id == m_id )
    {
        if ( m_mode != PHOTOS_MODE_AUTOMATIC )
            return;
        DEBUG_BLOCK
        Plasma::Animator::self()->stopCustomAnimation( m_id );
        m_id = 0;
        QTimer::singleShot( m_timer, this, SLOT( automaticAnimBegin() ) );
    }
}


void PhotosScrollWidget::animate( qreal )
{
    DEBUG_BLOCK
    switch ( m_mode )
    {
        case PHOTOS_MODE_INTERACTIVE :
        {
            // If we're are near the border and still asking to go higher !
            if ( ( ( this->childItems().first()->pos().x() + this->childItems().first()->boundingRect().width() + 10 ) > this->boundingRect().width() ) && ( m_speed < 0 ) )
            {
                Plasma::Animator::self()->stopCustomAnimation( m_id );
                m_id = 0;
                return;
            }
            // If we're are near the border and still asking to go down
            if ( ( ( this->childItems().last()->pos().x() - 10 ) < 0 ) && ( m_speed > 0 ) )
            {
                Plasma::Animator::self()->stopCustomAnimation( m_id );
                m_id = 0;
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
            if ( !m_pixmaplist.empty() )
            {

                if ( ( m_pixmaplist.at( m_currentPix - 1 )->pos().x() + m_pixmaplist.at( m_currentPix - 1)->boundingRect().width() + m_margin )  <= ( - m_margin/2 + 1))
                {
                    automaticAnimEnd ( m_id );
                    return;
                }
                
                m_actualpos -= 1;
                
                for( int a = 0 ; a < m_pixmaplist.size(); a++ )
                {
                    if ( a == 0 )
                        m_pixmaplist.at( a )->setPos( m_actualpos, m_pixmaplist.at( a )->pos().y() );
                    else
                        m_pixmaplist.at( a )->setPos( m_pixmaplist.at( a - 1 )->pos().x() + m_pixmaplist.at( a - 1 )->boundingRect().width() + m_margin - 1, m_pixmaplist.at( a )->pos().y() );
                    m_pixmaplist.at( a )->show();
                }
            }
            
       //     m_actualpos = right;    
            break;
        }
        case PHOTOS_MODE_FADING :
        {
            break;
        }
    }        
}

#include "PhotosScrollWidget.moc"

