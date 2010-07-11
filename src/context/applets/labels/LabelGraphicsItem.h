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

#ifndef LABEL_GRAPHICS_ITEM_H
#define LABEL_GRAPHICS_ITEM_H

#include "context/Applet.h"

#include <QGraphicsTextItem>
#include <QPixmap>

class QGraphicsPixmapItem;
class QGraphicsSceneHoverEvent;
class QGraphicsSceneMouseEvent;

class LabelGraphicsItem : public QGraphicsTextItem
{
    Q_OBJECT

public:
    LabelGraphicsItem( const QString &text, int deltaPointSize, QGraphicsItem *parent );
    ~LabelGraphicsItem();

    void setDeltaPointSize( int deltaPointSize );
    void setSelected( bool selected );
    
protected:
    virtual void hoverLeaveEvent( QGraphicsSceneHoverEvent *event );
    virtual void hoverEnterEvent( QGraphicsSceneHoverEvent *event );
    virtual void mousePressEvent( QGraphicsSceneMouseEvent *event );

private:
    bool                m_selected;
    QPixmap             m_addLabelPixmap;
    QPixmap             m_removeLabelPixmap;
    QPixmap             m_blacklistLabelPixmap;
    QPixmap             m_listLabelPixmap;
    QGraphicsPixmapItem *m_addLabelItem;
    QGraphicsPixmapItem *m_removeLabelItem;
    QGraphicsPixmapItem *m_blacklistLabelItem;
    QGraphicsPixmapItem *m_listLabelItem;
    
signals:
    void toggled( const QString &label );
    void blacklisted( const QString &label );
    void list( const QString &label );
};

#endif
