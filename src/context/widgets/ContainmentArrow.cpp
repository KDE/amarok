/****************************************************************************************
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
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

#include "ContainmentArrow.h"

#include "core/support/Debug.h"

#include <KStandardDirs>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>

// by default this item just grabs the coords of its parents, and positions itself to maximise in the
// direction that it is pointing to.

namespace Context 
{

ContainmentArrow::ContainmentArrow( QGraphicsItem *parent, int direction ) :
    QGraphicsItem( parent ),
    m_showing( false ),
    m_disabled( false ),
    m_timer( 0 ) ,
    m_arrowSvg( 0 ),
    m_containment( 0 )
{
    DEBUG_BLOCK
    
    setZValue( 10000000 );
    setFlag( ItemClipsToShape, false );
    setFlag( ItemClipsChildrenToShape, false );
    setFlag( ItemIgnoresTransformations, true );
    setAcceptsHoverEvents( true );
    
    m_timer = new QTimer( this );
    connect( m_timer, SIGNAL( timeout() ), this, SLOT( timeToHide() ) );
    
    m_arrowSvg = new Context::Svg( this );
    m_arrowSvg->setImagePath( KStandardDirs::locate( "data", "amarok/images/navigation_arrows.svg" ) );
    m_arrowSvg->setContainsMultipleImages( true );
    
    debug() << "got svg path: " << m_arrowSvg->imagePath();
    
    m_containment = dynamic_cast<Containment *>( parent );
    if( !m_containment )
    {
        debug() << "ERROR! ContainmentArrow needs to be passed a Containment parent!";
    }
    else
    {
        qreal height = 0, width = 0;
        switch( direction )
        {
            case DOWN:
            case UP:
            {
                width = m_containment->size().width();
                debug() << " up/down arrow original width: " << width;
                QRectF arrow;
                if( direction == UP )
                    arrow = m_arrowSvg->elementRect( "up_arrow" );
                else
                    arrow = m_arrowSvg->elementRect( "down_arrow" );
                m_aspectRatio = arrow.width() / arrow.height();
            
                height = width / m_aspectRatio;
                debug() << "up/down arrow m_aspectRatio and height is: " << m_aspectRatio << height;
                debug() << "got UP/DOWN arrow with sizes: " << width << height;
                break;
            }
            case LEFT:
            case RIGHT:
            {
                height = m_containment->size().height();
                QRectF arrow;
                debug() << " left/right arrow original height: " << height;
            
                if( direction == LEFT )
                    arrow = m_arrowSvg->elementRect( "left_arrow" );
                else
                    arrow = m_arrowSvg->elementRect( "right_arrow" );            
                m_aspectRatio = arrow.width() / arrow.height();

                width = height * m_aspectRatio;
                debug() << "left/right arrow m_aspectRatio and width is: " << m_aspectRatio << width;

                debug() << "got RIGHT/LEFT arrow with sizes: " << width << height;
                break;
            }
            default:
                error() << "Unspecified state, setting 0 size for arrows";
        }
        m_size = QSize( width, height );
    }
    
    debug() << "ContainmentArrow: SETTING DIRECTION TO: " << direction;
    m_arrowDirection = direction;
}

ContainmentArrow::~ContainmentArrow()
{
    // delete m_timer;
    // delete m_arrowSvg;
    // delete m_containment;
}


QRectF 
ContainmentArrow::boundingRect() const
{
    return QRectF( QPointF( 0, 0 ), m_size );

}

QSize 
ContainmentArrow::size() const
{
    return m_size;
}

void 
ContainmentArrow::resize( const QSizeF newSize ) 
{
    DEBUG_BLOCK
    prepareGeometryChange();
    switch( m_arrowDirection )
    {
    case DOWN: // anchor to new width
    case UP:
    {
        qreal newheight = newSize.width() / m_aspectRatio;
        m_size = QSize( newSize.width(), newheight );
        break;
    }
    case LEFT:
    case RIGHT: // anchor on height
    {
        qreal newWidth = newSize.height() * m_aspectRatio;
        m_size = QSize( newWidth, newSize.height() );
        break;
    }
    }
    m_arrowSvg->resize( m_size );
    debug() << "updating new size to: " << m_size;
    update();
}

void
ContainmentArrow::enable()
{
    m_disabled = false;
}

void ContainmentArrow::disable()
{
    m_disabled = true;
}


void 
ContainmentArrow::show()
{
    // DEBUG_BLOCK
    m_showing = true;
    Plasma::Animator::self()->animateItem( this, Plasma::Animator::AppearAnimation );
}

void 
ContainmentArrow::hide()
{

    m_showing = false;
    update();
    Plasma::Animator::self()->animateItem( this, Plasma::Animator::DisappearAnimation );
}

void 
ContainmentArrow::paint( QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget )
{
    // DEBUG_BLOCK
    
    Q_UNUSED( option )
    Q_UNUSED( widget )
    
    if( !m_showing )
        return;
    
    // debug() << "in ContainmenArrow::paint, painting in " << boundingRect() << " with opacity: " << m_animHighlightFrame;
    
    p->save();
    
    p->setOpacity( m_animHighlightFrame );
    if( m_arrowDirection == UP )
        m_arrowSvg->paint( p, boundingRect(), "up_arrow" );
    else if( m_arrowDirection == DOWN )
        m_arrowSvg->paint( p, boundingRect(), "down_arrow" );
    else if( m_arrowDirection == LEFT )
        m_arrowSvg->paint( p, boundingRect(), "left_arrow" );
    else if( m_arrowDirection == RIGHT )
        m_arrowSvg->paint( p, boundingRect(), "right_arrow" );
    p->restore();
    
}


void 
ContainmentArrow::hoverEnterEvent( QGraphicsSceneHoverEvent *event )
{
    // DEBUG_BLOCK
    if( m_hovering || m_disabled )
        return;
    if( m_animHighlightId )
        Plasma::Animator::self()->stopCustomAnimation( m_animHighlightId );
    if( m_timer->isActive() )
        m_timer->stop();
    m_hovering = true;
    m_showing = true;
    m_animHighlightId = Plasma::Animator::self()->customAnimation( 10, 240, Plasma::Animator::EaseInCurve,
                                                                   this, "animateHighlight" );
    QGraphicsItem::hoverEnterEvent( event );
}

void 
ContainmentArrow::hoverLeaveEvent( QGraphicsSceneHoverEvent *event )
{
    // DEBUG_BLOCK
    if( m_disabled )
        return;
    if( m_animHighlightId )
        Plasma::Animator::self()->stopCustomAnimation( m_animHighlightId );
    m_hovering = false;
    m_animHighlightFrame = 0.0;
    // m_showing = false;
    
    // m_timer->start( 100 );
    timeToHide();
    
    QGraphicsItem::hoverLeaveEvent( event );
}


void
ContainmentArrow::timeToHide()
{
    m_timer->stop();

    m_animHighlightId = Plasma::Animator::self()->customAnimation( 10, 100, Plasma::Animator::EaseOutCurve,
                                                                   this, "animateHighlight" );
    
}

void
ContainmentArrow::animateHighlight( qreal progress ) //SLOT
{
    // DEBUG_BLOCK
    if( m_hovering )
        m_animHighlightFrame = progress;
    else
        m_animHighlightFrame = 1.0 - progress;

    if( progress >= 1.0 )
        m_animHighlightId = 0;
    update();
}

void 
ContainmentArrow::mousePressEvent( QGraphicsSceneMouseEvent *event )
{
    // DEBUG_BLOCK
    event->accept();
}

void 
ContainmentArrow::mouseReleaseEvent( QGraphicsSceneMouseEvent *event )
{
    // DEBUG_BLOCK
    if ( boundingRect().contains( event->pos() ) && !m_disabled )
    {
        debug() << "EMITTING changeContainment!";
        emit changeContainment( m_arrowDirection );
        // TODO add up/down
        if( m_timer->isActive() )
            m_timer->stop();
            
    }
}

}

#include "ContainmentArrow.moc"
