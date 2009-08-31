/****************************************************************************************
 * Copyright (c) 2003-2008 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2005 Gabor Lehel <illissius@gmail.com>                                 *
 * Copyright (c) 2008 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
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

#ifndef SLIDERWIDGET_H
#define SLIDERWIDGET_H

#include <QList>
#include <QPixmap>
#include <QSlider>
#include <QVector>

class QPalette;
class QTimer;
class BookmarkTriangle;

namespace Amarok
{
    class Slider : public QSlider
    {
        Q_OBJECT

        public:
            explicit Slider( Qt::Orientation, uint max = 0, QWidget* parent = 0 );

            virtual void setValue( int );

            //WARNING non-virtual - and thus only really intended for internal use
            //this is a major flaw in the class presently, however it suits our
            //current needs fine
            int value() const { return adjustValue( QSlider::value() ); }

        signals:
            //we emit this when the user has specifically changed the slider
            //so connect to it if valueChanged() is too generic
            //Qt also emits valueChanged( int )
            void sliderReleased( int );

        protected:
            virtual void wheelEvent( QWheelEvent* );
            virtual void mouseMoveEvent( QMouseEvent* );
            virtual void mouseReleaseEvent( QMouseEvent* );
            virtual void mousePressEvent( QMouseEvent* );
            virtual void slideEvent( QMouseEvent* );
            virtual void resizeEvent( QResizeEvent * ) { m_needsResize = true; }

            void paintCustomSlider( QPainter *p, int x, int y, int width, int height, double pos = -1.0 );
            void paintCustomSliderNG( QPainter *p, int x, int y, int width, int height, double pos = -1.0 );

            bool m_sliding;

            /// we flip the value for vertical sliders
            int adjustValue( int v ) const
            {
               int mp = (minimum() + maximum()) / 2;
               return orientation() == Qt::Vertical ? mp - (v - mp) : v;
            }

            static const int m_borderWidth = 6;
            static const int m_borderHeight = 6;

            static const int m_sliderInsertX = 5;
            static const int m_sliderInsertY = 5;

        private:
            bool m_outside;
            int  m_prevValue;
            bool m_needsResize;

            QPixmap m_topLeft;
            QPixmap m_topRight;
            QPixmap m_top;
            QPixmap m_bottomRight;
            QPixmap m_right;
            QPixmap m_bottomLeft;
            QPixmap m_bottom;
            QPixmap m_left;

            Q_DISABLE_COPY( Slider )
    };

    class VolumeSlider: public Slider
    {
        Q_OBJECT

        public:
            explicit VolumeSlider( uint max, QWidget *parent = 0 );

        protected:
            virtual void paintEvent( QPaintEvent* );

            virtual void mousePressEvent( QMouseEvent* );
            virtual void contextMenuEvent( QContextMenuEvent* );
            virtual void wheelEvent( QWheelEvent *e );
            virtual void resizeEvent(QResizeEvent * event);

        private:
            Q_DISABLE_COPY( VolumeSlider )
    };

    class TimeSlider : public Amarok::Slider
    {
        Q_OBJECT

        public:
            TimeSlider( QWidget *parent );

            void setSliderValue( int value );
            void drawTriangle( const QString &name, int x );
            void clearTriangles();

        public slots:
            void slotTriangleClicked( int );

        protected:
            virtual void paintEvent( QPaintEvent* );
            virtual void mousePressEvent( QMouseEvent* );
            virtual void resizeEvent(QResizeEvent * event);
            virtual bool event ( QEvent * event );

        private:
            Q_DISABLE_COPY( TimeSlider )

            QTimer *m_animTimer; // Used for a smooth progress.
            QList<BookmarkTriangle*> m_triangles;
            double m_knobX; // The position of the current indicator.
    };
}

#endif

