/***************************************************************************
                       amarokslider.h  -  description
                          -------------------
 begin                : Dec 15 2003
 copyright            : (C) 2003-2008 by Mark Kretschmann
 email                : kretschmann@kde.org
 copyright            : (C) 2005 by Gábor Lehel
 email                : illissius@gmail.com
 copyright            : (C) 2008 by Dan Meltzer
 email                : parallelgrapefruit@gmail.com
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

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
            Slider( Qt::Orientation, QWidget*, uint max = 0 );

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

            virtual void paletteChange( const QPalette & oldPalette );

            void paintCustomSlider( QPainter *p, int x, int y, int width, int height, double pos = -1.0 );

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
            explicit VolumeSlider( QWidget *parent, uint max = 0 );

        protected:
            virtual void paintEvent( QPaintEvent* );

            virtual void paletteChange( const QPalette& );
            virtual void slideEvent( QMouseEvent* );
            virtual void mousePressEvent( QMouseEvent* );
            virtual void contextMenuEvent( QContextMenuEvent* );
            virtual void wheelEvent( QWheelEvent *e );
            virtual void resizeEvent(QResizeEvent * event);

        signals:
            void mute();

        private:
            Q_DISABLE_COPY( VolumeSlider )

            int m_sliderWidth;
            int m_sliderHeight;
            int m_marginLeft, m_marginRight;
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
            virtual void paletteChange( const QPalette& );
            virtual void resizeEvent(QResizeEvent * event);

        private:
            Q_DISABLE_COPY( TimeSlider )

            QTimer *m_animTimer; // Used for a smooth progress.
            QList<BookmarkTriangle*> m_triangles;
            int m_sliderHeight;
            double m_knobX; // The position of the current indicator.
    };
}

#endif

