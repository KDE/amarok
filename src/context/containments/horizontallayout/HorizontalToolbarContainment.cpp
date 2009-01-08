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

#include "HorizontalToolbarContainment.h"

#include "AppletToolbar.h"
#include "ContextView.h"
#include "Debug.h"
#include "HorizontalAppletLayout.h"

#include <QGraphicsLinearLayout>

Context::HorizontalToolbarContainment::HorizontalToolbarContainment( QObject *parent, const QVariantList &args )
    : Containment( parent, args )
    , m_mainLayout( 0 )
    , m_toolbar( 0 )
    , m_applets( 0 )
{    
    m_mainLayout = new QGraphicsLinearLayout( Qt::Vertical, this );
        
    m_toolbar = new AppletToolbar( 0 );
    m_applets = new HorizontalAppletLayout( 0 );
    
    m_mainLayout->addItem( m_applets );
    m_mainLayout->addItem( m_toolbar );
}

Context::HorizontalToolbarContainment::~HorizontalToolbarContainment()
{
    
}

void 
Context::HorizontalToolbarContainment::constraintsEvent( Plasma::Constraints constraints )
{
 //   m_toolbar->setGeometry( 0, contentsRect().height() - 40, contentsRect().width(), 40 );
 //   m_toolbar->setGeometry( contentsRect() );
    m_mainLayout->setGeometry( contentsRect() );
}

QList<QAction*> 
Context::HorizontalToolbarContainment::contextualActions()
{
    
}

void 
Context::HorizontalToolbarContainment::paintInterface(QPainter *painter,
                                                      const QStyleOptionGraphicsItem *option,
                                                      const QRect& contentsRect)
{
    
}

void 
Context::HorizontalToolbarContainment::saveToConfig( KConfigGroup &conf )
{
    
}

void 
Context::HorizontalToolbarContainment::loadConfig( const KConfigGroup &conf )
{
    
}

Context::ContextView*
Context::HorizontalToolbarContainment::view()
{
    
}

#include "HorizontalToolbarContainment.moc"
