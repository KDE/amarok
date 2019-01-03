/****************************************************************************************
 * Copyright (c) 2010 Daniel Faust <hessijames@gmail.com>                               *
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

#ifndef LABEL_OVERLAY_BUTTON_H
#define LABEL_OVERLAY_BUTTON_H

#include <QGraphicsItem>
#include <QPixmap>

class KIconEffect;
class QGraphicsSceneHoverEvent;
class QGraphicsSceneMouseEvent;
class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

class LabelOverlayButton : public QObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES( QGraphicsItem )
    Q_PROPERTY( qreal opacity READ opacity WRITE setOpacity )

public:
    explicit LabelOverlayButton( QGraphicsItem *parent );
    ~LabelOverlayButton() override;

    void setPixmap( const QPixmap& pixmap );
    QPixmap pixmap();
    void setSize( int size );
    int size();
    void updateHoverStatus();

    QRectF boundingRect() const;
    void paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget );

protected:
    void hoverEnterEvent( QGraphicsSceneHoverEvent *event ) override;
    void hoverLeaveEvent( QGraphicsSceneHoverEvent *event ) override;

private:
    KIconEffect     *m_iconEffect;
    QPixmap         m_pixmap;
    QPixmap         m_scaledPixmap;
    int             m_size;
    
};

#endif
