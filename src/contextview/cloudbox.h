/*
  Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#ifndef CLOUDBOX_H
#define CLOUDBOX_H

#include "contextbox.h"

#include <QTimeLine>

namespace Context
{


/**
A simple text item to go in a coud box

	@author 
*/
    class CloudTextItem : public QGraphicsTextItem
    {

    Q_OBJECT
    public:
        CloudTextItem(QString text, QGraphicsItem *parent = 0, QGraphicsScene *scene = 0 );

    protected:
        void hoverEnterEvent ( QGraphicsSceneHoverEvent * event );
        void hoverLeaveEvent ( QGraphicsSceneHoverEvent * event );

    public slots:
        void colorFadeSlot( int step );

    private:
        QTimeLine * m_timeLine;

    };




/**
A simple ContextBox that provides a cloud like view of a group of weighted items. Just for fun at this point! :-)

	@author 
*/
    class CloudBox : public ContextBox
    {
    public:

        CloudBox( QGraphicsItem *parent = 0, QGraphicsScene *scene = 0 );

        void addText(QString text, int weight );

    private:

        qreal m_runningX;
        qreal m_runningY;
        qreal m_currentLineMaxHeight;

        int m_maxFontSize;
        int m_minFontSize;
       

    };

}

#endif
