/****************************************************************************************
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
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

//
//  This provides a simple on-hover SVG arrow to switch between containments.
//

#ifndef CONTAINMENT_ARROW_H
#define CONTAINMENT_ARROW_H

#include "amarok_export.h"
#include "context/Svg.h"

#include <QGraphicsItem>
#include <QObject>
#include <QTimer>

enum ArrowDirection {
    LEFT,
    RIGHT,
    DOWN,
    UP,
    DOWN_RIGHT,
    DOWN_LEFT,
    UP_RIGHT,
    UP_LEFT
};


#include "context/Containment.h" // needs ArrowDirection to be defined first

namespace Context {
    
class SvgRenderJob;
    
class AMAROK_EXPORT ContainmentArrow : public QObject, public QGraphicsItem
{
    Q_OBJECT
public:
    explicit ContainmentArrow( QGraphicsItem *parent  = 0, int direction = RIGHT );
    ~ContainmentArrow();
    
    virtual QRectF boundingRect() const;

    QSize size() const;
    void resize( const QSizeF newSize );
    
    void enable();
    void disable();
    
public slots:
    void show();
    void hide();
    
signals:
    void changeContainment( int to );
    
protected:
    virtual void paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0 );
    void hoverEnterEvent( QGraphicsSceneHoverEvent *event );
    void hoverLeaveEvent( QGraphicsSceneHoverEvent *event );
    void mousePressEvent( QGraphicsSceneMouseEvent *event );
    void mouseReleaseEvent( QGraphicsSceneMouseEvent *event );

    
private Q_SLOTS:
    void animateHighlight( qreal progress );
    void timeToHide();
    
private:
    QSize m_size;
    qreal m_animHighlightFrame;
    int m_animHighlightId;
    bool m_hovering;
    bool m_showing;
    bool m_disabled;
    qreal m_aspectRatio;
    
    QTimer *m_timer;
    
    Svg* m_arrowSvg;
    int m_arrowDirection;
    Plasma::Containment *m_containment;
};

 // namespace

}
#endif
