/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  osd.h   -  Provides an interface to a plain QWidget, which is independent of KDE (bypassed to X11)
  begin:     Fre Sep 26 2003
  copyright: (C) 2003 by Christian Muehlhaeuser
  email:     muesli@chareit.net
*/

#ifndef SPLASH_H
#define SPLASH_H

#include <qpixmap.h>
#include <qwidget.h> //baseclass

class QFont;
class QString;
class QStringList;
class QTimer;

class OSDWidget : public QWidget
{
    Q_OBJECT
      
      public:
        OSDWidget();
      
      public slots:
        void showSplash(const QString& imagePath);
        void removeOSD();
      
      protected:
        void paintEvent(QPaintEvent*);
        void mousePressEvent( QMouseEvent* );
        
        static const int SPLASH_DURATION = 25000; 
        
        QTimer      *timer;
        QPixmap     osdBuffer;
};

#endif
