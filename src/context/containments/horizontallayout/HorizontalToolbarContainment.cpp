/***************************************************************************
* copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
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

Context::HorizontalToolbarContainment::HorizontalToolbarContainment( QObject *parent, const QVariantList &args )
    : Containment( parent, args )
    , m_toolbar( 0 )
{
    DEBUG_BLOCK
    
    m_toolbar = new AppletToolbar( this );
}

Context::HorizontalToolbarContainment::~HorizontalToolbarContainment()
{
    
}

void 
Context::HorizontalToolbarContainment::constraintsEvent( Plasma::Constraints constraints )
{
 //   m_toolbar->setGeometry( 0, contentsRect().height() - 40, contentsRect().width(), 40 );
    m_toolbar->setGeometry( contentsRect() );
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
