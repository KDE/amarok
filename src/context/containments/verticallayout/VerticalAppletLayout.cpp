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

#include <KConfig>

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
    showAtIndex( m_showingIndex );
    
}

void 
Context::VerticalAppletLayout::addApplet( Plasma::Applet* applet, int location )
{
    DEBUG_BLOCK
    debug() << "layout told to add applet at" << location;
    if( location < 0 ) // being told to add at end
    {
        m_appletList << applet;
        showAtIndex( minIndexWithAppletOnScreen ( m_appletList.size() - 1 ) );
        location = m_appletList.size() - 1; // so the signal has the correct location
    } else
    {
        m_appletList.insert( location, applet );
        showAtIndex( minIndexWithAppletOnScreen ( location ) );
    }
    debug() << "emitting addApplet with location" << location;
    emit appletAdded( applet, location );
}


void
Context::VerticalAppletLayout::saveToConfig( KConfigGroup &conf )
{
    DEBUG_BLOCK
    QStringList plugins;

    for( int i = 0; i < m_appletList.size(); i++ )
    {
        Plasma::Applet *applet = m_appletList[ i ];
        if( applet != 0 )
        {
            debug() << "saving applet" << applet->name();
            plugins << applet->pluginName();
        }
        conf.writeEntry( "plugins", plugins );
    }
    conf.writeEntry( "firstShowingApplet", m_showingIndex );
}

void
Context::VerticalAppletLayout::refresh()
{
    showAtIndex( m_showingIndex );
}

QSizeF 
Context::VerticalAppletLayout::totalSize()
{
    QSizeF sizeR( boundingRect().width(), 0 );
    qreal size = 0.0;
    foreach( Plasma::Applet* applet, m_appletList )
        size += applet->effectiveSizeHint( Qt::PreferredSize, QSizeF( boundingRect().width(), -1 ) ).height();
    sizeR.setHeight( size );
    return sizeR;
}


void 
Context::VerticalAppletLayout::showApplet( Plasma::Applet* applet ) // SLOT
{
    showAtIndex( m_appletList.indexOf( applet ) );
}


void 
Context::VerticalAppletLayout::moveApplet( Plasma::Applet* applet, int oldLoc, int newLoc)
{
    DEBUG_BLOCK
    // if oldLoc is -1 we search for the applet to get the real location
    if( oldLoc == -1 )
        oldLoc = m_appletList.indexOf( applet );
    if( oldLoc == -1 )
        debug() << "COULDN'T FIND APPLET IN LIST!";
    
 //   debug() << "moving applet in layout from" << oldLoc << "to" << newLoc;
        
    if( oldLoc <  0 || oldLoc > m_appletList.size() - 1 || newLoc < 0 || newLoc > m_appletList.size() || oldLoc == newLoc )
        return;
    m_appletList.insert( newLoc, m_appletList.takeAt( oldLoc ) );
    showAtIndex( minIndexWithAppletOnScreen( qMin( oldLoc, newLoc ) ) );
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
    showAtIndex( minIndexWithAppletOnScreen( m_showingIndex ) );
}

void
Context::VerticalAppletLayout::showAtIndex( int index )
{
    DEBUG_BLOCK
    if( index < 0 || index > m_appletList.size() )
        return;
    
    prepareGeometryChange();
    
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

int 
Context::VerticalAppletLayout::minIndexWithAppletOnScreen( int loc )
{
    DEBUG_BLOCK
    qreal height = 0.0;
    int index = -1;
    if( boundingRect().height() == 0 || 
      ( m_appletList.size() == 0     || loc > m_appletList.size() - 1 ) ) // if we have a 0 height this is b/c we are starting up and don't have a real size yet
        return 0;                      // for now just show all the applets 
    for( int i = loc; i >= 0; i-- )
    {    
        index = i;
        debug() << "height:" << height;
        qreal curHeight = m_appletList[ i ]->effectiveSizeHint( Qt::PreferredSize, QSizeF( boundingRect().width(), -1 ) ).height();
        debug() << "calculating:" << curHeight << " + " << height << " > " << boundingRect().height();
        if( ( curHeight + height ) > boundingRect().height() )
            break;
            
        height += curHeight;
    }
    return index;
}

#include "VerticalAppletLayout.moc"
