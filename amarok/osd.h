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

#ifndef OSD_H
#define OSD_H

#include <qtimer.h>
#include <qpainter.h>
#include <qapplication.h>
#include <qpixmap.h>
#include <qbitmap.h>
#include <qregion.h>
#include <qstyle.h>
#include <qregexp.h>

class OSDWidget : public QWidget
{
    Q_OBJECT
      public:
        OSDWidget();
        void showOSD(const QString &text);
        void setFont(QFont newfont);
        void setColor(QColor newcolor);
      protected slots:
        void removeOSD();
        void minReached();
//        void dblClick();
      protected:
        void paintOSD(const QString &text);
        void paintEvent(QPaintEvent*);
//        void mouseDoubleClickEvent(QMouseEvent *e);
        QString text;
        QTimer *timer;
        QTimer *timerMin;
        QFont font;
        QColor color;
        QPixmap osdBuffer;
        QStringList textBuffer;
};

#endif
