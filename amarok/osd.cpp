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

#include "osd.h"

#include <qapplication.h>
#include <qbitmap.h>
#include <qimage.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qregexp.h>
#include <qtimer.h>

OSDWidget::OSDWidget( const QString &appName )
    : QWidget(NULL, "osd",
              WType_TopLevel | WStyle_StaysOnTop |
              WStyle_Customize | WStyle_NoBorder |
              WStyle_Tool | WRepaintNoErase | WX11BypassWM)
      , m_appName( appName )
      , m_duration( 5000 )
      , m_textColor( 0, 0 , 0 )
      , m_bgColor( 0x00, 0x00, 0x80 )
      , m_offset( 10, 40 )
      , m_position( TopLeft )
      , m_screen( 0 )
{
    setFocusPolicy( NoFocus );
    timer = new QTimer( this );
    timerMin = new QTimer( this );
    connect( timer, SIGNAL( timeout() ), this, SLOT( removeOSD() ) );
    connect( timerMin, SIGNAL( timeout() ), this, SLOT( minReached() ) );
}


void OSDWidget::renderOSDText( const QString &text )
{
    QPainter paint;
    QColor bg( 0, 0, 0 );
    QColor fg( 255, 255, 255 );

    // Get desktop dimensions
    QWidget *d = QApplication::desktop()->screen( m_screen );

    QFontMetrics *fm = new QFontMetrics( font );
    /* AlignAuto = we want to align Arabic to the right, don't we? */
    QRect fmRect = fm->boundingRect( 0, 0, d->width(), d->height(), AlignAuto | WordBreak, text );

    QFont titleFont("Arial", 12, QFont::Bold);
    QFontMetrics *titleFm = new QFontMetrics( titleFont );

    fmRect.addCoords( 0, 0, 20, titleFm->height() );

    resize( fmRect.size() );
    //    QPixmap *buffer = new QPixmap( fmRect.width(), fmRect.height() );
    QPixmap buffer = QPixmap( fmRect.size() );

    // Draw the OnScreenMessage
    QPainter bufferPainter( &buffer );

    bufferPainter.setBrush( m_bgColor );
    bufferPainter.drawRoundRect( fmRect, 1500 / fmRect.width(), 1500 / fmRect.height() );
    //paintRoundRect( bufferPainter, fmRect );

    bufferPainter.setFont( titleFont );
    bufferPainter.setPen( m_textColor );
    bufferPainter.drawText( 10, 3, width()-1, height()-1, AlignLeft, m_appName );

    bufferPainter.setFont( font );

    // Draw the border around the text
    bufferPainter.setPen( black );
    /*  bufferPainter.drawText( 0, 0, width()-1, height()-1, AlignLeft | WordBreak, text );
      bufferPainter.drawText( 2, 0, width()-1, height()-1, AlignLeft | WordBreak, text );
      bufferPainter.drawText( 0, 2, width()-1, height()-1, AlignLeft | WordBreak, text );
      bufferPainter.drawText( 2, 2, width()-1, height()-1, AlignLeft | WordBreak, text );*/
    bufferPainter.drawText( 13, titleFm->height() + 1, width()-1, height()-1, AlignLeft | WordBreak, text );

    // Draw the text
    bufferPainter.setPen( m_textColor );
    bufferPainter.drawText( 10, titleFm->height() - 1, width()-1, height()-1, AlignLeft | WordBreak, text );
    bufferPainter.end();

    // Masking for transparency
    QBitmap bm( fmRect.size() );
    paint.begin( &bm );
    paint.fillRect( 0, 0, fmRect.width(), fmRect.height(), bg );
    paint.setBrush( fg );
    paint.drawRoundRect( fmRect, 1500 / fmRect.width(), 1500 / fmRect.height() );
/*    paint.setPen( Qt::color0 );
    paint.setFont( font );
    //  paint.drawText( 0, 0, width()-1, height()-1, AlignLeft | WordBreak, text );
    paint.drawText( 1, 1, width()-1, height()-1, AlignLeft | WordBreak, text );
    paint.drawText( 3, 3, width()-1, height()-1, AlignLeft | WordBreak, text );*/
    //  paint.drawText( 2, 0, width()-1, height()-1, AlignLeft | WordBreak, text );
    //  paint.drawText( 0, 2, width()-1, height()-1, AlignLeft | WordBreak, text );
    //  paint.drawText( 2, 2, width()-1, height()-1, AlignLeft | WordBreak, text );
    paint.end();

    delete fm;
    delete titleFm;

    osdBuffer = buffer;
    // Repaint the QWidget and get it on top
    setMask( bm );
    rePosition();
}

void OSDWidget::showOSD( const QString &text )
{
    if ( isEnabled() )
    {
        if ( timerMin->isActive() )
            textBuffer.append( text );
        else
        {
            if ( timer->isActive() ) timer->stop();
            renderOSDText( text );
            raise();
            QWidget::show();

            // let it disappear via a QTimer
            timer->start( m_duration, TRUE );
            timerMin->start( 150, TRUE );
        }
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


void OSDWidget::setTextColor(QColor newcolor)
{
    m_textColor = newcolor;
}

void OSDWidget::setBackgroundColor(QColor newColor)
{
    m_bgColor = newColor;
}


void OSDWidget::setOffset(int x, int y) // QPoint?
{
    m_offset.setX( x );
    m_offset.setY( y );
}

void OSDWidget::setPosition(Position pos)
{
    m_position = pos;
}

void OSDWidget::setScreen(uint screen)
{
    m_screen = screen;
    if( m_screen >= QApplication::desktop()->numScreens() )
        m_screen = QApplication::desktop()->numScreens() - 1;
}


//SLOT
void OSDWidget::minReached()
{
    if ( textBuffer.count() > 0 )
    {
        renderOSDText( textBuffer[0] );
        textBuffer.remove( textBuffer.at( 0 ) );
        raise();
        QWidget::show();

        // let it disappear via a QTimer
        timer->start( m_duration, TRUE );
        timerMin->start( 150, TRUE );
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

void OSDWidget::rePosition()
{
    int newX = 0, newY = 0;
    QRect screenRect = QApplication::desktop()->screenGeometry( m_screen );
    switch( m_position )
    {
        case TopLeft:
            newX = m_offset.x();
            newY = m_offset.y();
            break;
        case TopRight:
            newX = screenRect.width() - m_offset.x() - osdBuffer.width();
            newY = m_offset.y();
            break;
        case BottomLeft:
            newX = m_offset.x();
            newY = screenRect.height() - m_offset.y() - osdBuffer.height();
            break;
        case BottomRight:
            newX = screenRect.width() - m_offset.x() - osdBuffer.width();
            newY = screenRect.height() - m_offset.y() - osdBuffer.height();
            break;
        case Center:
            newX = ( screenRect.width() - osdBuffer.width() ) / 2;
            newY = ( screenRect.height() - osdBuffer.height() ) / 2;
            break;
    }

    // correct for screen position
    newX += screenRect.x();
    newY += screenRect.y();

    // TODO: check for sanity?
    if( newX != x() || newY != y() ) move( newX, newY );
}

#include "osd.moc"
