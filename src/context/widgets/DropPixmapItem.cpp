/****************************************************************************************
 * Copyright (c) 2009 Simon Esneault <simon.esneault@gmail.com>                         *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "DropPixmapItem"

#include "DropPixmapItem.h"

#include "core/support/Debug.h"

#include <QGraphicsSceneDragDropEvent>
#include <QMimeData>

DropPixmapItem::DropPixmapItem( QGraphicsItem* parent )
    : QGraphicsPixmapItem( parent )
{
    setAcceptDrops( true );
}

void DropPixmapItem::dropEvent(QGraphicsSceneDragDropEvent* event)
{
    DEBUG_BLOCK
    if( event->mimeData()->hasText() )
    {
        QString file( event->mimeData()->text() );
        debug() << "dropped:" << file;
        
        if ( file.contains( "http://" ) || file.contains( "https://" ) )
        {           
            m_url = KUrl( file );
            The::networkAccessManager()->getData( m_url, this,
                 SLOT(imageDownloadResult(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
        }
        
        else if ( file.contains( "file://" ) )
        {
            file.remove( "file://" );
            QPixmap cover;
            cover.load( file );
            if ( !cover.isNull() )
            {
                emit imageDropped( cover );
            }
            else
            {
                debug() << "not an image";
            }
        }
    }

    if( event->mimeData()->hasImage() )
    {
        debug() << "mimeData has image";
        emit imageDropped( qVariantValue< QPixmap >( event->mimeData()->imageData() ) );
    }
}

void DropPixmapItem::imageDownloadResult( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    if( !url.isValid() || m_url != url )
        return;

    m_url.clear();
    if( e.code != QNetworkReply::NoError )
    {
        debug() << "unable to download the image:" << e.description;
        return;
    }
    
    QPixmap cover;
    if( cover.loadFromData( data ) )
        emit imageDropped( cover );
    else
        debug() << "not an image";
}

DropPixmapLayoutItem::DropPixmapLayoutItem( QGraphicsLayoutItem *parent, bool isLayout )
    : QGraphicsLayoutItem( parent, isLayout )
{
    m_pixmap = new DropPixmapItem;
    m_pixmap->setZValue( 100 );
    setGraphicsItem( m_pixmap );
    connect( m_pixmap, SIGNAL(imageDropped(QPixmap)), SIGNAL(imageDropped(QPixmap)) );
}

DropPixmapLayoutItem::~DropPixmapLayoutItem()
{
    delete m_pixmap;
}

void
DropPixmapLayoutItem::setGeometry( const QRectF &rect )
{
    QGraphicsLayoutItem::setGeometry( rect );
    int width = m_pixmap->pixmap().width();
    int height = m_pixmap->pixmap().height();
    QPointF pos = rect.topLeft();
    pos.rx() += (rect.width() - width) / 2;
    pos.ry() += (rect.height() - height) / 2;
    m_pixmap->setPos( pos );
}

QSizeF
DropPixmapLayoutItem::sizeHint( Qt::SizeHint which, const QSizeF &constraint ) const
{
    Q_UNUSED( which )
    Q_UNUSED( constraint )
    return m_pixmap->boundingRect().size();
}

qreal
DropPixmapLayoutItem::opacity() const
{
    return m_pixmap->opacity();
}

void
DropPixmapLayoutItem::setOpacity( qreal value )
{
    m_pixmap->setOpacity( value );
}

QPixmap
DropPixmapLayoutItem::pixmap() const
{
    return m_pixmap->pixmap();
}

void
DropPixmapLayoutItem::setPixmap( const QPixmap &pixmap )
{
    m_pixmap->setPixmap( pixmap );
}

void
DropPixmapLayoutItem::show()
{
    m_pixmap->show();
}

void
DropPixmapLayoutItem::hide()
{
    m_pixmap->hide();
}

