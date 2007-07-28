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

#include "LastFmEventBox.h"

#include "../contextview.h"
#include "debug.h"

#include <klocale.h>

#include <QFontMetrics>
#include <QRegExp>

using namespace Context;

//////////////////////////////////////////////////////////////////////
//// CLASS LastFmEventMember
//////////////////////////////////////////////////////////////////////


LastFmEventMember::LastFmEventMember( QGraphicsItem * parent, LastFmEvent event, qreal top ) 
    : QGraphicsRectItem( parent )
    , m_left( 0 )
    , m_right( 0 )
{
    debug() << "generating event item for: " << event.title << " at location: " << top << endl;
    m_left = new QGraphicsTextItem( "", this );
    m_right = new QGraphicsTextItem( "", this );
    
    // get our bounding rect size first, so we can format properly
    qreal width = -1;
    width = parent->boundingRect().width();
    QFontMetrics* fm = new QFontMetrics( QFont() ); // get some font sizes
    qreal titleWidth = (qreal)fm->width( event.title );
    qreal locWidth = (qreal)fm->width( event.location );
    if( titleWidth > ( width - locWidth ) ) // title won't fit
        event.title = shrinkText( event.title, width - locWidth );
    m_left->setHtml( QString( "<b>%1</b><br>%2" ).arg( event.title, event.date ) );
    m_left->setPos( 0.0, 0.0 );
    m_right->setHtml( QString( "%1<br>%2" ).arg( event.location, event.city ) );
    m_right->setPos( width - locWidth, 0.0 );
    
    setRect( 0, top, width, qMax( m_left->boundingRect().height(), m_right->boundingRect().height() ) );
    
}    

void LastFmEventMember::ensureWidthFits( qreal width )
{
    Q_UNUSED( width );
    qreal right = m_right->boundingRect().width();  
    m_right->setPos( parentItem()->boundingRect().width() - right, 0.0 );
}

QString LastFmEventMember::shrinkText( QString text, const qreal length )
{
    QFontMetrics* fm = new QFontMetrics( QFont() );
    while( fm->width( text ) > length ) // chop a word off the end and try again
        text = text.left( text.length() - text.lastIndexOf( " " ) ) + "...";
    delete fm;
    return text;
}
    
/////////////////////////////////////////////////////////////////////////
//// CLASS LastFmEventBox
/////////////////////////////////////////////////////////////////////////

LastFmEventBox::LastFmEventBox( QGraphicsItem *parent, QGraphicsScene *scene ) 
    : ContextBox( parent, scene ) 
{}

void LastFmEventBox::setEvents( QList< LastFmEvent >* events )
{    
    if( events->size() == 0 )
    {
        QGraphicsTextItem* box = new QGraphicsTextItem( "", this );
        box->setHtml( "<html><body>" + i18n( "You are attending no events in the future!" ) + "</body></html>" );
        setContentHeight( box->boundingRect().height() );
        return;
    }
    
    int count = 0; 
    qreal bottom = 0.0;// bottom keeps track of the bottom of the last item 
    // in the list, so we don't need to calculate it on every iteration
    foreach( LastFmEvent event, *events )
    {
        count++;
        if( count >= 6 ) 
        {
            break; // only show 5 to keep it neat and small.
        }
        LastFmEventMember* member = new LastFmEventMember( m_contentRect, event, bottom );
        member->setPos( 0.0, bottom );
        m_events << member;
        bottom += member->rect().height();
    }
    setContentHeight( bottom );
}

void LastFmEventBox::clearContents()
{
    foreach( QGraphicsItem* item, m_contentRect->children() )
    {
        delete item;
    }
}

void LastFmEventBox::setContentHeight( qreal height )
{
    DEBUG_BLOCK
    debug() << "changing content height from: " << m_contentRect->rect().height() << " to: " << height << endl;
    QSizeF size( m_contentRect->rect().width(), height );
    setContentRectSize( size, false );
}

void LastFmEventBox::ensureWidthFits( const qreal width )
{    
    const qreal padding = ContextView::BOX_PADDING * 2;
    const qreal height  = m_contentRect->boundingRect().height();
    
    QSizeF newSize = QSizeF( width - padding, height );
    setContentRectSize( newSize, false );
    
    foreach( LastFmEventMember* event, m_events )
        event->ensureWidthFits( width );
}

#include "LastFmEventBox.moc"
