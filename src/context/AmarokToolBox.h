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
#include "Containment.h"
#include "ContextView.h"
#include "widgets/ToolBoxIcon.h"
#include "widgets/ToolBoxMenu.h"

#include <KIcon>

#include <plasma/animator.h>

#include <QAction>
#include <QGraphicsItem>
#include <QList>
#include <QObject>
#include <QStack>
#include <QTimer>


namespace Context{

class AmarokToolBoxMenu;
    
class AMAROK_EXPORT AmarokToolBox:  public QObject, public QGraphicsItem
{
    Q_OBJECT
public:
    explicit AmarokToolBox( QGraphicsItem *parent = 0 );
    ~AmarokToolBox();
    QRectF boundingRect() const;
    QPainterPath shape() const;

    QSize size() const;
    void resize( const QSize &newSize );
    
    void showTools();
    void hideTools();

    bool showingToolBox() const;
    bool showingMenu() const;

    void addAction( QAction *action );

    Containment *containment() const;
    
public slots:
    void showWidgetsMenu();
    void show();
    void hide();
    
protected:
    void paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0 );
    void hoverEnterEvent( QGraphicsSceneHoverEvent *event );
    void hoverLeaveEvent( QGraphicsSceneHoverEvent *event );
    void mousePressEvent( QGraphicsSceneMouseEvent *event );
    void mouseReleaseEvent( QGraphicsSceneMouseEvent *event );


protected slots:
    void animateHighlight( qreal progress );
    
Q_SIGNALS:
    void changeContainment( Plasma::Containment *containment );
    void correctToolBoxPos();
    
private:
    QSize m_size;
    qreal m_animHighlightFrame;
    int m_animHighlightId;
    bool m_hovering;
    bool m_showing;
    bool m_showingTools;

    int m_actionsCount;
    QTimer *m_timer;

    qreal m_animCircleFrame;
    int m_animCircleId;

    KIcon m_icon;
    AmarokToolBoxMenu *m_menu;

    Containment *m_containment;
    
private slots:
    void timeToHide();
    void animateCircle( qreal progress );
    void toolMoved( QGraphicsItem *item );
    
};

}

#endif
