/***************************************************************************
                        amarokslider.h  -  description
                           -------------------
  begin                : Dec 15 2003
  copyright            : (C) 2003 by Mark Kretschmann
  email                :
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROKSLIDER_H
    
    #define AMAROKSLIDER_H
    
    #include <qrangecontrol.h>
    #include <qwidget.h>
    
    class QMouseEvent;
    class QPaintEvent;
    class QSize;
    
    class AmarokSlider : public QWidget, public QRangeControl
    {
            Q_OBJECT
    
        public:
	    enum VDirection { TopDown = 1, BottomUp = 1 };
	    
            AmarokSlider( QWidget *, Qt::Orientation, VDirection = BottomUp );

            void setValue( int );
            bool sliding() { return m_isPressed; }
                
        signals:
            void sliderPressed();
            void sliderReleased();
            void valueChanged( int );    
            
        private:
            QSize minimumSizeHint() const;
            QSize sizeHint() const;
            
            void mouseMoveEvent( QMouseEvent * );
            void mousePressEvent( QMouseEvent * );
            void mouseReleaseEvent( QMouseEvent * );
            void paintEvent( QPaintEvent * );
            
            // ATTRIBUTES ------
            bool m_isPressed;
            Qt::Orientation m_orientation;
	    VDirection m_dir;
    };
   
#endif
