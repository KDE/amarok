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

#include "metabundle.h"
#include "osd.h"

#include <qapplication.h>
#include <qbitmap.h>
#include <qimage.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qregexp.h>
#include <qtimer.h>


OSDWidget::OSDWidget()
    : QWidget(NULL, "osd",
              WType_TopLevel | WStyle_StaysOnTop |
              WStyle_Customize | WStyle_NoBorder |
              WStyle_Tool | WRepaintNoErase | WX11BypassWM)
    , m_duration( 5000 )
{
    setFocusPolicy( NoFocus );
    timer = new QTimer( this );
    timerMin = new QTimer( this );
    connect( timer, SIGNAL( timeout() ), this, SLOT( removeOSD() ) );
    connect( timerMin, SIGNAL( timeout() ), this, SLOT( minReached() ) );
}


void OSDWidget::paintOSD( const QString &text )
{
    this->text = text;
    // Drawing process itself
    QPainter paint;
    QColor bg( 0, 0, 0 );
    QColor fg( 255, 255, 255 );

    QFontMetrics *fm = new QFontMetrics( font );

    // Get desktop dimensions
    QWidget *d = QApplication::desktop();

    QRect fmRect = fm->boundingRect( 0, 0, d->width(), d->height(), AlignLeft | WordBreak, this->text );
    fmRect.addCoords( 0, 0, fmRect.width() + 2, fmRect.height() + 2);

    resize( fmRect.size() );
    //    QPixmap *buffer = new QPixmap( fmRect.width(), fmRect.height() );
    QPixmap buffer = QPixmap( fmRect.size() );

    // Draw the OnScreenMessage
    QPainter paintBuffer( &buffer );
    paintBuffer.setFont( font );

    // Draw the border around the text
    paintBuffer.setPen( black );
    /*  paintBuffer.drawText( 0, 0, width()-1, height()-1, AlignLeft | WordBreak, this->text );
      paintBuffer.drawText( 2, 0, width()-1, height()-1, AlignLeft | WordBreak, this->text );
      paintBuffer.drawText( 0, 2, width()-1, height()-1, AlignLeft | WordBreak, this->text );
      paintBuffer.drawText( 2, 2, width()-1, height()-1, AlignLeft | WordBreak, this->text );*/
    paintBuffer.drawText( 3, 3, width()-1, height()-1, AlignLeft | WordBreak, this->text );

    // Draw the text
    paintBuffer.setPen( color );
    paintBuffer.drawText( 1, 1, width()-1, height()-1, AlignLeft | WordBreak, this->text );
    paintBuffer.end();

    // Masking for transparency
    QBitmap bm( fmRect.size() );
    paint.begin( &bm );
    paint.fillRect( 0, 0, fmRect.width(), fmRect.height(), bg );
    paint.setPen( Qt::color0 );
    paint.setFont( font );
    //  paint.drawText( 0, 0, width()-1, height()-1, AlignLeft | WordBreak, this->text );
    paint.drawText( 1, 1, width()-1, height()-1, AlignLeft | WordBreak, this->text );
    paint.drawText( 3, 3, width()-1, height()-1, AlignLeft | WordBreak, this->text );
    //  paint.drawText( 2, 0, width()-1, height()-1, AlignLeft | WordBreak, this->text );
    //  paint.drawText( 0, 2, width()-1, height()-1, AlignLeft | WordBreak, this->text );
    //  paint.drawText( 2, 2, width()-1, height()-1, AlignLeft | WordBreak, this->text );
    paint.end();

    delete fm;

    // Let's make it real, flush the buffers
    osdBuffer = buffer;
    // Repaint the QWidget and get it on top
    setMask( bm );
    //    qApp->syncX();
    QWidget::show();
    raise();

    // let it disappear via a QTimer
    timer->start( m_duration, TRUE );
    timerMin->start( 150, TRUE );
}

//SLOT
void OSDWidget::showOSD( const MetaBundle &bundle )
{
    if ( isEnabled() )
    {
        // Currently fixed to the top border of desktop, full width
        // This should be configurable, already on my TODO
        move( 10, 40 );
        resize( 0, 0 );

        // Strip HTML tags, expand basic HTML entities
        QString text = QString( "%1 - %2" ).arg( bundle.prettyTitle(), bundle.prettyLength() );

        QString plaintext = text.copy();
        plaintext.replace( QRegExp( "</?(?:font|a|b|i)\\b[^>]*>" ), QString( "" ) );
        plaintext.replace( QRegExp( "&lt;" ), QString( "<" ) );
        plaintext.replace( QRegExp( "&gt;" ), QString( ">" ) );
        plaintext.replace( QRegExp( "&amp;" ), QString( "&" ) );

        if ( timerMin->isActive() )
            textBuffer.append( plaintext );
        else
        {
            if ( timer->isActive() ) timer->stop();
            paintOSD( plaintext );
        }
    }
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
        //raise();
        repaint();

        // let it disappear via a QTimer
        timer->start( SPLASH_DURATION, true );
    }
}


void OSDWidget::setDuration(int ms)
{
    m_duration = ms;
}


void OSDWidget::setFont(QFont newfont)
{
    font = newfont;
}


void OSDWidget::setColor(QColor newcolor)
{
    color = newcolor;
}

//SLOT
void OSDWidget::minReached()
{
    if ( textBuffer.count() > 0 )
    {
        paintOSD( textBuffer[0] );
        textBuffer.remove( textBuffer.at( 0 ) );
    }
    else
        this->text = "";
}

//SLOT
void OSDWidget::removeOSD()
{
    // hide() and show() prevents flickering
    hide();
    this->text = "";
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


#include "osd.moc"
