/****************************************************************************************
 * Copyright (c) 2009 Pino Toscano <pino@kde.org>                                       *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef ANIMATEDWIDGET_H
#define ANIMATEDWIDGET_H

#include <qbasictimer.h>
#include <qpixmap.h>
#include <qwidget.h>

class AnimatedWidget : public QWidget
{
    Q_OBJECT

    public:
        explicit AnimatedWidget( const QString& iconName, QWidget *parent = 0 );
        virtual ~AnimatedWidget();

    public slots:
        void start();
        void stop();

    protected:
        void paintEvent( QPaintEvent *event );
        void resizeEvent( QResizeEvent *event );
        void timerEvent( QTimerEvent *event );

    private:
        void load();

        QString m_icon;
        QPixmap m_pixmap;
        int m_frames;
        int m_currentFrame;
        int m_size;
        QBasicTimer m_timer;
};

#endif
