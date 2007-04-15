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

#ifndef CONTEXTINTROANIMATION_H
#define CONTEXTINTROANIMATION_H

#include "graphicsitemfader.h"
#include "textfader.h"

#include <QObject>
#include <QGraphicsItem>

namespace Context {

/**
Displays a nice animation in the context view on startup.
Lots of hardcoded values at the moment....

	@author 
*/
class IntroAnimation : public QObject, public QGraphicsItem
{
Q_OBJECT
public:
    IntroAnimation( QGraphicsItem * parent = 0 );

    ~IntroAnimation();

    QRectF boundingRect() const;
    void paint(QPainter *painter,
                           const QStyleOptionGraphicsItem *option,
                           QWidget *widget);

    void startAnimation();
    void setFadeColor( const QColor &color );


signals: 
    void animationComplete();

private:
    GraphicsItemFader *m_logoFader;
    TextFader *m_textFader;

    int m_height;
    int m_width;

private slots:

    void imageFadeoutComplete();
    void textFadeinComplete();
    void trailTimeComplete();


};

}

#endif
