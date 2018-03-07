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

#include <QGraphicsObject>
#include <QWeakPointer>

class LabelOverlayButton;
class QGraphicsBlurEffect;
class QGraphicsPixmapItem;
class QGraphicsSceneHoverEvent;
class QGraphicsSceneMouseEvent;
class QGraphicsTextItem;
class QPropertyAnimation;

class LabelGraphicsItem : public QGraphicsObject
{
    Q_OBJECT
    Q_PROPERTY( qreal hoverValue READ hoverValue WRITE setHoverValue )
    Q_PROPERTY( QPointF pos READ pos WRITE setPos )

public:
    LabelGraphicsItem( const QString& text, qreal deltaPointSize, QGraphicsItem *parent );
    ~LabelGraphicsItem();

    QString text();
    void setText( const QString& text );
    void setDeltaPointSize( qreal deltaPointSize );
    void setSelected( bool selected );
    void setSelectedColor( QColor color );
    void setBackgroundColor( QColor color );
    void showBlacklistButton( bool enabled );
    
    void updateHoverStatus();
    
    QRectF boundingRect() const;
    void paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget );

protected:
    virtual void hoverLeaveEvent( QGraphicsSceneHoverEvent *event );
    virtual void hoverEnterEvent( QGraphicsSceneHoverEvent *event );
    virtual void mousePressEvent( QGraphicsSceneMouseEvent *event );

private:
    qreal hoverValue();
    void setHoverValue( qreal value );
    void updateGeometry();

    QGraphicsTextItem               *m_textItem;
    QGraphicsPixmapItem             *m_backgroundItem;
    QGraphicsBlurEffect             *m_backgroundBlurEffect;
    QColor                           m_backgroundColor;
    
    qreal                            m_hoverValue;
    QColor                           m_hoverColor;
    QWeakPointer<QPropertyAnimation> m_hoverValueAnimation;
    bool                             m_selected;
    QColor                           m_selectedColor;

    bool                             m_showBlacklistButton;
    
    QWeakPointer<LabelOverlayButton> m_addLabelItem;
    QWeakPointer<LabelOverlayButton> m_removeLabelItem;
    QWeakPointer<LabelOverlayButton> m_listLabelItem;
    QWeakPointer<LabelOverlayButton> m_blacklistLabelItem;
    
    QWeakPointer<QPropertyAnimation> m_addLabelAnimation;
    QWeakPointer<QPropertyAnimation> m_removeLabelAnimation;
    QWeakPointer<QPropertyAnimation> m_listLabelAnimation;
    QWeakPointer<QPropertyAnimation> m_blacklistLabelAnimation;
    
Q_SIGNALS:
    void toggled( const QString &label );
    void blacklisted( const QString &label );
    void list( const QString &label );
};

#endif

