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
#include <qtimer.h>

#include <kcursor.h>         //previewWidget
#include <kdebug.h>
#include <kglobalsettings.h> //activeTitleColor()


OSDWidget::OSDWidget( const QString &appName, QWidget *parent, const char *name )
        : QWidget( parent, name,
                   WType_TopLevel | WStyle_StaysOnTop |
                   WStyle_Customize | WStyle_NoBorder |
                   WStyle_Tool | WRepaintNoErase | WX11BypassWM )
        , m_appName( appName )
        , m_duration( 5000 )
        , timer( new QTimer( this ) )
        , timerMin( new QTimer( this ) )
        , m_position( Top )
        , m_screen( 0 )
        , m_offset( 10, 40 )
        , m_dirty( false )
{
    setFocusPolicy( NoFocus );
    setBackgroundMode( NoBackground );
    unsetColors();

    connect( timer, SIGNAL( timeout() ), SLOT( removeOSD() ) );
    connect( timerMin, SIGNAL( timeout() ), SLOT( minReached() ) );
}


void OSDWidget::renderOSDText( const QString &text )
{
    static QBitmap mask;

    // Set a sensible maximum size, don't cover the whole desktop or cross the screen
    QSize max = QApplication::desktop() ->screen( m_screen ) ->size() - QSize( 40, 100 );
    QFont titleFont( "Arial", 12, QFont::Bold );
    QFontMetrics titleFm( titleFont );

    // The title cannnot be taller than one line
    // AlignAuto = align Arabic to the right, etc.
    QRect titleRect = titleFm.boundingRect( 0, 0, max.width(), titleFm.height(), AlignAuto, m_appName );
    // The osd cannot be larger than the screen
    QRect textRect = fontMetrics().boundingRect( 0, 0, max.width(), max.height(), AlignAuto | WordBreak, text );

    if ( textRect.width() < titleRect.width() )
        textRect.setWidth( titleRect.width() );

    //this should by within the screen bounds, there is a change the height is too much though
    textRect.addCoords( 0, 0, 20, titleRect.height() );

    osdBuffer.resize( textRect.size() );
    mask.resize( textRect.size() );
    resize( textRect.size() );
    rePosition();

    // Start painting!
    QPainter bufferPainter( &osdBuffer );
    QPainter maskPainter( &mask );

    // Draw backing rectangle
    bufferPainter.setPen( Qt::black );
    bufferPainter.setBrush( backgroundColor() );
    bufferPainter.drawRoundRect( textRect, 1500 / textRect.width(), 1500 / textRect.height() );
    bufferPainter.setFont( font() );

    const uint w = width() - 1;
    const uint h = height() - 1;

    // Draw the text shadow
    if ( m_shadow ) {
        bufferPainter.setPen( backgroundColor().dark( 175 ) );
        bufferPainter.drawText( 13, titleFm.height() + 1, w, h, AlignLeft | WordBreak, text );
    }

    // Draw the text
    bufferPainter.setPen( foregroundColor() );
    bufferPainter.drawText( 10, titleFm.height() - 1, w, h, AlignLeft | WordBreak, text );

    // Draw the title text
    bufferPainter.setFont( titleFont );
    bufferPainter.drawText( 10, 3, w, h, AlignLeft, m_appName );

    // Masking for transparency
    mask.fill( Qt::black );
    maskPainter.setBrush( Qt::white );
    maskPainter.drawRoundRect( textRect, 1500 / textRect.width(), 1500 / textRect.height() );
    setMask( mask );

    m_currentText = text;
    m_dirty = false;
}

void OSDWidget::showOSD( const QString &text, bool preemptive )
{
    if ( isEnabled() && !text.isEmpty() ) {
        if ( preemptive == false && timerMin->isActive() ) {
            textBuffer.append( text ); // queue
        } else {
            timer->stop();

            if ( m_currentText != text || m_dirty ) renderOSDText( text );

            if ( !isVisible() ) {
                raise();
                QWidget::show();
            } else
                update();

            // let it disappear via a QTimer
            if ( m_duration )   // if duration is 0 stay forever
            {
                timer->start( m_duration, TRUE );
                timerMin->start( 150 );
            }
        }
    }
}

void OSDWidget::setDuration( int ms )
{
    m_duration = ms;
}


//TODO personally I feel you should just insist people call refresh()
//     after they set settings and then inline all these

void OSDWidget::setFont( QFont newFont )
{
    QWidget::setFont( newFont );
    refresh();
}

void OSDWidget::setShadow( bool shadow )
{
    m_shadow = shadow;
    refresh();
}

void OSDWidget::setTextColor( QColor newColor )
{
    setPaletteForegroundColor( newColor );
    refresh();
}

void OSDWidget::setBackgroundColor( QColor newColor )
{
    setPaletteBackgroundColor( newColor );
    refresh();
}

void OSDWidget::unsetColors()
{
    setPaletteForegroundColor( KGlobalSettings::activeTextColor() );
    setPaletteBackgroundColor( KGlobalSettings::activeTitleColor() );

    refresh();
}


void OSDWidget::setOffset( int x, int y )
{
    m_offset = QPoint( x, y );
    rePosition();
}


void OSDWidget::setPosition( Position pos )
{
    m_position = pos;
    rePosition();
}

void OSDWidget::setScreen( uint screen )
{
    m_screen = screen;
    if ( m_screen >= QApplication::desktop() ->numScreens() )
        m_screen = QApplication::desktop() ->numScreens() - 1;
    rePosition();
}

void OSDWidget::setHorizontalAutoCenter( bool center )
{
    amaroK::OSD::m_horizontalAutoCenter = center;
    rePosition();
}

void OSDWidget::refresh()
{
    if ( isVisible() ) {
        //we need to update the buffer
        renderOSDText( m_currentText );

        //may as well update(), visible or not, if we're not visible Qt will wait until we are
        update();
    } else m_dirty = true; //ensure we are re-rendered before we are shown
}


//SLOT
void OSDWidget::minReached()
{
    if ( !textBuffer.isEmpty() ) {
        renderOSDText( textBuffer.front() );
        textBuffer.pop_front();
        update();

        if ( m_duration )   // if duration is 0 stay forever
        {
            //timerMin is still running
            timer->start( m_duration, TRUE );
        }
    } else timerMin->stop();
}


void OSDWidget::paintEvent( QPaintEvent* )
{
    bitBlt( this, 0, 0, &osdBuffer );
}

void OSDWidget::mousePressEvent( QMouseEvent* )
{
    removeOSD();
}

void OSDWidget::show()
{
    if ( m_dirty ) {
        //renderOSDText() sets m_dirty=false for us
        renderOSDText( m_currentText );
    }

    QWidget::show();
}


void OSDWidget::rePosition()
{
    QPoint newPos = m_offset;
    const QRect screenRect = QApplication::desktop() ->screenGeometry( m_screen );

    switch ( m_position ) {
        case Free:
        break;

        case Top:
        newPos.rx() = ( screenRect.width() - osdBuffer.width() ) / 2;
        newPos.ry() = MARGIN;
        break;

        case Center:
        newPos.rx() = ( screenRect.width() - osdBuffer.width() ) / 2;
        newPos.ry() = ( screenRect.height() - osdBuffer.height() ) / 2;
        break;

        case Bottom:
        newPos.rx() = ( screenRect.width() - osdBuffer.width() ) / 2;
        newPos.ry() = screenRect.height() - osdBuffer.height() - MARGIN;
        break;
    }

    // correct for screen position
    newPos += screenRect.topLeft();

    // Be nice and center the text, if the user wishes so
    if ( amaroK::OSD::m_horizontalAutoCenter )
        newPos.setX( screenRect.width() / 2 - osdBuffer.width() / 2 );

    // TODO: check for sanity?
    move( newPos ); //no need to check if pos() == newPos, Qt does that too
}


//////  OSDPreviewWidget below /////////////////////

QPoint OSDPreviewWidget::m_previewOffset;


OSDPreviewWidget::OSDPreviewWidget( const QString &appName ) : OSDWidget( appName )
{
    m_dragging = false;
    setDuration( 0 );
}


void OSDPreviewWidget::mousePressEvent( QMouseEvent *event )
{
    m_dragOffset = event->pos();
    if ( event->button() == LeftButton && !m_dragging ) {
        grabMouse( KCursor::sizeAllCursor() );
        m_dragging = true;
    }
}

void OSDPreviewWidget::mouseReleaseEvent( QMouseEvent * /*event*/ )
{
    if ( m_dragging ) {
        m_dragging = false;
        releaseMouse();

        // compute current Position && offset
        QDesktopWidget *desktop = QApplication::desktop();
        int currentScreen = desktop->screenNumber( pos() );

        if ( currentScreen != -1 ) {
            // set new data
            m_previewOffset = pos();
            m_position = Free;
            m_screen = currentScreen;

            emit positionChanged( m_screen, m_position, x(), y() );
        }
    }
}

void OSDPreviewWidget::mouseMoveEvent( QMouseEvent *e )
{
    if ( m_dragging && this == mouseGrabber() ) {
        // Be nice and center the text, if the user wishes so
        if ( amaroK::OSD::m_horizontalAutoCenter ) {
            const QRect screenRect = QApplication::desktop() ->screenGeometry( m_screen );
            move( screenRect.width() / 2 - osdBuffer.width() / 2, ( e->globalPos() - m_dragOffset ).y() );
        } else
            move( e->globalPos() - m_dragOffset );
    }
}


//////  amaroK::OSD below /////////////////////

#include "enginecontroller.h"
#include "metabundle.h"
#include <qregexp.h>

bool amaroK::OSD::m_horizontalAutoCenter;

void amaroK::OSD::showTrack( const MetaBundle &bundle )
{
    // Strip HTML tags, expand basic HTML entities
    QString text = bundle.prettyTitle();

    if ( bundle.length() ) {
        text += " - ";
        text += bundle.prettyLength();
    }

    text.replace( QRegExp( "</?(?:font|a|b|i)\\b[^>]*>" ), QString::null );
    text.replace( "&lt;", "<" );
    text.replace( "&gt;", ">" );
    text.replace( "&amp;", "&" );

    m_text = text;

    showTrack();
}

#include "osd.moc"
