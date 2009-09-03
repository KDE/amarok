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

#include "DropPixmapItem.h"

#include "Debug.h"

#include <KIO/Job>
#include <KUrl>

#include <QGraphicsSceneDragDropEvent>
#include <QMimeData>

#define DEBUG_PREFIX "DropPixmapItem"


DropPixmapItem::DropPixmapItem( QGraphicsItem* parent )
    : QGraphicsPixmapItem( parent )
    , m_job( 0 )
{
    setAcceptDrops( true );
}

void DropPixmapItem::dropEvent(QGraphicsSceneDragDropEvent* event)
{
    Q_UNUSED( event )
    
    DEBUG_BLOCK
    
    if  ( event->mimeData()->hasText() )
    {
        QString file( event->mimeData()->text() );
        debug() << "DropPixmapItem::dropped : " << file;
        
        if ( file.contains( "http://" ) || file.contains( "https://" ) )
        {           
            m_job = KIO::storedGet( KUrl( file ), KIO::Reload, KIO::HideProgressInfo );
            connect( m_job, SIGNAL( result( KJob* ) ), SLOT( imageDownloadResult( KJob* ) ) );
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
                debug() << "DropPixmapItem::not an image";
            }
        }
    }

    if ( event->mimeData()->hasImage() )
    {
        debug() << "DropPixmapItem:: mimeData has image";
        emit imageDropped( qVariantValue< QPixmap >( event->mimeData()->imageData() ) );
    }
}

void DropPixmapItem::imageDownloadResult( KJob *job )
{
    if ( !m_job ) // if it's not the correct job
        return;
    
    DEBUG_BLOCK
    if ( m_job->error() != KJob::NoError && m_job == job ) // It's the correct job but it errored out
    {
        debug() << "DropPixmapItem::unable to download the image: " << job->errorString();
        m_job = 0; // clear job
        return;
    }
    
    // Get the result
    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    QString jobUrl( storedJob->url().toMimeDataString() );

    QPixmap cover;
    if ( !cover.loadFromData( storedJob->data() ) )
    {
        debug() << "DropPixmapItem::not an image";
        return;
    }
//     
    emit imageDropped( cover );
}

#include "DropPixmapItem.moc"
