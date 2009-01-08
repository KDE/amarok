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
  /*  painter->save();
    painter->setPen( QColor( Qt::green ) );   
    painter->setOpacity( 0.75 ); 
    painter->drawRect( boundingRect() );
    painter->restore();
    */
}

void
Context::VerticalAppletLayout::resizeEvent( QGraphicsSceneResizeEvent * event )
{
    // update all the applet widths
    foreach( Plasma::Applet* applet, m_appletList )
        applet->resize( event->newSize().width(), applet->size().height() );
}

void 
Context::VerticalAppletLayout::addApplet( Plasma::Applet* applet, int location )
{
    DEBUG_BLOCK
    debug() << "layout told to add applet at" << location;
    if( location < 0 ) // being told to add at end
    {
        m_appletList << applet;
        showAtIndex( m_appletList.size() - 1 );
        location = m_appletList.size() - 1; // so the signal has the correct location
    } else
    {
        m_appletList.insert( location, applet );
        showAtIndex( location );
    }
    debug() << "emitting addApplet with location" << location;
    emit appletAdded( applet, location );
}

void 
Context::VerticalAppletLayout::showApplet( Plasma::Applet* applet ) // SLOT
{
    showAtIndex( m_appletList.indexOf( applet ) );
}


void 
Context::VerticalAppletLayout::appletRemoved( Plasma::Applet* app )
{
    DEBUG_BLOCK
    int removedIndex = m_appletList.indexOf( app );
    debug() << "removing applet at index:" << removedIndex;
    m_appletList.removeAll( app );
    if( m_showingIndex > removedIndex )
        m_showingIndex--;
    showAtIndex( m_showingIndex );
}

void
Context::VerticalAppletLayout::showAtIndex( int index )
{
    DEBUG_BLOCK
    if( index < 0 || index > m_appletList.size() )
        return;
    
    qreal runningHeight = 0.0, currentHeight = 0.0;
    qreal width =  boundingRect().width();
    debug() << "showing applet at index" << index;
    debug() << "using applet width of " << width;
    for( int i = index - 1; i >= 0; i-- ) // lay out backwards above the view
    {
        debug() << "UPWARDS dealing with" << m_appletList[ i ]->name();
        currentHeight = m_appletList[ i ]->effectiveSizeHint( Qt::PreferredSize, QSizeF( width, -1 ) ).height();
        runningHeight -= currentHeight;
        m_appletList[ i ]->setPos( 0, runningHeight );
        debug() << "UPWARDS putting applet #" << i << " at" << 0 << runningHeight;
        debug() << "UPWARDS got applet sizehint height:" << currentHeight;
        m_appletList[ i ]->resize( width, currentHeight );
        m_appletList[ i ]->show();
    }
    runningHeight = currentHeight = 0.0;
    for( int i = index; i < m_appletList.size(); i++ ) // now lay out desired item at top and rest below it
    {
        debug() << "dealing with" << m_appletList[ i ]->name();
        debug() << "putting applet #" << i << " at" << 0 << runningHeight;
        m_appletList[ i ]->setPos( 0, runningHeight );
        currentHeight = m_appletList[ i ]->effectiveSizeHint( Qt::PreferredSize, QSizeF( width, -1 ) ).height();
        runningHeight += currentHeight;
        debug() << "next applet will go at:" << runningHeight;
        debug() << "got applet sizehint height:" << currentHeight;
        m_appletList[ i ]->resize( width, currentHeight );
        m_appletList[ i ]->show();
    }
    
    m_showingIndex = index;
}

#include "VerticalAppletLayout.moc"
