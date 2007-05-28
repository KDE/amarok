/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

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
            virtual void hoverEnterEvent ( QGraphicsSceneHoverEvent * );
            virtual void hoverLeaveEvent ( QGraphicsSceneHoverEvent * );
            virtual void mousePressEvent ( QGraphicsSceneMouseEvent * );

        public slots:
            void colorFadeSlot( int step );

        private:
            QTimeLine * m_timeLine;

        signals:
            void clicked( QString text );
    };


    /**
      A simple ContextBox that provides a cloud like view of a group of weighted items. Just for fun at this point! :-)
      @author
    */
    class CloudBox : public ContextBox
    {

    public:
        CloudBox( QGraphicsItem *parent = 0, QGraphicsScene *scene = 0 );

        void addText( QString text, int weight=10, QObject *receiver=0, const char *slot=0 );

        /**
         * Makes sure all elements are correctly positioned
         * (the last line might not have been printed yet
         */
        void done();

    private:

        /**
         * Adjusts the current line in the cloud so all ellements are aligned vertically
         * and the whole line is centered in the cloud box
         */
        void adjustCurrentLinePos();

        qreal m_runningX;
        qreal m_runningY;
        qreal m_currentLineMaxHeight;

        int m_maxFontSize;
        int m_minFontSize;

        QList<CloudTextItem *> m_currentLineItems;
    };

}

#endif
