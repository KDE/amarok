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

#include <QGraphicsLinearLayout>

Context::VerticalToolbarContainment::VerticalToolbarContainment( QObject *parent, const QVariantList &args )
    : Containment( parent, args )
    , m_mainLayout( 0 )
    , m_toolbar( 0 )
    , m_applets( 0 )
{    
    m_mainLayout = new QGraphicsLinearLayout( Qt::Vertical, this );
        
    m_applets = new VerticalAppletLayout( this );
    debug() << "corona at this point:" << corona();
    m_toolbar = new AppletToolbar( this );
    
    debug() << "containment has corona:" << corona();
    
    m_mainLayout->addItem( m_applets );
    m_mainLayout->addItem( m_toolbar );
    
    connect( this, SIGNAL( appletAdded( Plasma::Applet*, const QPointF & ) ),
             this, SLOT( addApplet( Plasma::Applet*, const QPointF & ) ) );
}

Context::VerticalToolbarContainment::~VerticalToolbarContainment()
{
    
}

void 
Context::VerticalToolbarContainment::constraintsEvent( Plasma::Constraints constraints )
{
 //   m_toolbar->setGeometry( 0, contentsRect().height() - 40, contentsRect().width(), 40 );
 //   m_toolbar->setGeometry( contentsRect() );
    m_mainLayout->setGeometry( contentsRect() );
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
    
}

void 
Context::VerticalToolbarContainment::loadConfig( const KConfigGroup &conf )
{
    
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

Plasma::Applet* 
Context::VerticalToolbarContainment::addApplet( Plasma::Applet* applet, const QPointF & )
{
    DEBUG_BLOCK
    // TODO for now just put the applets as placeholders in the CV
    m_applets->addApplet( applet, -1 );
}

#include "VerticalToolbarContainment.moc"
