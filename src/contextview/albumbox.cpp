/***************************************************************************
 * copyright            : (C) 2007 Seb Ruiz <ruiz@kde.org>                 *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "albumbox.h"

#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QGraphicsScene>
#include <QRectF>

using namespace Context;

AlbumItem::AlbumItem( QGraphicsItem *parent, QGraphicsScene *scene )
    : QGraphicsItemGroup( parent, scene )
    , m_coverItem( 0 )
    , m_textItem( 0 )
{
}

void AlbumItem::setCover( const QString &location )
{
    QPixmap image( location );
    setCover( image );
}

void AlbumItem::setCover( const QPixmap &pixmap )
{
    if( !m_coverItem )
    {
        m_coverItem = new QGraphicsPixmapItem( this, scene() );
        m_coverItem->setPos( 0, 0 );
        if( m_textItem )
            m_textItem->setPos( 60, 0 );
    }
    if( !pixmap.isNull() )
        m_coverItem->setPixmap( pixmap );
}

void AlbumItem::setText( const QString &text )
{
    if( !m_textItem )
    {
        m_textItem = new QGraphicsTextItem( this, scene() );
        m_textItem->setPos( m_coverItem ? 60 : 0, 0 );
    }
    m_textItem->setPlainText( text );
}

// finds the lowest point of the group
qreal AlbumItem::bottom()
{
    qreal b = 0;
    if( m_textItem )
        b = m_textItem->boundingRect().bottom();
    if( m_coverItem && m_coverItem->boundingRect().bottom() > b )
        b = m_coverItem->boundingRect().bottom();
    return b;
}

AlbumBox::AlbumBox( QGraphicsItem *parent, QGraphicsScene *scene )
    : ContextBox( parent, scene )
    , m_bottom( 0 )
{
}

void AlbumBox::addAlbumInfo( const QString &pixLocation, const QString &text )
{
    AlbumItem *albumRow = new AlbumItem( m_contentRect );
    albumRow->setCover( pixLocation );
    albumRow->setText( text );

    albumRow->setPos( 0, m_bottom );
    m_bottom += albumRow->bottom();

    setContentRectSize( QSize( boundingRect().width(), m_bottom ) );
}

