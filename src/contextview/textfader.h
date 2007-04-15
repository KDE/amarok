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

#ifndef CONTEXTTEXTFADER_H
#define CONTEXTTEXTFADER_H

#include <QObject>
#include <QGraphicsTextItem>
#include <QTimeLine>

namespace Context {

/**
A specialized QGraphicsTextItem that can fade in or out. As opposed to the GraphicstemFader 
(which could also be used on a QGraphicsTextItem) this class provides real fading independent
of the backgorund

	@author Nikolaj Hald Nielsen
*/
    class TextFader : public QGraphicsTextItem{
    Q_OBJECT
    public:
        TextFader(const QString & text, QGraphicsItem * parent = 0);

        ~TextFader();

        void setStartAlpha( int alpha );
        void setTargetAlpha( int alpha );
        void setDuration( int ms );
        void setFPS( int fps );
        void startFading();

        virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);


    signals:
        void animationComplete();

    public slots:
        void fadeSlot( int step );


    private:
        QTimeLine * m_timeLine;
        int m_startAlpha;
        int m_targetAlpha;
        float m_alphaStep;
        int m_fps;
        int m_duration;
        int m_animationSteps;
        int m_currentAlpha;


    };

}

#endif
