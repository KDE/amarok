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
    : QWidget(NULL, "osd",
              WType_TopLevel | WStyle_StaysOnTop |
              WStyle_Customize | WStyle_NoBorder |
              WStyle_Tool | WRepaintNoErase | WX11BypassWM)
{
    // Currently fixed to the top border of desktop, full width
    // This should be configurable, already on my TODO

    move( 10, 40 );
    resize( 0, 0 );
    setFocusPolicy( NoFocus );
    timer = new QTimer( this );
    connect( timer, SIGNAL( timeout() ), this, SLOT( removeOSD() ) );
}


//SLOT
void OSDWidget::showSplash( const QString& imagePath )
{
    if ( isEnabled() )
    {
        QImage image( imagePath );
        osdBuffer = image;

        QBitmap bm( image.size() );
        QPainter p( &bm );
        p.drawImage( 0, 0, image.createAlphaMask() );

        QWidget *d = QApplication::desktop();
        move( d->width()  / 2 - image.width () / 2,
              d->height() / 2 - image.height() / 2 );
        resize( osdBuffer.size() );

        setMask( bm );

        show();
        repaint();

        // let it disappear via a QTimer
        timer->start( SPLASH_DURATION, true );
    }
}


//SLOT
void OSDWidget::removeOSD()
{
    // hide() and show() prevents flickering
    hide();
}


void OSDWidget::paintEvent( QPaintEvent* )
{
    QPainter p( this );
    p.drawPixmap( 0, 0, osdBuffer );
    p.end();
}


void OSDWidget::mousePressEvent( QMouseEvent* )
{
    removeOSD();
}


#include "splash.moc"
