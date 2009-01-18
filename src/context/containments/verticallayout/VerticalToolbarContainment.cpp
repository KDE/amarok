/***************************************************************************
* copyright            : (C) 2007 Leo Franchi <lfranchi@kde.org>           *
**************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "VerticalToolbarContainment.h"

#include "AppletToolbar.h"
#include "ContextView.h"
#include "Debug.h"
#include "VerticalAppletLayout.h"

#include <KConfig>

#include <QGraphicsLinearLayout>

#define TOOLBAR_X_OFFSET 2000

Context::VerticalToolbarContainment::VerticalToolbarContainment( QObject *parent, const QVariantList &args )
    : Containment( parent, args )
    , m_toolbar( 0 )
    , m_applets( 0 )
{    
    setContainmentType( CustomContainment );
    setDrawWallpaper( false );
            
    m_applets = new VerticalAppletLayout( this );
    debug() << "corona at this point:" << corona();
    m_toolbar = new AppletToolbar( this );
    
    // NOTE the toolbar is not set within this view. ToolbarView is actually over the area that the toolbar is placed
    // this is so we can easily get a scolling QGV without having to deal with constant placements of the toolbar
    m_toolbar->setZValue( m_applets->zValue() + 100 );
    m_toolbar->setPos( TOOLBAR_X_OFFSET, 0 );
    debug() << "containment has corona:" << corona();
    
    connect( this, SIGNAL( appletRemoved( Plasma::Applet* ) ), 
             this, SLOT( appletRemoved( Plasma::Applet* ) ) );
    connect( this, SIGNAL( appletRemoved( Plasma::Applet* ) ), 
             this, SIGNAL( geometryChanged() ) );
             
    connect( m_applets,  SIGNAL( appletAdded( Plasma::Applet*, int ) ), 
             m_toolbar,      SLOT( appletAdded( Plasma::Applet*, int) ) );
    connect( m_applets, SIGNAL(  appletAdded( Plasma::Applet*, int ) ), 
             this, SIGNAL( geometryChanged() ) );
             
    connect( m_toolbar, SIGNAL( showApplet( Plasma::Applet* ) ), m_applets, SLOT( showApplet( Plasma::Applet* ) ) );
    connect( m_toolbar, SIGNAL( moveApplet( Plasma::Applet*, int, int ) ), m_applets, SLOT( moveApplet( Plasma::Applet*, int, int ) ) );
    connect( m_toolbar, SIGNAL( addAppletToContainment( const QString&, int ) ), this, SLOT( addApplet( const QString&, int ) ) );
}

Context::VerticalToolbarContainment::~VerticalToolbarContainment()
{
    
}

void 
Context::VerticalToolbarContainment::constraintsEvent( Plasma::Constraints constraints )
{
 //   m_toolbar->setGeometry( 0, contentsRect().height() - 40, contentsRect().width(), 40 );
 //   m_toolbar->setGeometry( contentsRect() );
    m_applets->setGeometry( contentsRect() );
    QRectF geom = m_toolbar->geometry();
    geom.setWidth( contentsRect().width() );
    m_toolbar->setGeometry( geom );
}

QList<QAction*> 
Context::VerticalToolbarContainment::contextualActions()
{
    return QList< QAction* >();
}

void 
Context::VerticalToolbarContainment::paintInterface(QPainter *painter,
                                                      const QStyleOptionGraphicsItem *option,
                                                      const QRect& contentsRect)
{
    
}


void
Context::VerticalToolbarContainment::saveToConfig( KConfigGroup &conf )
{
    m_applets->saveToConfig( conf );
}


void
Context::VerticalToolbarContainment::loadConfig( const KConfigGroup &conf )
{
    DEBUG_BLOCK

    QStringList plugins = conf.readEntry( "plugins", QStringList() );
    debug() << "plugins.size(): " << plugins.size();

    foreach( const QString& plugin, plugins )
    {
        debug() << "Adding applet: " << plugin;
        addApplet( plugin, -1 );
    }
}

void 
Context::VerticalToolbarContainment::setView( ContextView* view )
{
    DEBUG_BLOCK
    m_view = view;
    // kick the toolbar with a real corona no w
    emit updatedContainment( this );
}

Context::ContextView*
Context::VerticalToolbarContainment::view()
{
    return m_view;
}

QRectF 
Context::VerticalToolbarContainment::boundingRect () const
{
    return QRectF( QPointF( 0, 0), m_applets->totalSize() );
}

Plasma::Applet* 
Context::VerticalToolbarContainment::addApplet( const QString& pluginName, const int loc ) // SLOT
{
    DEBUG_BLOCK
    Plasma::Applet* applet = Plasma::Containment::addApplet( pluginName );
    if( applet == 0 )
        debug() << "FAILED ADDING APPLET TO CONTAINMENT!! NOT FOUND!!";
    else
        m_applets->addApplet( applet, loc );
    return applet;
}

void    
Context::VerticalToolbarContainment::appletRemoved( Plasma::Applet* applet )
{
    m_applets->appletRemoved( applet );
    m_toolbar->appletRemoved( applet );
}

void
Context::VerticalToolbarContainment::wheelEvent( QWheelEvent* event )
{
    //eat wheel events, we dont want scrolling
}

#include "VerticalToolbarContainment.moc"
