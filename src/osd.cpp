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

#include "amarokconfig.h" //previewWidget
#include "osd.h"

#include <qapplication.h>
#include <qbitmap.h>
#include <qpainter.h>

#include <kdebug.h>
#include <kglobalsettings.h> //unsetColors()

#include <X11/Xlib.h> //reposition()


OSDWidget::OSDWidget( const QString &appName, QWidget *parent, const char *name )
        : QWidget( parent, name, WType_TopLevel | WNoAutoErase | WStyle_Customize | WX11BypassWM | WStyle_StaysOnTop )
        , m_appName( appName )
        , m_duration( 5000 )
        , m_shadow( true )
        , m_alignment( Middle )
        , m_screen( 0 )
        , m_y( MARGIN )
        , m_dirty( false )
{
    setFocusPolicy( NoFocus );
    setBackgroundMode( NoBackground );
    unsetColors();

    connect( &timer,     SIGNAL( timeout() ), SLOT( hide() ) );
    connect( &timerMin,  SIGNAL( timeout() ), SLOT( minReached() ) );
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

    // Start painting!
    QPainter bufferPainter( &osdBuffer );
    QPainter maskPainter( &mask );

    // Draw backing rectangle
    bufferPainter.setPen( Qt::black );
    bufferPainter.setBrush( backgroundColor() );
    bufferPainter.drawRoundRect( textRect, 1500 / textRect.width(), 1500 / textRect.height() );
    bufferPainter.setFont( font() );

    const uint w = textRect.width()  - 1;
    const uint h = textRect.height() - 1;

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

    //do last to reduce noticeable change when showing multiple OSDs in succession
    reposition( textRect.size() );

    m_currentText = text;
    m_dirty = false;

    update();
}


void OSDWidget::showOSD( const QString &text, bool preemptive )
{
    if ( isEnabled() && !text.isEmpty() ) {
        if ( preemptive || !timerMin.isActive() ) {
            m_currentText = text;
            m_dirty = true;

            show();
        }
        else textBuffer.append( text ); //queue
    }
}


void OSDWidget::minReached() //SLOT
{
    if ( !textBuffer.isEmpty() ) {
        renderOSDText( textBuffer.front() );
        textBuffer.pop_front();

        if( m_duration )
            //timerMin is still running
            timer.start( m_duration, TRUE );
    }
    else timerMin.stop();
}


void OSDWidget::setDuration( int ms )
{
    m_duration = ms;

    if( !m_duration ) timer.stop();
}

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

void OSDWidget::setOffset( int /*x*/, int y )
{
    //m_offset = QPoint( x, y );
    m_y = y;
    reposition();
}

void OSDWidget::setAlignment( Alignment a )
{
    m_alignment = a;
    reposition();
}

void OSDWidget::setScreen( uint screen )
{
    const int n = QApplication::desktop()->numScreens();
    m_screen = (screen >= n) ? n-1 : screen;
    reposition();
}


void OSDWidget::paintEvent( QPaintEvent* )
{
    bitBlt( this, 0, 0, &osdBuffer );
}

void OSDWidget::mousePressEvent( QMouseEvent* )
{
    hide();
}

void OSDWidget::show()
{
    if ( m_dirty ) renderOSDText( m_currentText );

    QWidget::show();

    if ( m_duration ) //duration 0 -> stay forever
    {
        timer.start( m_duration, TRUE ); //calls hide()
        timerMin.start( 150 ); //calls minReached()
    }
}

void OSDWidget::refresh()
{
    if ( isVisible() )
    {
        //we need to update the buffer
        renderOSDText( m_currentText );
    }
    else m_dirty = true; //ensure we are re-rendered before we are shown
}

void OSDWidget::reposition( QSize newSize )
{
    if( !newSize.isValid() ) newSize = size();

    QPoint newPos( MARGIN, m_y );
    const QRect screen = QApplication::desktop()->screenGeometry( m_screen );

    //TODO m_y is the middle of the OSD, and don't exceed screen margins

    switch ( m_alignment ) {
    case Left:
        break;

    case Right:
        newPos.rx() = screen.width() - MARGIN - newSize.width();
        break;

    case Center:
        newPos.ry() = (screen.height() - newSize.height()) / 2;

        //FALL THROUGH

    case Middle:
        newPos.rx() = (screen.width() - newSize.width()) / 2;
        break;
    }

    // correct for screen position
    newPos += screen.topLeft();

    //ensure we are painted before we move
    if( isVisible() ) paintEvent( 0 );

    //fancy X11 move+resize, reduces visual artifacts
    XMoveResizeWindow( x11Display(), winId(), newPos.x(), newPos.y(), newSize.width(), newSize.height() );
}



//////  OSDPreviewWidget below /////////////////////

#include <kcursor.h>         //previewWidget
#include <klocale.h>

OSDPreviewWidget::OSDPreviewWidget( const QString &appName, QWidget *parent, const char *name )
    : OSDWidget( appName, parent, name )
    , m_dragging( false )
{
    m_currentText = i18n( "OSD Preview - drag to reposition" );
    m_duration    = 0;
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
            m_screen = currentScreen;
            m_y      = QWidget::y();

            emit positionChanged();
        }
    }
}

void OSDPreviewWidget::mouseMoveEvent( QMouseEvent *e )
{
    //TODO make snapZone into percentage of screen size
    //TODO consider using QWidget y() property?
    //TODO do slick move on and then disappear

    if ( m_dragging && this == mouseGrabber() )
    {
        const QRect screen  = QApplication::desktop()->screenGeometry( m_screen );
        const uint  hcenter     = screen.width() / 2;
        const uint  eGlobalPosX = e->globalPos().x();
        const uint  snapZone    = screen.width() / 16;

        QPoint newPos = e->globalPos() - m_dragOffset - screen.topLeft();
        int maxY = screen.height()-height()-MARGIN;
        if( newPos.y() < MARGIN ) newPos.ry() = MARGIN;
        if( newPos.y() > maxY ) newPos.ry() = maxY;

        QPoint destination( MARGIN, newPos.y() );

        if( eGlobalPosX < (hcenter-snapZone) )
        {
            m_alignment = Left;
        }
        else if( eGlobalPosX > (hcenter+snapZone) )
        {
            m_alignment = Right;
            destination.rx() = screen.width() - MARGIN - width();
        }
        else
        {
            const uint eGlobalPosY = e->globalPos().y();
            const uint vcenter     = screen.height()/2;

            destination.rx() = hcenter - width()/2;

            if( eGlobalPosY >= (vcenter-snapZone) && eGlobalPosY <= (vcenter+snapZone) )
            {
                m_alignment = Center;
                destination.ry() = vcenter - height()/2;
            }
            else m_alignment = Middle;
        }

        destination += screen.topLeft();

        move( destination );
    }
}



//////  amaroK::OSD below /////////////////////

#include "metabundle.h"
#include <qregexp.h>

void amaroK::OSD::showTrack( const MetaBundle &bundle )
{
    // Strip HTML tags, expand basic HTML entities
    QString text = bundle.prettyTitle();

    if ( bundle.length() ) {
        text += " - ";
        text += bundle.prettyLength();
    }

    text.replace( QRegExp( "</?(?:font|a|b|i)\\b[^>]*>" ), QString::null );
    text.replace( "&lt;",  "<" );
    text.replace( "&gt;",  ">" );
    text.replace( "&amp;", "&" );

    m_text = text;

    showTrack();
}

#include "osd.moc"
