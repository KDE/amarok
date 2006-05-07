/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

// begin:     Fre Sep 26 2003
// copyright: (C) 2003 Christian Muehlhaeuser
//            (C) 2005 Max Howell

#include <qapplication.h>
#include <qpixmap.h>
#include <qprocess.h>

#include <kstandarddirs.h>
#include "splash.h"

extern "C"
{
    #include <unistd.h> //usleep
}

Splash::Splash()
        : QWidget( 0, 0, WType_TopLevel | WX11BypassWM | WStyle_StaysOnTop )
{
    QString path = KStandardDirs().findResource("data", "amarok/images/splash_screen.jpg");

    QPixmap splash( path );
    resize( splash.size() );
    setBackgroundPixmap( splash );
    setFocusPolicy( NoFocus );

    //NOTE Don't break Xinerama!
    const QRect d = QApplication::desktop()->screenGeometry( QApplication::desktop()->screenNumber( QPoint() ) );
    QPoint p = d.topLeft();
    p.rx() += (d.width() - width()) / 2;
    p.ry() += (d.height() - height()) / 2;
    move( p );

    show();
}

void
Splash::mousePressEvent( QMouseEvent* )
{
    hide();
}
