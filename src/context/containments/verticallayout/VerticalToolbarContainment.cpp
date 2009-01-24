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

#include "ContextView.h"
#include "Debug.h"
#include "VerticalAppletLayout.h"

#include <KConfig>

#include <QGraphicsLinearLayout>

#define TOOLBAR_X_OFFSET 2000

Context::VerticalToolbarContainment::VerticalToolbarContainment( QObject *parent, const QVariantList &args )
    : Containment( parent, args )
    , m_applets( 0 )
{    
    setContainmentType( CustomContainment );
    setDrawWallpaper( false );
            
    m_applets = new VerticalAppletLayout( this );
    
    connect( this, SIGNAL( appletRemoved( Plasma::Applet* ) ), 
             this, SLOT( appletRemoved( Plasma::Applet* ) ) );
    connect( this, SIGNAL( appletRemoved( Plasma::Applet* ) ), 
             this, SIGNAL( geometryChanged() ) );
             
    connect( m_applets,  SIGNAL( appletAdded( Plasma::Applet*, int ) ), 
             this,      SIGNAL( appletAdded( Plasma::Applet*, int) ) ); // goes out to applet toolbar
    connect( m_applets, SIGNAL(  appletAdded( Plasma::Applet*, int ) ), 
             this, SIGNAL( geometryChanged() ) );
            
}

Context::VerticalToolbarContainment::~VerticalToolbarContainment()
{
    
}

void 
Context::VerticalToolbarContainment::constraintsEvent( Plasma::Constraints constraints )
{
    m_applets->setGeometry( contentsRect() );
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
}


void
Context::VerticalToolbarContainment::showApplet( Plasma::Applet* applet )
{
    m_applets->showApplet( applet );
}

void
Context::VerticalToolbarContainment::moveApplet( Plasma::Applet* applet, int a, int b)
{
    m_applets->moveApplet( applet, a, b);
}

void
Context::VerticalToolbarContainment::wheelEvent( QGraphicsSceneWheelEvent* event )
{
    //eat wheel events, we dont want scrolling
}

#include "VerticalToolbarContainment.moc"
