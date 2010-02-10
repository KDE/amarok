/****************************************************************************************
 * Copyright (c) 2008 William Viana Soares <vianasw@gmail.com>                          *
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

#include "ToolBoxMenu.h"

#include "Debug.h"

#include <plasma/applet.h>
#include <plasma/corona.h>
#include <Plasma/Animator>
#include <Plasma/Animation>

#include <kicon.h>

#define ENTRY_HEIGHT  32
#define ENTRY_WIDTH  180
#define ENTRY_MARGIN   5
#define OFFSET_Y      40

namespace Context
{

AmarokToolBoxMenu::AmarokToolBoxMenu( QGraphicsItem *parent, bool runningAppletsOnly )
    : QObject()
    , QGraphicsItem( parent )
    , m_containment( 0 )
    , m_removeApplets( false )
    , m_menuSize( 4 )
    , m_installScriptedApplet( 0 )
    , m_showing( 0 )
    , m_delay( 250 )
{
    QMap< QString, QString > allApplets;
    QStringList appletsToShow; // we need to store the list of
    // all appletname->appletpluginname pairs, even if we do not show them

    foreach ( const KPluginInfo& info, Plasma::Applet::listAppletInfo( QString(), "amarok" ) )
    {
        if ( info.property( "NoDisplay" ).toBool() )
        {
            // we don't want to show the hidden category
            continue;
        }

        allApplets.insert( info.name(), info.pluginName() );
        if( !runningAppletsOnly )
            appletsToShow << info.name();
    }

    if( runningAppletsOnly )
    {
       m_removeApplets = true;
       Containment* cont = dynamic_cast<Containment *>( parent );
       if( cont )
       {
           foreach( Plasma::Applet* applet, cont->applets() )
           {
               appletsToShow << applet->name();
           }
       }
    }

    init( allApplets, appletsToShow );
}

AmarokToolBoxMenu::~AmarokToolBoxMenu()
{}

void
AmarokToolBoxMenu::init( QMap< QString, QString > allApplets, QStringList appletsToShow )
{
    setAcceptsHoverEvents( true );

    m_appletsList = allApplets;

    m_timer = new QTimer( this );
    m_scrollDelay = new QTimer( this );

    connect( m_timer, SIGNAL( timeout() ), this, SLOT( timeToHide() ) );
    connect( m_scrollDelay, SIGNAL( timeout() ), this, SLOT( delayedScroll() ) );

    //insert in the stack so the first applet in alphabetical order is the first
    appletsToShow.sort();
    for( int i = appletsToShow.size() - 1; i >= 0; i-- )
        m_bottomMenu.push( appletsToShow[i] );

    m_hideIcon = new ToolBoxIcon( this );

    QAction *hideMenu = new QAction( this );
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

    m_hideIcon->setPos( 5, boundingRect().height() - ( ENTRY_HEIGHT + ENTRY_MARGIN ) * m_menuSize - OFFSET_Y + ENTRY_MARGIN * 2 );
    m_hideIcon->setZValue( zValue() + 1 );
    m_hideIcon->hide();

    m_upArrow = new ToolBoxIcon( this );
    m_downArrow = new ToolBoxIcon( this );
    createArrow( m_upArrow, "up" );
    createArrow( m_downArrow, "down" );

/* TODO disabled adding scripted applets. framework is in place, but needs some more 
    polish before it's ready for a release. so, turning it off for now. 
    m_installScriptedApplet = new ToolBoxIcon( this );
    m_installScriptedApplet->setDrawBackground( true );
    m_installScriptedApplet->setOrientation( Qt::Horizontal );
    m_installScriptedApplet->setText( i18n( "Install New Applets" ) );
    const QSizeF size( ENTRY_WIDTH - 60, ENTRY_HEIGHT - 9 );
    m_installScriptedApplet->setMinimumSize( size );
    m_installScriptedApplet->setMaximumSize( size );
    m_installScriptedApplet->resize( size );
    m_installScriptedApplet->setZValue( zValue() + 1 );
    m_installScriptedApplet->hide();

    connect( m_installScriptedApplet, SIGNAL( clicked() ), this, SIGNAL( installApplets() ) );
    */
}

void
AmarokToolBoxMenu::setContainment( Containment *newContainment )
{
    if( m_containment != newContainment )
    {
        Plasma::Corona *corona = newContainment->corona();
        if( !corona )
            return;

        //disconnect containments just in case we use setContainment somewhere else and we end up with
        //more signals connected that we wanted
        QList<Plasma::Containment *> containments = corona->containments();
        foreach( Plasma::Containment *containment, containments )
        {
            disconnect( containment, SIGNAL( appletAdded( Plasma::Applet *, QPointF ) ),
                 this, SLOT( appletAdded( Plasma::Applet *) ) );
            disconnect( containment, SIGNAL( appletRemoved( Plasma::Applet * ) ),
                 this, SLOT( appletRemoved( Plasma::Applet * ) ) );
        }
        m_containment = newContainment;
        initRunningApplets();
        populateMenu();
    }
}

Containment *
AmarokToolBoxMenu::containment() const
{
    return m_containment;
}

QRectF
AmarokToolBoxMenu::boundingRect() const
{
    return QRectF( QPointF( 0, 0 ), QSize( ENTRY_WIDTH + 5, ( ENTRY_HEIGHT +  ENTRY_MARGIN ) * ( m_menuSize + 2 ) ) );
}

void
AmarokToolBoxMenu::populateMenu()
{
    for( int i = 0; i < m_menuSize; i++ )
    {
        if( m_bottomMenu.isEmpty() )
            continue;
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
    if( !m_containment )
        return;

    Plasma::Corona *corona = m_containment->corona();

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
    if( m_removeApplets )
    {
        m_menuSize = qMin( 4, containment()->applets().size() );
        m_hideIcon->setPos( 5, boundingRect().height() - ( ENTRY_HEIGHT + ENTRY_MARGIN ) * m_menuSize - OFFSET_Y + ENTRY_MARGIN * 2 );
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
            if( m_removeApplets )
            {
                m_menuSize = qMin( 4, this->containment()->applets().size() );
            }
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
            if( m_removeApplets )
            {
                m_menuSize = qMin( 4, this->containment()->applets().size() );
                if( m_menuSize == 0 )
                    hide();
            }
        }
    }
}

bool
AmarokToolBoxMenu::showing() const
{
    return m_showing;
}

void
AmarokToolBoxMenu::repopulateMenu()
{
    m_bottomMenu.clear();
    m_topMenu.clear();
    m_currentMenu.clear();
    foreach( Plasma::Applet* applet, containment()->applets() )
    {
        m_bottomMenu.push( applet->name() );
    }
    populateMenu();
}

void
AmarokToolBoxMenu::show( bool refreshApplets )
{
    if( showing() )
        return;

    if( m_timer->isActive() )
        m_timer->stop();

    m_showing = true;

    if( m_removeApplets && refreshApplets ) // we need to refresh on view to get all running applets
        repopulateMenu();

    if( m_bottomMenu.count() > 0 )
    {
        m_downArrow->setPos( boundingRect().width() / 2 - m_downArrow->size().width()/2,
                            boundingRect().height() - 10 );
        m_downArrow->resetTransform();
        m_downArrow->show();
    }

    if( m_topMenu.count() > 0 )
    {
        const int height = static_cast<int>( m_currentMenu.first()->boundingRect().height() ) + ENTRY_MARGIN;
        m_upArrow->resetTransform();
        m_upArrow->setPos( boundingRect().width()/2 - m_upArrow->size().width()/2,
                            boundingRect().height() - m_menuSize * height - OFFSET_Y + ENTRY_MARGIN * 2 );
        m_upArrow->show();
    }

    m_hideIcon->setPos( 5, boundingRect().height() - ( ENTRY_HEIGHT + ENTRY_MARGIN ) * m_menuSize - OFFSET_Y + ENTRY_MARGIN * 2 );
    m_hideIcon->show();
    setZValue( zValue() + 10000 );

#if 0
    /* TODO disabled adding scripted applets */
    m_installScriptedApplet->setPos( 30, boundingRect().height() - ( ENTRY_HEIGHT + ENTRY_MARGIN ) * m_menuSize - OFFSET_Y + ENTRY_MARGIN * 2 );
    m_installScriptedApplet->show();
#endif

    for( int i = m_currentMenu.count() - 1; i >= 0; i-- )
    {
        ToolBoxIcon *entry = m_currentMenu[m_currentMenu.count() - i - 1];
        entry->show();
        const int height = static_cast<int>( entry->boundingRect().height() ) + ENTRY_MARGIN;

        Plasma::Animation *entryAnimation = m_entryAnimation.data();
        if( !entryAnimation )
        {
            entryAnimation = Plasma::Animator::create( Plasma::Animator::SlideAnimation );
            entryAnimation->setTargetWidget( entry );
            entryAnimation->setProperty( "direction", Plasma::Animation::MoveAny );
            m_entryAnimation = entryAnimation;
        }
        else
            entryAnimation->stop();

        entryAnimation->setProperty( "distancePointF", QPointF( 5, boundingRect().height() - height * i - OFFSET_Y ) );
        entryAnimation->start( QAbstractAnimation::DeleteWhenStopped );
    }
}

void
AmarokToolBoxMenu::hide()
{
    if( !showing() )
        return;

    if( m_timer->isActive() )
        m_timer->stop();

    setZValue( zValue() - 10000 );
    m_showing = false;
    foreach( QGraphicsItem *c, QGraphicsItem::children() )
    {
        c->hide();
    }

    emit menuHidden();
}

void
AmarokToolBoxMenu::setupMenuEntry( ToolBoxIcon *entry, const QString &appletName )
{
    entry->setDrawBackground( true );
    entry->setOrientation( Qt::Horizontal );
    entry->setText( appletName );

    const QSizeF size( ENTRY_WIDTH, ENTRY_HEIGHT );
    entry->setMinimumSize( size );
    entry->setMaximumSize( size );
    entry->resize( size );

    entry->setPos( 5, boundingRect().height() );

    entry->setZValue( zValue() + 1 );
    entry->setData( 0, QVariant( m_appletsList[appletName] ) );
    entry->show();
    if( m_removeApplets )
    {
        connect( entry, SIGNAL( appletChosen( const QString & ) ),
                 this, SLOT( removeApplet( const QString & ) ) );
    } else
    {
        connect( entry, SIGNAL( appletChosen( const QString & ) ), this, SLOT( addApplet( const QString & ) ) );
    }
}

void
AmarokToolBoxMenu::addApplet( const QString &pluginName )
{
    DEBUG_BLOCK
    if( !pluginName.isEmpty() && containment() )
    {
        // we can't add directly b/c we may want to say at what location, so we just report
        //containment()->addApplet( pluginName );
        emit addAppletToContainment( pluginName );
    }
}

void
AmarokToolBoxMenu::removeApplet( const QString& pluginName )
{
    if( pluginName.isEmpty()  )
        return;

    // this is not ideal, but we look through all running applets to find
    // the one that we want
    foreach( Plasma::Applet* applet, containment()->applets() )
    {
        if( applet->pluginName() == pluginName )
        {
            // this is the applet we want to remove
            applet->destroy();
            // the rest of the cleanup should happen in
            // appletRemoved, which is called by the containment
        }
    }
    // get the name from the pluginName, and remove from the menu
    foreach( ToolBoxIcon* entry, m_currentMenu )
    {
        if( entry->data( 0 ) == pluginName )
            m_currentMenu.removeAll( entry );
    }
    hide();
    show( false );
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
    if( !m_bottomMenu.empty() )
    {
        ToolBoxIcon *entryToRemove = m_currentMenu.first();
        m_currentMenu.removeFirst();
        int i = m_menuSize - 1;
        const int height = static_cast<int>( entryToRemove->boundingRect().height() ) + ENTRY_MARGIN;
        m_topMenu.push( entryToRemove->text() );
        delete entryToRemove;

        foreach( ToolBoxIcon *entry, m_currentMenu )
        {
            Plasma::Animation *entryAnimation = m_entryAnimation.data();
            if( !entryAnimation )
            {
                entryAnimation = Plasma::Animator::create( Plasma::Animator::SlideAnimation );
                entryAnimation->setTargetWidget( entry );
                entryAnimation->setProperty( "direction", Plasma::Animation::MoveAny );
            }
            else
                entryAnimation->stop();

            entryAnimation->setProperty( "distancePointF", QPointF( 5, boundingRect().height() - height * i - OFFSET_Y ) );
            entryAnimation->start( QAbstractAnimation::DeleteWhenStopped );
            i--;
        }

        ToolBoxIcon *entryToAdd = new ToolBoxIcon( this );
        const QString appletName = m_bottomMenu.pop();
        setupMenuEntry( entryToAdd, appletName );
        m_currentMenu << entryToAdd;
        entryToAdd->setPos( 5, boundingRect().height() - OFFSET_Y );

        Plasma::Animation *newEntryAnimation = m_newEntryAnimation.data();
        if( !newEntryAnimation )
        {
            newEntryAnimation = Plasma:Animator::create( Plasma::Animator::FadeAnimation );
            newEntryAnimation->setTargetWidget( entryToAdd );
            newEntryAnimation->setProperty( "startOpacity", 0.0 );
            newEntryAnimation->setProperty( "targetOpacity", 1.0 );
        }
        else
            newEntryAnimation->stop();

        newEntryAnimation->start( QAbstractAnimation::DeleteWhenStopped );

        if( m_bottomMenu.isEmpty() ) {
            Plasma::Animation *downArrowAnimation = m_downArrowAnimation.data();
            if( !downArrowAnimation )
            {
                downArrowAnimation = Plasma::Animator::create( Plasma::Animator::FadeAnimation );
                downArrowAnimation->setTargetWidget( m_downArrow );
                downArrowAnimation->setProperty("startOpacity", 1.0);
                downArrowAnimation->setProperty("targetOpacity", 0.0);
                m_downArrowAnimation = downArrowAnimation;
            }

            downArrowAnimation->start( QAbstractAnimation::DeleteWhenStopped );
        }

        if( m_topMenu.count() > 0 && !m_upArrow->isVisible() )
        {
            m_upArrow->resetTransform();
            m_upArrow->setPos( boundingRect().width()/2 - m_upArrow->size().width()/2,
                               boundingRect().height() - m_menuSize * height - OFFSET_Y + ENTRY_MARGIN * 2 );
            m_upArrow->show();
        }
    }
}

void
AmarokToolBoxMenu::scrollUp()
{
    if( !m_topMenu.empty() )
    {
        ToolBoxIcon *entryToRemove = m_currentMenu.last();
        m_currentMenu.removeLast();
        const int height = static_cast<int>( entryToRemove->boundingRect().height() ) + ENTRY_MARGIN;
        m_bottomMenu.push( entryToRemove->text() );
        delete entryToRemove;

        int entries = m_currentMenu.count();
        for( int i = entries - 1; i >= 0; i-- )
        {
            ToolBoxIcon *entry = m_currentMenu[i];
            Plasma::Animation *entryAnimation = m_entryAnimation.data();
            if( !entryAnimation )
            {
                entryAnimation = Plasma::Animator::create( Plasma::Animator::SlideAnimation );
                entryAnimation->setTargetWidget( entry );
                entryAnimation->setProperty( "direction", Plasma::Animation::MoveAny );
                m_entryAnimation = entryAnimation;
            }
            else
                entryAnimation->stop();

            entryAnimation->setProperty( "distancePointF", QPointF( 5, boundingRect().height() - height * ( entries - i - 1 ) - OFFSET_Y ) );
            entryAnimation->start( QAbstractAnimation::DeleteWhenStopped );
        }

        ToolBoxIcon *entryToAdd = new ToolBoxIcon( this );
        const QString appletName = m_topMenu.pop();
        setupMenuEntry( entryToAdd, appletName );
        m_currentMenu.prepend( entryToAdd );
        entryToAdd->setPos( 5, boundingRect().height() - height * ( m_menuSize - 1 ) - OFFSET_Y );

        Plasma::Animation *newEntryAnimation = m_newEntryAnimation.data();
        if( !newEntryAnimation )
        {
            newEntryAnimation = Plasma:Animator::create( Plasma::Animator::FadeAnimation );
            newEntryAnimation->setTargetWidget( entryToAdd );
            newEntryAnimation->setProperty( "startOpacity", 0.0 );
            newEntryAnimation->setProperty( "targetOpacity", 1.0 );
        }
        else
            newEntryAnimation->stop();

        newEntryAnimation->start( QAbstractAnimation::DeleteWhenStopped );

        if( m_topMenu.isEmpty() )
        {
            Plasma::Animation *upArrowAnimation = m_upArrowAnimation.data();
            if( !upArrowAnimation )
            {
                upArrowAnimation = Plasma::Animator::create( Plasma::Animator::FadeAnimation );
                upArrowAnimation->setTargetWidget( m_upArrow );
                upArrowAnimation->setProperty("startOpacity", 0.0);
                upArrowAnimation->setProperty("targetOpacity", 1.0);
                upArrowAnimation->setDirection( QAbstractAnimation::Backward );
                m_upArrowAnimation = upArrowAnimation;
            }

            upArrowAnimation->start( QAbstractAnimation::DeleteWhenStopped );
        }

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
    if( event->orientation() == Qt::Horizontal || !showing() )
        return;
    if( m_pendingScrolls.size() > 0 )
    {
        if( m_pendingScrolls.last() == ScrollDown && event->delta() > 0 )
            m_pendingScrolls.clear();
        else if( m_pendingScrolls.last() == ScrollUp && event->delta() < 0 )
            m_pendingScrolls.clear();
    }

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

#include "ToolBoxMenu.moc"
