/***************************************************************************
* copyright            : (C) 2008 Leo Franchi <lfranchi@kde.org>         *
****************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "VerticalAppletLayout.h"

#include "Containment.h"
#include "Debug.h"
#include "plasma/applet.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneResizeEvent>

Context::VerticalAppletLayout::VerticalAppletLayout( QGraphicsItem* parent )
    : QGraphicsWidget( parent )
    , m_showingIndex( -1 )
{
}

Context::VerticalAppletLayout::~VerticalAppletLayout()
{
    
}

void 
Context::VerticalAppletLayout::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget )
{
  //  DEBUG_BLOCK
    
 //   debug() << "drawing rect:" << boundingRect();
    painter->save();
    painter->setPen( QColor( Qt::green ) );   
    painter->setOpacity( 0.75 ); 
    painter->drawRect( boundingRect() );
    painter->restore();
}

void
Context::VerticalAppletLayout::resizeEvent( QGraphicsSceneResizeEvent * event )
{
    
}

void 
Context::VerticalAppletLayout::addApplet( Plasma::Applet* applet, int location )
{
    DEBUG_BLOCK
    if( location < 0 ) // being told to add at end
    {
        m_appletList << applet;
        if( m_showingIndex < 0 ) //first applet to be added, nothing being shown yet
        {
            showAtIndex( 0 );
        } else
        {
            applet->hide();
        }
    }
}

void 
Context::VerticalAppletLayout::showAtIndex( int index )
{
    if( index < 0 )
        return;
        
    // for  now we are just maximising the currently shown applet
    for( int i = 0; i > m_appletList.size(); i++ )
    {
        if( i != index )
            m_appletList[ i ]->hide();
    }
    m_appletList[ index ]->setPos( 0, 0 );
    m_appletList[ index ]->resize( size() );
    m_showingIndex = 0;
}

#include "VerticalAppletLayout.moc"
