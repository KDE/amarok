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

#ifndef FADINGIMAGEITEM_H
#define FADINGIMAGEITEM_H

#include <QObject>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QTimeLine>

namespace Context
{


    /**
    A simple "widget" for the context view that provides a fading image
    Will be ported to use QGraphicsSvgItem once that successfully renders
    the Amarok logo file

	@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
    */
    class FadingImageItem : public QObject, public QGraphicsPixmapItem
    {

    Q_OBJECT
    public:
        FadingImageItem( const QPixmap & pixmap, QGraphicsItem * parent = 0 );
        void setFadeColor( const QColor &color );
        void setTargetAlpha( int alpha );
        void startFading();

    public slots:
        void fadeSlot( int step );

    private:
        QTimeLine * m_timeLine;
        QGraphicsRectItem * m_shadeRectItem;

        QColor m_fadeColor;
        int m_targetAlpha;
        int m_animationSteps;

    };

}


#endif
