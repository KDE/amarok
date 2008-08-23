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

#include <KColorScheme>
#include <KPluginInfo>
#include <KStandardDirs>

#include <plasma/applet.h>
#include <plasma/theme.h>

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
    , m_size( 90, 90 )
    , m_hovering( 0 )    
    , m_showing( 0 )
    , m_showingTools( 0 )
    , m_actionsCount( 0 )
    , m_animCircleFrame( 0 )
    , m_animCircleId( 0 )
    , m_icon( "configure" )
    , m_containment( 0 )    
{
    DEBUG_BLOCK
        
    connect( Plasma::Animator::self(), SIGNAL( movementFinished( QGraphicsItem* ) ),
             this, SLOT( toolMoved( QGraphicsItem* ) ) );

    setZValue( 10000000 );
    setFlag( ItemClipsToShape, false );
    setFlag( ItemClipsChildrenToShape, false );
    setFlag( ItemIgnoresTransformations, true );
    setAcceptsHoverEvents( true );
    
    m_timer = new QTimer( this );
    connect( m_timer, SIGNAL( timeout() ), this, SLOT( timeToHide() ) );

    m_menu = new AmarokToolBoxMenu( this );
    m_containment = dynamic_cast<Containment *>( parent );
    m_menu->setContainment( m_containment );

    connect( m_menu, SIGNAL( menuHidden() ), this, SLOT( show() ) );
    connect( m_menu, SIGNAL( changeContainment( Plasma::Containment * ) ),
             this, SIGNAL( changeContainment( Plasma::Containment * ) ) );
             
    QAction *appletMenuAction = new QAction( i18n( "Add Widgets..." ), this );
    appletMenuAction->setIcon( KIcon( "list-add" ) );
    appletMenuAction->setVisible( true );
    appletMenuAction->setEnabled( true );
    connect( appletMenuAction, SIGNAL( triggered() ), this, SLOT( showWidgetsMenu() ) );
    addAction( appletMenuAction );
    
    
    
}

AmarokToolBox::~AmarokToolBox()
{
    delete m_timer;
}

QRectF
AmarokToolBox::boundingRect() const
{
    return QRectF( QPointF( 0.0, 0.0 ), this->size() );
}

void
AmarokToolBox::show()
{
    resize( QSize( 90, 90 ) );
    emit correctToolBoxPos();
//     if( containment() )
//     {
//         ContextView *cv = dynamic_cast<ContextView *>( containment()->view() );
//         if( cv )
//             containment()->correctToolBoxPos( cv->zoomLevel() );
//     }
    m_showing = true;
    Plasma::Animator::self()->animateItem( this, Plasma::Animator::AppearAnimation );
}

void
AmarokToolBox::hide()
{
    m_showing = false;
    update();
//     Plasma::Animator::self()->animateItem( this, Plasma::Animator::DisappearAnimation );
}

bool
AmarokToolBox::showingToolBox() const
{
    return m_showing;
}

bool
AmarokToolBox::showingMenu() const
{
    return !showingToolBox();
}

QPainterPath
AmarokToolBox::shape() const
{
    QPainterPath path;
    int toolSize = ( size().width()/3 * 2 )+ ( int ) m_animCircleFrame;
    
    QPointF center( boundingRect().width()/2, boundingRect().height() );
    path.moveTo( center );

    path.arcTo( QRectF( boundingRect().width()/2.0 - toolSize/2.0,
                        boundingRect().bottom() - toolSize/2.0 ,
                        toolSize, toolSize ), 0, 180 );
    path.closeSubpath();
    return path;
}

void
AmarokToolBox::showWidgetsMenu()
{
    hideTools();
    hide();
    
    //resize the toolbox to hold the widgets menu
    resize( m_menu->boundingRect().size().toSize() );

    emit correctToolBoxPos();

    m_menu->setPos( 0, 0 );
    m_menu->show();
    
}

Containment *
AmarokToolBox::containment() const
{
    return m_containment;
}


void
AmarokToolBox::paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget )
{
    Q_UNUSED( option )
    Q_UNUSED( widget )
    
    if( !showingToolBox() )
        return;
    
    painter->save();
    painter->translate( QPoint( boundingRect().width()/2, boundingRect().height() ) );

    QColor color1 = KColorScheme(QPalette::Active, KColorScheme::Window,
                               Plasma::Theme::defaultTheme()->colorScheme()).background().color();
    color1.setAlpha( 120 );

    QColor color2 =  KColorScheme(QPalette::Active, KColorScheme::Window,
                               Plasma::Theme::defaultTheme()->colorScheme()).foreground().color();

    QPainterPath p = shape();
    
    QRadialGradient gradient( QPointF( boundingRect().width() / 2.0, boundingRect().height() ),
                              size().width()/3 * 2 + m_animCircleFrame );
    gradient.setFocalPoint( QPointF( boundingRect().width() / 2.0, boundingRect().height() ) );


    color2.setAlpha( 0 );
    gradient.setColorAt( 0, color2 );
    color2.setAlpha( 120 );
    gradient.setColorAt( .80, color2 );
    gradient.setColorAt( .90, color1 );

    painter->restore();
    
    painter->save();
    if( m_animHighlightFrame > 0.5 )
    {        
        painter->setOpacity( m_animHighlightFrame );
        m_icon.paint( painter, QRect( QPoint( (int)boundingRect().width()/2 - 12,
                                              (int)boundingRect().height()/2  + 18 ), QSize( 24, 24 ) ) );
    }
    else
    {
        painter->setOpacity( 0.6 );
        m_icon.paint( painter, QRect( QPoint( (int)boundingRect().width()/2 - 12,
                                              (int)boundingRect().height()/2 + 18 ), QSize( 24, 24 ) ),
                      Qt::AlignCenter, QIcon::Disabled, QIcon::Off );
    }
    
    painter->setPen( QPen( Qt::gray, 1 ) );
    painter->setRenderHint( QPainter::Antialiasing, true );
    painter->setBrush( gradient );
    painter->drawPath( p );
    painter->restore();
    
}

QSize
AmarokToolBox::size() const
{
    return m_size;
}

void
AmarokToolBox::resize( const QSize &newSize )
{
    prepareGeometryChange();
    m_size = newSize;
    update();
}

void
AmarokToolBox::showTools()
{
    int i = 1;
    m_showingTools = true;
    foreach( QGraphicsItem *tool, QGraphicsItem::children() )
    {        
        qreal rad = size().width() + 12;
        
        QPoint center( boundingRect().width()/2, boundingRect().height() );

        tool->setPos( center );
        tool->show();
        
        qreal x = rad - ( ( tool->boundingRect().width() + 4 ) * i ) + 20;
        qreal y = -sqrt( rad * rad - x * x ) + size().width() + 30;
        Plasma::Animator::self()->moveItem( tool, Plasma::Animator::SlideInMovement, QPoint( x + 25, y ) );
        
        i++;
    }
    if( m_animCircleId )
        Plasma::Animator::self()->stopCustomAnimation( m_animCircleId );
    
    m_animCircleId = Plasma::Animator::self()->customAnimation( 10, 240, Plasma::Animator::EaseInCurve,
                                                                this, "animateCircle" );    
}

void
AmarokToolBox::hideTools()
{
    if( m_showingTools )
    {
        m_showingTools = false;
        foreach( QGraphicsItem *tool, QGraphicsItem::children() )
        {
            AmarokToolBoxMenu *toolBoxMenu = dynamic_cast<AmarokToolBoxMenu *>( tool );
            if( toolBoxMenu == m_menu )
                continue;
            else
            {
                ToolBoxIcon *amarokTool = dynamic_cast<ToolBoxIcon *>( tool );
                if( amarokTool && !amarokTool->text().isEmpty() )
                    continue;
            }
            
            QPoint center( boundingRect().width()/2, boundingRect().height() );
            Plasma::Animator::self()->moveItem( tool, Plasma::Animator::SlideInMovement, center );
        }
        if( m_animCircleId )
            Plasma::Animator::self()->stopCustomAnimation( m_animCircleId );
        
        m_animCircleId = Plasma::Animator::self()->customAnimation( 10, 240, Plasma::Animator::EaseInCurve,
                                                                this, "animateCircle" );
    }
}

void
AmarokToolBox::toolMoved( QGraphicsItem *item )
{
    if ( !m_showingTools&&
        QGraphicsItem::children().indexOf( static_cast<Plasma::Applet*>( item ) ) != -1 )
    {
        item->hide();
    }
}

void
AmarokToolBox::hoverEnterEvent( QGraphicsSceneHoverEvent *event )
{
    if( m_hovering )
        return;
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
    
    m_timer->start( 2000 );
    
    QGraphicsItem::hoverLeaveEvent( event );
}

void
AmarokToolBox::timeToHide()
{
    m_timer->stop();

    m_animHighlightId = Plasma::Animator::self()->customAnimation( 10, 240, Plasma::Animator::EaseOutCurve,
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
    tool->resize( iconSize );
    
    m_actionsCount++;
    
    tool->hide();
    tool->setZValue( zValue() + 1 );

    //make enabled/disabled tools appear/disappear instantly
//     connect(tool, SIGNAL(changed()), this, SLOT(updateToolBox()));
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
    if( showingMenu() )
        return;
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
        m_animCircleFrame = size().width()/3 * progress;
    else
        m_animCircleFrame = size().width()/3 * ( 1.0 - progress );

    if ( progress >= 1 )
        m_animCircleId = 0;
    update();
}

}

#include "AmarokToolBox.moc"
