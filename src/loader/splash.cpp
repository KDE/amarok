/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  osd.cpp  -  Provides an interface to a plain QWidget, which is independent of KDE (bypassed to X11)
  begin:     Fre Sep 26 2003
  copyright: (C) 2003 by Christian Muehlhaeuser
  email:     muesli@chareit.net
*/

#include "splash.h"

#include <qapplication.h>
#include <qbitmap.h>
#include <qimage.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qtimer.h>


OSDWidget::OSDWidget()
    : QWidget( 0, "osd",
              WType_TopLevel | WStyle_StaysOnTop |
              WStyle_Customize | WStyle_NoBorder |
              WStyle_Tool | WRepaintNoErase | WX11BypassWM)
{
    // Currently fixed to the top border of desktop, full width
    // This should be configurable, already on my TODO

    setFocusPolicy( NoFocus );
    timer = new QTimer( this );
    connect( timer, SIGNAL( timeout() ), this, SLOT( hide() ) );
}


//SLOT
void OSDWidget::showSplash( const QString& imagePath )
{
    QImage image( imagePath );
    osdBuffer.load( imagePath );

//     QBitmap bm( image.size() );
//     QPainter paint( &bm );
//     paint.drawImage( 0, 0, image.createAlphaMask() );
//     paint.end();

    // Keep in mind the Xinerama-case when changing this
    QRect d = QApplication::desktop()->screenGeometry( QApplication::desktop()->screenNumber( QPoint(0,0) ) );
    QPoint p = d.topLeft();
    p.rx() += (d.width()-image.width()) / 2;
    p.ry() += (d.height()-image.height()) / 2;
    move( p );
    resize( osdBuffer.size() );

//     setMask( bm );

    show();

    // let it disappear via a QTimer
    timer->start( SPLASH_DURATION, true );
}


void OSDWidget::paintEvent( QPaintEvent* )
{
    bitBlt( this, 0, 0, &osdBuffer );
}


void OSDWidget::mousePressEvent( QMouseEvent* )
{
    hide();
}


#include "splash.moc"
