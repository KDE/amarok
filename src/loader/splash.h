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

#include <qwidget.h> //baseclass

class Splash : public QWidget
{
public:
    Splash();

protected:
    virtual void mousePressEvent( QMouseEvent* );
};

#endif
