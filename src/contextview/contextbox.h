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

#include <QGraphicsItemGroup>

class QGraphicsItem;
class QGraphicsRectItem;
class QGraphicsScene;
class QSize;

namespace Context
{

class ContextBox : public QGraphicsRectItem
{

    public:
        ContextBox( QGraphicsItem *parent = 0, QGraphicsScene *scene = 0 );
        ~ContextBox() { /* delete, disconnect and disembark */ }

        virtual void setTitle( const QString &title );

    protected:
        virtual void setBoundingRectSize( const QSize &sz );
        void setContentRectSize( const QSize &sz );

        QGraphicsTextItem *m_titleItem;
        QGraphicsRectItem *m_titleBarRect;
        QGraphicsRectItem *m_contentRect;
};

}

#endif

