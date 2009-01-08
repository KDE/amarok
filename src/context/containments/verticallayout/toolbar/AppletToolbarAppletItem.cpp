/**************************************************************************
* copyright            : (C) 2008 Leo Franchi <lfranchi@kde.org  >        *
**************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "AppletToolbarAppletItem.h"

#include "Debug.h"

#include "plasma/applet.h"

#include <QStyleOptionGraphicsItem>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>


Context::AppletToolbarAppletItem::AppletToolbarAppletItem( QGraphicsItem* parent, Plasma::Applet* applet )
    : QGraphicsWidget( parent )
    , m_applet( applet )
    , m_label( 0 )
    , m_labelPadding( 5 )
{
    m_label = new QGraphicsSimpleTextItem( this );
    if( m_applet )
       m_label->setText( m_applet->name() );
    else
        m_label->setText( "no applet name" );
        
}

Context::AppletToolbarAppletItem::~AppletToolbarAppletItem()
{
    
}

void 
Context::AppletToolbarAppletItem::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget )
{
  //  DEBUG_BLOCK
    painter->save();
    QColor fillColor( 102, 102, 102, 210 );
    QPainterPath fillPath;
    fillPath.addRoundedRect( boundingRect(), 5, 5 );
    painter->fillPath( fillPath ,fillColor );
    painter->restore();
}

QSizePolicy 
Context::AppletToolbarAppletItem::sizePolicy () const
{
    return QSizePolicy( QSizePolicy::Expanding,  QSizePolicy::Expanding );
}

void 
Context::AppletToolbarAppletItem::resizeEvent( QGraphicsSceneResizeEvent * event )
{
    m_label->setPos( ( boundingRect().width() / 2 ) - ( m_label->boundingRect().width() / 2 ),  ( boundingRect().height() / 2 ) - ( m_label->boundingRect().height() / 2 ) );
}

QSizeF 
Context::AppletToolbarAppletItem::sizeHint( Qt::SizeHint which, const QSizeF & constraint ) const
{
    if( which == Qt::MinimumSize )
        return QSizeF( m_label->boundingRect().width() + 2 * m_labelPadding, QGraphicsWidget::sizeHint( which, constraint ).height() );
    else
        return QGraphicsWidget::sizeHint( which, constraint );
}

void 
Context::AppletToolbarAppletItem::mousePressEvent( QGraphicsSceneMouseEvent * event )
{
    emit appletChosen( m_applet );
}

#include "AppletToolbarAppletItem.moc"
