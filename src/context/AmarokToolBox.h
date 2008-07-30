/*******************************************************************************
* copyright              : (C) 2008 William Viana Soares <vianasw@gmail.com>   *
*                                                                              *
********************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifndef AMAROK_TOOLBOX_H
#define AMAROK_TOOLBOX_H

#include "amarok_export.h"

#include <KIcon>

#include <plasma/animator.h>

#include <QAction>
#include <QGraphicsItem>
#include <QObject>
#include <QTimer>

namespace Context{
    
class AMAROK_EXPORT AmarokToolBox:  public QObject, public QGraphicsItem
{
    Q_OBJECT
public:
    explicit AmarokToolBox( QGraphicsItem *parent = 0 );
    ~AmarokToolBox();
    QRectF boundingRect() const;
    QPainterPath shape() const;

    int size() const;
    void setSize( int newSize );
    
    void showTools();
    void hideTools();

    void show();
    void hide();
    bool showing() const;

    void addAction( QAction *action );

protected:
    void paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0 );
    void hoverEnterEvent( QGraphicsSceneHoverEvent *event );
    void hoverLeaveEvent( QGraphicsSceneHoverEvent *event );
    void mousePressEvent( QGraphicsSceneMouseEvent *event );
    void mouseReleaseEvent( QGraphicsSceneMouseEvent *event );


protected slots:
    void animateHighlight( qreal progress );
        
private:
    int m_size;
    qreal m_animHighlightFrame;
    int m_animHighlightId;
    bool m_hovering;
    bool m_showing;
    bool m_showingTools;

    int m_actionsCount;
    QTimer *m_timer;

    qreal m_animCircleFrame;
    int m_animCircleId;
 
private slots:
    void timeToHide();
    void animateCircle( qreal progress );
    
};


}

#endif
