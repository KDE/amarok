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

#ifndef GRAPHICSITEMFADER_H
#define GRAPHICSITEMFADER_H

#include "contextbox.h"

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
    class GraphicsItemFader : public QObject
    {

    Q_OBJECT
    public:
        explicit GraphicsItemFader( ContextBox *item );
        ~GraphicsItemFader();

        void setFadeColor( const QColor &color );
        void setStartAlpha( int alpha );
        void setTargetAlpha( int alpha );
        void setDuration( int ms );
        void setFPS( int fps );
        void startFading();
        ContextBox* contentItem() { return m_contentItem; }

    signals:
        void animationComplete();

    public slots:
        void fadeSlot( int step );
        void fadeFinished();

    private:
        QTimeLine*  m_timeLine;
        ContextBox* m_contentItem;

        QColor m_fadeColor;
        int m_startAlpha;
        int m_targetAlpha;
        qreal m_alphaStep;
        int m_fps;
        int m_duration;
        int m_animationSteps;
        int m_width;
        int m_height;

    };

}


#endif
