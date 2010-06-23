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
#include "NetworkAccessManagerProxy.h"

#include <QGraphicsSceneDragDropEvent>
#include <QMimeData>
#include <QNetworkReply>
#include <QNetworkRequest>

DropPixmapItem::DropPixmapItem( QGraphicsItem* parent )
    : QGraphicsPixmapItem( parent )
{
    setAcceptDrops( true );
    connect( The::networkAccessManager(), SIGNAL(finished(QNetworkReply*)),
                                          SLOT(imageDownloadResult(QNetworkReply*)) );
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
            QNetworkRequest req( m_url );
            The::networkAccessManager()->get( req );
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

void DropPixmapItem::imageDownloadResult( QNetworkReply *reply )
{
    const KUrl url = reply->request().url();
    if( !url.isValid() || m_url != url )
        return;

    m_url.clear();
    if( reply->error() != QNetworkReply::NoError )
    {
        debug() << "unable to download the image:" << reply->errorString();
        reply->deleteLater();
        return;
    }
    
    QPixmap cover;
    if( cover.loadFromData( reply->readAll() ) )
    {
        emit imageDropped( cover );
    }
    else
    {
        debug() << "not an image";
    }
    reply->deleteLater();
}

#include "DropPixmapItem.moc"
