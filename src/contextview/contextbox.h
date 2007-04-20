/***************************************************************************
 * copyright            : (C) 2007 Seb Ruiz <ruiz@kde.org>                 *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_CONTEXTBOX_H
#define AMAROK_CONTEXTBOX_H

#include <QGraphicsRectItem>

class QGraphicsItem;
class QGraphicsRectItem;
class QGraphicsScene;
class QGraphicsSceneMouseEvent;
class QSize;
class QTimeLine;

namespace Context
{

class ContextBox : public QObject, public QGraphicsRectItem
{
    Q_OBJECT

    public:
        ContextBox( QGraphicsItem *parent = 0, QGraphicsScene *scene = 0 );
        ~ContextBox() { /* delete, disconnect and disembark */ }

        virtual void setTitle( const QString &title );
        virtual void toggleVisibility();

    protected:
        virtual void mousePressEvent( QGraphicsSceneMouseEvent *event );
        virtual void setBoundingRectSize( const QSize &sz );
        void setContentRectSize( const QSize &sz );

        QGraphicsTextItem *m_titleItem;
        QGraphicsRectItem *m_titleBarRect;
        QGraphicsRectItem *m_contentRect;

        bool  m_goingUp;
        qreal m_optimumHeight;
        qreal m_animationIncrement;
        QTimeLine *m_animationTimer;

    protected slots:
        void visibilityTimerSlot();
};

}

#endif

