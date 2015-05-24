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

#ifndef DROPPIXMAPITEM_H
#define DROPPIXMAPITEM_H

#include "amarok_export.h"
#include "network/NetworkAccessManagerProxy.h"

#include <QUrl>

#include <QGraphicsLayoutItem>
#include <QGraphicsPixmapItem>

//forward
class QGraphicsSceneDragDropEvent;

/**
* \brief A QGraphicsPixmapItem on which you can drop an image
*
* Used for drag'n drop support for the cover. Will download the file if it's a link (from webrowser)
*
* \sa QGraphicsPixmapItem
*
* \author Simon Esneault <simon.esneault@gmail.com>
*/

class AMAROK_EXPORT DropPixmapItem : public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT
    public:
        
        DropPixmapItem( QGraphicsItem* parent = 0 );

    signals:
        void imageDropped( const QPixmap &pixmap );
        
    public slots:
        /**
        * Result of the image fetching stuff
        */
        void imageDownloadResult( const QUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e );
        
    protected slots:
        /**
        * Reimplement dropEvent
        */
        virtual void dropEvent( QGraphicsSceneDragDropEvent* );

    private:
        QUrl m_url;

};

class AMAROK_EXPORT DropPixmapLayoutItem : public QObject, public QGraphicsLayoutItem
{
    Q_OBJECT
    Q_INTERFACES( QGraphicsLayoutItem )
    Q_PROPERTY( QPixmap pixmap READ pixmap WRITE setPixmap )
    Q_PROPERTY( qreal opacity READ opacity WRITE setOpacity )

public:
    explicit DropPixmapLayoutItem( QGraphicsLayoutItem *parent = 0, bool isLayout = false );
    virtual ~DropPixmapLayoutItem();

    virtual void setGeometry( const QRectF &rect );

    qreal opacity() const;
    void setOpacity( qreal value );

    QPixmap pixmap() const;
    void setPixmap( const QPixmap &pixmap );

    void show();
    void hide();

signals:
    void imageDropped( const QPixmap &pixmap );

protected:
    virtual QSizeF sizeHint( Qt::SizeHint which, const QSizeF &constraint = QSizeF() ) const;

private:
    DropPixmapItem *m_pixmap;
};

#endif // DROPPIXMAPITEM_H
