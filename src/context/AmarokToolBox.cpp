/*******************************************************************************
* copyright              : (C) 2008 William Viana Soares <vianasw@gmail.com>   *
*                                                                              *
********************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "AmarokToolBox.h"
#include "Debug.h"
#include "widgets/ToolBoxIcon.h"

#include <KColorScheme>

#include <plasma/applet.h>
#include <plasma/theme.h>
// #include <plasma/widgets/icon.h>

#include <QGraphicsSceneHoverEvent>
#include <QPainter>
#include <QRadialGradient>
#include <QGraphicsView>
#include <QColor>

#include <cmath>

namespace Context
{

AmarokToolBox::AmarokToolBox( QGraphicsItem *parent )
    : QGraphicsItem( parent )
    , m_size( 30 )
    , m_hovering( 0 )    
    , m_showing( 0 )
    , m_showingTools( 0 )
    , m_actionsCount( 0 )
    , m_animCircleFrame( 0 )
    , m_animCircleId( 0 )
{
    DEBUG_BLOCK
//     connect( Plasma::Animator::self(), SIGNAL(movementFinished(QGraphicsItem*)), this, SLOT(toolMoved(QGraphicsItem*)));
//     connect(this, SIGNAL(toggled()), this, SLOT(toggle()));

    setZValue( 10000000 );
    setFlag( ItemClipsToShape, false );
    setFlag( ItemClipsChildrenToShape, false );
    setFlag( ItemIgnoresTransformations, true );
    setAcceptsHoverEvents( true );
    m_timer = new QTimer(this);
}

AmarokToolBox::~AmarokToolBox()
{}

QRectF
AmarokToolBox::boundingRect() const
{
    return QRectF( 0, 0, this->size() * 3, this->size() * 3 );
}

void
AmarokToolBox::show()
{
    m_showing = true;
}

void
AmarokToolBox::hide()
{
    m_showing = false;
}

bool
AmarokToolBox::showing() const
{
    return m_showing;
}

QPainterPath
AmarokToolBox::shape() const
{
    DEBUG_BLOCK
    QPainterPath path;
    int toolSize = ( size() * 2 )+ ( int ) m_animCircleFrame;

    debug() << "animCircleFrame: " << m_animCircleFrame;    
    debug() << "boundingRect: " << boundingRect();
    
    QPointF center( boundingRect().width()/2, boundingRect().height() );
    debug() << "center: " << center;
    path.moveTo( center );

    path.arcTo( QRectF( boundingRect().width()/2.0 - toolSize/2.0,
                        boundingRect().bottom() - toolSize/2.0 ,
                        toolSize, toolSize ), 0, 180 );
//     path.arcTo( boundingRect(), 0, -180 );
    return path;
}

void
AmarokToolBox::paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget )
{
    DEBUG_BLOCK
    Q_UNUSED( option )
    Q_UNUSED( widget )

    if( !showing() )
        return;
    
    DEBUG_LINE_INFO
    painter->save();
    painter->translate( boundingRect().topLeft() );

    QColor color1 = KColorScheme(QPalette::Active, KColorScheme::Window,
                               Plasma::Theme::defaultTheme()->colorScheme()).background().color();
    color1.setAlpha( 40 );

    QColor color2 =  KColorScheme(QPalette::Active, KColorScheme::Window,
                               Plasma::Theme::defaultTheme()->colorScheme()).foreground().color();
    color2.setAlpha( 60 );

    QPainterPath p = shape();
    
    QRadialGradient gradient( QPointF( boundingRect().width() / 2.0, boundingRect().height() ),
                              size() * 2 + m_animCircleFrame );
    gradient.setFocalPoint( QPointF( boundingRect().width() / 2.0, boundingRect().height() ) );
    gradient.setColorAt( 0, color1 );
    gradient.setColorAt( .87, color1 );
    gradient.setColorAt( .97, color2 );
    color2.setAlpha( 0 );
    gradient.setColorAt( 1, color2 );

    painter->restore();
    
    painter->save();
    if( m_animHighlightFrame > 0.5 )
        painter->setOpacity( m_animHighlightFrame );
    else
        painter->setOpacity( 0.5 );
    painter->setPen( Qt::NoPen );
    painter->setRenderHint( QPainter::Antialiasing, true );
    painter->setBrush( gradient );
    painter->drawPath( p );
    painter->restore();
    
}

int
AmarokToolBox::size() const
{
    return m_size;
}

void
AmarokToolBox::setSize( int newSize )
{
    m_size = newSize;
}

void
AmarokToolBox::showTools()
{
    foreach( QGraphicsItem *c, QGraphicsItem::children() )
        c->show();
    if( m_animCircleId )
        Plasma::Animator::self()->stopCustomAnimation( m_animCircleId );
    m_showingTools = true;
    m_animCircleId = Plasma::Animator::self()->customAnimation( 10, 240, Plasma::Animator::EaseInCurve,
                                                                this, "animateCircle" );    
}

void
AmarokToolBox::hideTools()
{
    if( m_showingTools )
    {
        foreach( QGraphicsItem *c, QGraphicsItem::children() )
            c->hide();
        if( m_animCircleId )
            Plasma::Animator::self()->stopCustomAnimation( m_animCircleId );

        m_showingTools = false;
        m_animCircleId = Plasma::Animator::self()->customAnimation( 10, 240, Plasma::Animator::EaseInCurve,
                                                                this, "animateCircle" );
    }
}

void
AmarokToolBox::hoverEnterEvent( QGraphicsSceneHoverEvent *event )
{
    DEBUG_BLOCK
    if( m_animHighlightId )
        Plasma::Animator::self()->stopCustomAnimation( m_animHighlightId );
    if( m_timer->isActive() )
        m_timer->stop();
    m_hovering = true;
    m_animHighlightId = Plasma::Animator::self()->customAnimation( 10, 240, Plasma::Animator::EaseInCurve,
                                                                   this, "animateHighlight" );
    QGraphicsItem::hoverEnterEvent( event );
}

void
AmarokToolBox::hoverLeaveEvent( QGraphicsSceneHoverEvent *event )
{
    if( m_animHighlightId )
        Plasma::Animator::self()->stopCustomAnimation( m_animHighlightId );
    m_hovering = false;

    connect( m_timer, SIGNAL( timeout() ), this, SLOT( timeToHide() ) );
    m_timer->start( 2000 );
    
    QGraphicsItem::hoverLeaveEvent( event );
}

void
AmarokToolBox::timeToHide()
{
    m_timer->stop();
    m_animHighlightId = Plasma::Animator::self()->customAnimation( 10, 240, Plasma::Animator::EaseInCurve,
                                                                   this, "animateHighlight" );
    hideTools();
    
}

void
AmarokToolBox::animateHighlight( qreal progress )
{
    if( m_hovering )
        m_animHighlightFrame = progress;
    else
        m_animHighlightFrame = 1.0 - progress;

    if( progress >= 1.0 )
        m_animHighlightId = 0;
    update();
}

void
AmarokToolBox::addAction( QAction *action )
{
    DEBUG_BLOCK
    if ( !action ) {
        return;
    }

    ToolBoxIcon *tool = new ToolBoxIcon( this );

    tool->setAction( action );
    tool->setText( "" );
    tool->setToolTip( action->text() );
    tool->setDrawBackground( false );
    tool->setOrientation( Qt::Horizontal );
    QSizeF iconSize = tool->sizeFromIconSize( 22 );
    tool->setMinimumSize( iconSize );
    tool->setMaximumSize( iconSize );
    tool->resize( tool->size() );
    
    m_actionsCount++;
    
    tool->hide();
    tool->setZValue( zValue() + 1 );
    qreal rad = size()*3 + 15;
    qreal x = rad - ( ( tool->size().width() + 5 ) * m_actionsCount  ) ;
    qreal y = -sqrt( rad * rad - x * x ) + size()*4;
    tool->setPos( x + 5, y );
    //make enabled/disabled tools appear/disappear instantly
//     connect(tool, SIGNAL(changed()), this, SLOT(updateToolBox()));
    debug() << "x,y: " << x << sqrt( rad* rad - x * x );
}

void
AmarokToolBox::mousePressEvent( QGraphicsSceneMouseEvent *event )
{
    DEBUG_BLOCK
    event->accept();
}

void
AmarokToolBox::mouseReleaseEvent( QGraphicsSceneMouseEvent *event )
{
    DEBUG_BLOCK
    if ( boundingRect().contains( event->pos() ) )
    {
        if( m_timer->isActive() )
            m_timer->stop();
        if( m_showingTools )
            hideTools();
        else
            showTools();
    }
}

void
AmarokToolBox::animateCircle( qreal progress )
{
    if( m_showingTools )
        m_animCircleFrame = size() * progress;
    else
        m_animCircleFrame = size() * ( 1.0 - progress );

    if ( progress >= 1 )
        m_animCircleId = 0;
    update();
}

}

#include "AmarokToolBox.moc"
