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
                if( amarokTool && amarokTool->text() != "" )
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

AmarokToolBoxMenu::AmarokToolBoxMenu( QGraphicsItem *parent )
    : QGraphicsItem( parent )
    , m_containment( 0 )
    , m_menuSize( 4 )
    , m_showing( 0 )
    , m_delay( 250 )
{
    setAcceptsHoverEvents( true );
    
    foreach ( const KPluginInfo& info, Plasma::Applet::listAppletInfo( QString(), "amarok" ) )
    {
        if ( info.property( "NoDisplay" ).toBool() )
        {
            // we don't want to show the hidden category
            continue;
        }

        m_appletsList.insert( info.name(), info.pluginName() );
    }

    m_timer = new QTimer( this );
    m_scrollDelay = new QTimer( this );
    
    connect( m_timer, SIGNAL( timeout() ), this, SLOT( timeToHide() ) );
    connect( m_scrollDelay, SIGNAL( timeout() ), this, SLOT( delayedScroll() ) );
    
    //insert in the stack so the first applet in alphabetical order is the first one
    QStringList appletsNames = m_appletsList.keys();
    for( int i = appletsNames.size() - 1; i >= 0; i-- )
        m_bottomMenu.push( appletsNames[i] );
    
    populateMenu();
    
    m_hideIcon = new ToolBoxIcon( this );
    
    QAction *hideMenu = new QAction( "", this );
    hideMenu->setIcon( KIcon( "window-close" ) );
    hideMenu->setEnabled( true );
    hideMenu->setVisible( true );

    connect( hideMenu, SIGNAL( triggered() ), this, SLOT( hide() ) );
    m_hideIcon->setAction( hideMenu );
    m_hideIcon->setToolTip( i18n( "Hide menu" ) );
    QSizeF iconSize = m_hideIcon->sizeFromIconSize( 22 );

    m_hideIcon->setMinimumSize( iconSize );
    m_hideIcon->setMaximumSize( iconSize );
    m_hideIcon->resize( m_hideIcon->size() );

    m_hideIcon->setPos( 5, boundingRect().height() - 32 * m_menuSize - 50 );
    m_hideIcon->setZValue( zValue() + 1 );
    m_hideIcon->hide();
    
    m_upArrow = new ToolBoxIcon( this );
    m_downArrow = new ToolBoxIcon( this );
    createArrow( m_upArrow, "up" );
    createArrow( m_downArrow, "down" );
}

AmarokToolBoxMenu::~AmarokToolBoxMenu()
{
    delete m_hideIcon;
    delete m_downArrow;
    delete m_upArrow;
    delete m_timer;
    delete m_scrollDelay;
}

void
AmarokToolBoxMenu::setContainment( Containment *newContainment )
{
    m_containment = newContainment;
    initRunningApplets();
}

Containment *
AmarokToolBoxMenu::containment() const
{
    return m_containment;
}

QRectF
AmarokToolBoxMenu::boundingRect() const
{
    return QRectF( QPointF( 0, 0 ), QSize( 185, 42 * ( m_menuSize + 2 ) ) );
}

void
AmarokToolBoxMenu::populateMenu()
{
    for( int i = 0; i < m_menuSize; i++ )
    {
        ToolBoxIcon *entry = new ToolBoxIcon( this );

        const QString appletName = m_bottomMenu.pop();

        setupMenuEntry( entry, appletName );
        entry->hide();
        m_currentMenu << entry;
    }
}

void
AmarokToolBoxMenu::initRunningApplets()
{
    if( !containment() )
        return;
    
    Plasma::Corona *corona = containment()->corona();
    
    if( !corona )
        return;
    
    m_runningApplets.clear();
    QList<Plasma::Containment *> containments = corona->containments();
    foreach( Plasma::Containment *containment, containments )
    {
        connect( containment, SIGNAL( appletAdded( Plasma::Applet *, QPointF ) ),
                 this, SLOT( appletAdded( Plasma::Applet *) ) );
        connect( containment, SIGNAL( appletRemoved( Plasma::Applet * ) ),
                 this, SLOT( appletRemoved( Plasma::Applet * ) ) );
        QList<QString> appletNames;
        foreach( Plasma::Applet *applet, containment->applets() )
        {
            appletNames << applet->pluginName();
            m_appletNames[applet] = applet->pluginName();
        }
        m_runningApplets[containment] = appletNames;
    }
}

void
AmarokToolBoxMenu::appletAdded( Plasma::Applet *applet )
{
    if( sender() != 0 )
    {
        Plasma::Containment *containment = dynamic_cast<Plasma::Containment *>( sender() );
        if( containment )
        {
            m_runningApplets[containment] << applet->pluginName();
            m_appletNames[applet] = applet->pluginName();
        }
    }
}

void
AmarokToolBoxMenu::appletRemoved( Plasma::Applet *applet )
{
    if( sender() != 0 )
    {
        Plasma::Containment *containment = dynamic_cast<Plasma::Containment *>( sender() );
        if( containment )
        {
            QString name = m_appletNames.take( applet );
            m_runningApplets[containment].removeAll( name );
        }
    }
}

bool
AmarokToolBoxMenu::showing() const
{
    return m_showing;
}

void
AmarokToolBoxMenu::show()
{
    if( showing() )
        return;
    
    m_showing = true;
    
    if( m_bottomMenu.count() > 0 )
    {
        m_downArrow->setPos( boundingRect().width() / 2 - m_downArrow->size().width()/2,
                            boundingRect().height() - 20 );
        m_downArrow->resetTransform();
        m_downArrow->show();        
    }
    
    if( m_topMenu.count() > 0 )
    {
        const int height = static_cast<int>( m_currentMenu.first()->boundingRect().height() ) + 9;
        m_upArrow->resetTransform();
        m_upArrow->setPos( boundingRect().width()/2 - m_upArrow->size().width()/2,
                            boundingRect().height() - m_menuSize * height - 50 );
        m_upArrow->show();
    }
    
    m_hideIcon->show();
    for( int i = m_currentMenu.count() - 1; i >= 0; i-- )
    {
        ToolBoxIcon *entry = m_currentMenu[m_currentMenu.count() - i - 1];
        entry->show();
        const int height = static_cast<int>( entry->boundingRect().height() ) + 9;
        
        Plasma::Animator::self()->moveItem( entry, Plasma::Animator::SlideInMovement,
                                            QPoint( 5, boundingRect().height() - height * i - 50 ) );
    }
}

void
AmarokToolBoxMenu::hide()
{
    if( !showing() )
        return;
    m_showing = false;
    foreach( QGraphicsItem *c, QGraphicsItem::children() )
        c->hide();
    emit menuHidden();
}

void
AmarokToolBoxMenu::setupMenuEntry( ToolBoxIcon *entry, const QString &appletName )
{
        entry->setDrawBackground( true );
        entry->setOrientation( Qt::Horizontal );
        entry->setText( appletName );

        QSizeF size( 180, 24 );
        entry->setMinimumSize( size );
        entry->setMaximumSize( size );
        entry->resize( size );

        entry->setPos( 5, boundingRect().height() );

        entry->setZValue( zValue() + 1 );
        entry->setData( 0, QVariant( m_appletsList[appletName] ) );
        entry->show();
        connect( entry, SIGNAL( addApplet( const QString & ) ), this, SLOT( addApplet( const QString & ) ) );
}

void
AmarokToolBoxMenu::addApplet( const QString &pluginName )
{
    DEBUG_BLOCK
    if( pluginName != QString() )
    {
        bool appletFound = false;
        //First we check if the applet is already running and in that case we just change
        //to the containment where the applet is otherwise we add the applet to the current containment.
        foreach( Plasma::Containment *containment, m_runningApplets.keys() )
        {
            if( m_runningApplets[containment].contains( pluginName ) )
            {
                emit changeContainment( containment );
                appletFound = true;
                break;
            }
        }
        if( !appletFound && containment() )
            containment()->addApplet( pluginName );
    }
}


void
AmarokToolBoxMenu::createArrow( ToolBoxIcon *arrow, const QString &direction )
{
    QAction *action = new QAction( "", this );

    if( direction == "up" )
        action->setIcon( KIcon( "arrow-up" ) );
    else
        action->setIcon( KIcon( "arrow-down" ) );

    action->setVisible( true );
    action->setEnabled( true );
    if( direction == "up" )
        connect( action, SIGNAL( triggered() ), this, SLOT( scrollUp() ) );
    else
        connect( action, SIGNAL( triggered() ), this, SLOT( scrollDown() ) );

    arrow->setAction( action );
    arrow->setDrawBackground( false );
    arrow->setOrientation( Qt::Horizontal );

    QSizeF iconSize = arrow->sizeFromIconSize( 22 );

    arrow->setMinimumSize( iconSize );
    arrow->setMaximumSize( iconSize );
    arrow->resize( arrow->size() );

    arrow->setZValue( zValue() + 1 );
    arrow->hide();

}

void
AmarokToolBoxMenu::scrollDown()
{
    DEBUG_BLOCK
    if( !m_bottomMenu.empty() )
    {
        ToolBoxIcon *entryToRemove = m_currentMenu.first();
        m_currentMenu.removeFirst();
        int i = m_menuSize - 1;
        const int height = static_cast<int>( entryToRemove->boundingRect().height() ) + 9;
        m_topMenu.push( entryToRemove->text() );
        delete entryToRemove;
        
        foreach( ToolBoxIcon *entry, m_currentMenu )
        {
            Plasma::Animator::self()->moveItem( entry, Plasma::Animator::SlideInMovement,
                                            QPoint( 5, boundingRect().height() - height * i - 50 ) );
            i--;
        }
        
        ToolBoxIcon *entryToAdd = new ToolBoxIcon( this );
        const QString appletName = m_bottomMenu.pop();
        setupMenuEntry( entryToAdd, appletName );
        m_currentMenu << entryToAdd;
        entryToAdd->setPos( 5, boundingRect().height() - 50 );
        Plasma::Animator::self()->animateItem( entryToAdd, Plasma::Animator::AppearAnimation );

        if( m_bottomMenu.isEmpty() )
            Plasma::Animator::self()->animateItem( m_downArrow, Plasma::Animator::DisappearAnimation );
        
        if( m_topMenu.count() > 0 && !m_upArrow->isVisible() )
        {
            m_upArrow->resetTransform();
            m_upArrow->setPos( boundingRect().width()/2 - m_upArrow->size().width()/2,
                               boundingRect().height() - m_menuSize * height - 50 );
            m_upArrow->show();
        }
    }
}

void
AmarokToolBoxMenu::scrollUp()
{
    DEBUG_BLOCK
    if( !m_topMenu.empty() )
    {
        ToolBoxIcon *entryToRemove = m_currentMenu.last();
        m_currentMenu.removeLast();
        const int height = static_cast<int>( entryToRemove->boundingRect().height() ) + 9;
        m_bottomMenu.push( entryToRemove->text() );
        delete entryToRemove;

        int entries = m_currentMenu.count();
        for( int i = entries - 1; i >= 0; i-- )
        {
            ToolBoxIcon *entry = m_currentMenu[i];
            Plasma::Animator::self()->moveItem( entry, Plasma::Animator::SlideInMovement,
                                            QPoint( 5, boundingRect().height() - height * ( entries - i - 1 ) - 50 ) );
        }

        ToolBoxIcon *entryToAdd = new ToolBoxIcon( this );
        const QString appletName = m_topMenu.pop();
        setupMenuEntry( entryToAdd, appletName );
        m_currentMenu.prepend( entryToAdd );
        entryToAdd->setPos( 5, boundingRect().height() - height * ( m_menuSize - 1 ) - 50 );
        Plasma::Animator::self()->animateItem( entryToAdd, Plasma::Animator::AppearAnimation );

        if( m_topMenu.isEmpty() )
            Plasma::Animator::self()->animateItem( m_upArrow, Plasma::Animator::DisappearAnimation );

        if( m_bottomMenu.count() > 0 && !m_downArrow->isVisible() )
        {
            m_downArrow->resetTransform();
            m_downArrow->show();
        }
    }
}

void
AmarokToolBoxMenu::paint( QPainter *painter,  const QStyleOptionGraphicsItem *option, QWidget *widget )
{
    Q_UNUSED( painter )
    Q_UNUSED( option )
    Q_UNUSED( widget )
}

void
AmarokToolBoxMenu::hoverEnterEvent( QGraphicsSceneHoverEvent *event )
{
    if( m_timer->isActive() )
        m_timer->stop();
    QGraphicsItem::hoverEnterEvent( event );
}

void
AmarokToolBoxMenu::hoverLeaveEvent( QGraphicsSceneHoverEvent *event )
{
   m_timer->start( 2000 );
   QGraphicsItem::hoverLeaveEvent( event );
}

void
AmarokToolBoxMenu::wheelEvent( QGraphicsSceneWheelEvent *event )
{
    DEBUG_BLOCK
    
    if( event->delta() < 0 )
        m_pendingScrolls << ScrollDown;
    else
        m_pendingScrolls << ScrollUp;
    
    if( !m_scrollDelay->isActive() )
        m_scrollDelay->start( m_delay );
}

void
AmarokToolBoxMenu::delayedScroll()
{
    if( m_pendingScrolls.empty() )
        return;
    
    if( m_pendingScrolls.first() == ScrollDown )
        scrollDown();
    else
        scrollUp();
    
    m_pendingScrolls.removeFirst();

    m_scrollDelay->stop();
    if( !m_pendingScrolls.empty() )
        m_scrollDelay->start( qMax( 150, m_delay - m_pendingScrolls.size() * 20 ) );
}

void
AmarokToolBoxMenu::timeToHide()
{
    m_timer->stop();
    hide();
}


}

#include "AmarokToolBox.moc"
