/*
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

/*
  osd.cpp  -  Provides an interface to a plain QWidget, which is independent of KDE (bypassed to X11)
  begin:     Fre Sep 26 2003
  copyright: (C) 2003 by Christian Muehlhaeuser, 2004 by Seb Ruiz
  email:     chris@chris.de; seb100@optusnet.com.au
*/

#include "amarokconfig.h" //previewWidget
#include "osd.h"
#include "collectiondb.h" //for albumCover location

#include <qapplication.h>
#include <qbitmap.h>
#include <qpainter.h>
#include <qimage.h> 

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


void OSDWidget::renderOSDText( const QString &text , const QString &imageLocation )
{
    static QBitmap mask;

    // Set a sensible maximum size, don't cover the whole desktop or cross the screen
    QSize max = QApplication::desktop() ->screen( m_screen ) ->size() - QSize( MARGIN*2 + 20, 100 );
    QFont titleFont( "Arial", 12, QFont::Bold );
    QFontMetrics titleFm( titleFont );
    
    // The title cannnot be taller than one line
    QRect titleRect = titleFm.boundingRect( 0, 0, max.width(), titleFm.height(), AlignLeft, m_appName );
    // The osd cannot be larger than the screen
    QRect textRect = fontMetrics().boundingRect( 0, 0, max.width(), max.height(), AlignLeft | WordBreak, text );
    int showCover = 0;
    // determine appropriate image size based on size of screen
    int imageSize = QApplication::desktop()->screen( m_screen )->width() / 16;
    
    // we don't want to show the cover if it is the generic one.
    if ( imageLocation.find( QString("nocover") ) == -1 && m_cover && !imageLocation.isEmpty())
        showCover = 1;
        
    if ( textRect.width() < titleRect.width() )
        textRect.setWidth( titleRect.width() );

    //this should still be within the screen bounds
    QImage image = QImage::QImage();
    if ( showCover )
    {
        // does the file exist? if not, tell the osd where to shove it.
        bool result = image.load( imageLocation );
        if (result == false)
            showCover = 0;
        else
            image = image.smoothScale( imageSize, imageSize );
    }
    //dimensions
    if ( showCover )
    {
        if ( textRect.height() + titleRect.height() < (imageSize + 20) )
            textRect.setHeight( imageSize + 20 );
        else
            textRect.setBottom( titleRect.height() + textRect.height() );
            
        // we add pixels to the width because of the image size, and 40 for padding;
        // 10px before image, 10px after image, 20px after text
        textRect.addCoords( 0, 0, imageSize + 40, 0);
    }
    else
    {
        // add 20 pixels to the width (so the text isn't on the border), and add the height of the titleRect
        // so we can see the last line!
        textRect.addCoords( 0, 0, 20, titleRect.height() );
    }
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
    //text position in Rect.
    int textPosition = 0;
    //shadow offset.
    int shadowOffset = 0;
    //image position.
    int imagePosition = 0;
    

    // Paint the album cover if existant
    if ( showCover )
    {
        if ( text.isRightToLeft() )
        {
            imagePosition = -10;
            bufferPainter.drawImage( textRect.width() - imageSize - 10, -imagePosition, image );
            imagePosition -= imageSize;
        }
        else
        {
            imagePosition = 10;
            bufferPainter.drawImage( imagePosition, imagePosition, image );
            imagePosition += imageSize;
        }
    }
    
    //set text position according to direction.
    if ( text.isRightToLeft() )
    {
        textPosition = imagePosition - 10;
        shadowOffset = -3;
    } else
    {
        textPosition = imagePosition + 10;
        shadowOffset = 3;
    } //text position set

    // Draw the text shadow
    if ( m_shadow )
    {
        bufferPainter.setPen( backgroundColor().dark( 175 ) );
        bufferPainter.drawText( textPosition+shadowOffset, titleFm.height() + 1, w, h, AlignAuto | WordBreak, text );
    }

    // Draw the text
    bufferPainter.setPen( foregroundColor() );
    bufferPainter.drawText( textPosition, titleFm.height() - 1, w, h, AlignAuto | WordBreak, text );

    // Draw the title text
    bufferPainter.setFont( titleFont );
    bufferPainter.drawText( textPosition, 3, w, h, AlignLeft, m_appName );

    // Masking for transparency
    mask.fill( Qt::black );
    maskPainter.setBrush( Qt::white );
    maskPainter.drawRoundRect( textRect, 1500 / textRect.width(), 1500 / textRect.height() );
    setMask( mask );

    // Do last to reduce noticeable change when showing multiple OSDs in succession
    reposition( textRect.size() );

    m_currentText = text;
    m_dirty = false;

    update();
}


void OSDWidget::show( const QString &text, const QString &image, bool preemptive )
{
    if ( isEnabled() && !text.isEmpty() )
    {
        if ( preemptive || !timerMin.isActive() )
        {
            m_currentText = text;
            m_currentImage = image;
            m_dirty = true;

            show();
        } else {
            textBuffer.append( text ); //queue
            imageBuffer.append( image ); //queue
        }
    }
}


void OSDWidget::minReached() //SLOT
{
    if ( !textBuffer.isEmpty() )
    {
        renderOSDText( textBuffer.front(), imageBuffer.front() );
        textBuffer.pop_front();
        imageBuffer.pop_front();

        if( m_duration )
            //timerMin is still running
            timer.start( m_duration, TRUE );
    } else
        timerMin.stop();
}


void OSDWidget::setDuration( int ms )
{
    m_duration = ms;

    if( !m_duration )
        timer.stop();
}


void OSDWidget::setFont(const QFont &newFont )
{
    QWidget::setFont( newFont );
    refresh();
}


void OSDWidget::setShadow( bool shadow )
{
    m_shadow = shadow;
    refresh();
}


void OSDWidget::setTextColor( const QColor &newColor )
{
    setPaletteForegroundColor( newColor );
    refresh();
}


void OSDWidget::setBackgroundColor( const QColor &newColor )
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


void OSDWidget::setScreen( int screen )
{
    const int n = QApplication::desktop()->numScreens();
    m_screen = (screen >= n) ? n-1 : screen;
    reposition();
}


bool OSDWidget::event( QEvent *e )
{
    switch( e->type() )
    {
        case QEvent::Paint:
            bitBlt( this, 0, 0, &osdBuffer );
            return TRUE;

        case QEvent::ApplicationPaletteChange:
            if ( !AmarokConfig::osdUseCustomColors() ) //FIXME not portable!
                unsetColors(); //updates colors for new palette
            return TRUE;

        default:
            return QWidget::event( e );
    }
}


void OSDWidget::mousePressEvent( QMouseEvent* )
{
    hide();
}


void OSDWidget::show()
{
    if ( m_dirty )
        renderOSDText( m_currentText, m_currentImage );

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
        renderOSDText( m_currentText , m_currentImage );
    } else
        m_dirty = true; //ensure we are re-rendered before we are shown
}


void OSDWidget::reposition( QSize newSize )
{
    if( !newSize.isValid() ) newSize = size();

    QPoint newPos( MARGIN, m_y );
    const QRect screen = QApplication::desktop()->screenGeometry( m_screen );

    //TODO m_y is the middle of the OSD, and don't exceed screen margins

    switch ( m_alignment )
    {
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

    //ensure we don't dip below the screen
    if ( newPos.y() + newSize.height() > screen.height() - MARGIN )
        newPos.ry() = screen.height() - MARGIN - newSize.height();

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

    if ( event->button() == LeftButton && !m_dragging )
    {
        grabMouse( KCursor::sizeAllCursor() );
        m_dragging = true;
    }
}


void OSDPreviewWidget::mouseReleaseEvent( QMouseEvent * /*event*/ )
{
    if ( m_dragging )
    {
        m_dragging = false;
        releaseMouse();

        // compute current Position && offset
        QDesktopWidget *desktop = QApplication::desktop();
        int currentScreen = desktop->screenNumber( pos() );

        if ( currentScreen != -1 )
        {
            // set new data
            m_screen = currentScreen;
            m_y      = QWidget::y();

            emit positionChanged();
        }
    }
}


void OSDPreviewWidget::mouseMoveEvent( QMouseEvent *e )
{
    if ( m_dragging && this == mouseGrabber() )
    {
        const QRect screen      = QApplication::desktop()->screenGeometry( m_screen );
        const uint  hcenter     = screen.width() / 2;
        const uint  eGlobalPosX = e->globalPos().x() - screen.left();
        const uint  snapZone    = screen.width() / 8;

        QPoint destination = e->globalPos() - m_dragOffset - screen.topLeft();
        int maxY = screen.height() - height() - MARGIN;
        if( destination.y() < MARGIN ) destination.ry() = MARGIN;
        if( destination.y() > maxY ) destination.ry() = maxY;

        if( eGlobalPosX < (hcenter-snapZone) )
        {
            m_alignment = Left;
            destination.rx() = MARGIN;
        }
        else if( eGlobalPosX > (hcenter+snapZone) )
        {
            m_alignment = Right;
            destination.rx() = screen.width() - MARGIN - width();
        }
        else
        {
            const uint eGlobalPosY = e->globalPos().y() - screen.top();
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

amaroK::OSD*
amaroK::OSD::instance()
{
    static OSD osd;
    return &osd;
}


void
amaroK::OSD::showTrack( const MetaBundle &bundle ) //slot
{
    // set text to the value in the config file.
    QString text = AmarokConfig::osdText();

    // Use reg exps to remove anything within braces if the tag element is empty.
    // eg: text.replace( QRegExp( "\\{[^}]*%album[^}]*\\}" ), QString::null );
    // OSD: {Album: %album }
    // will not display if bundle.album() is empty.

    QString replaceMe = "\\{[^}]*%1[^}]*\\}";
    QStringList element, identifier;
    
    QString length, bitrate = QString::null;
    
    if( bundle.length())
        length = QString ("%1").arg(bundle.prettyLength());
    if( bundle.bitrate() )
        bitrate = QString ("%1").arg(bundle.prettyBitrate());
    
    element    << bundle.album() << bundle.track() << bundle.genre() << bundle.year()
               << bundle.track()<< length << bitrate;
    identifier << i18n("%album") << i18n("%track") << i18n("%genre") << i18n("%year")
               << i18n("%number") << i18n( "%length" ) << i18n( "%bitrate" );
        
    if ( bundle.artist().isEmpty() ) {
        text.replace( "%artist", bundle.prettyTitle( bundle.url().fileName() ), FALSE );
        text.replace( "%track", QString::null , FALSE);
    } else {
        text.replace( "%artist", bundle.artist() , FALSE );
        text.replace( "%track", bundle.title() , FALSE );
    }

    // Lets get the location of the cover image
    QString image = CollectionDB().albumImage( bundle.artist(), bundle.album() );

    for ( uint x = 0; x < identifier.count(); ++x )
    {
        if ( !element[x].isEmpty() )
            text.replace( identifier[x], element[x], FALSE );
        else
        {
            text.replace( QRegExp (replaceMe.arg( identifier[x] ) ), element[x] );
            text.replace( identifier[x], QString::null, FALSE );
        }
    }
    
    // If we end up replacing many lines with QString::null, we could get blank lines.  lets remove them.
    text.replace( QRegExp( "\n+" ) , "\n" );
    text.replace( QRegExp( "\n +\n" ) , "\n" );

    // remove the braces.
    text.replace( "{", QString::null );
    text.replace( "}", QString::null );

    text.replace( QRegExp( "</?(?:font|a|b|i)\\b[^>]*>" ), QString::null );
    text.replace( "&lt;",  "<" );
    text.replace( "&gt;",  ">" );
    text.replace( "&amp;", "&" );
    text.replace( "\\n", "\n" );

    m_text = text;
    m_image = image;

    showTrack();
}


void
amaroK::OSD::applySettings()
{
    setAlignment( (OSDWidget::Alignment)AmarokConfig::osdAlignment() );
    setDuration( AmarokConfig::osdDuration() );
    setEnabled( AmarokConfig::osdEnabled() );
    setOffset( AmarokConfig::osdXOffset(), AmarokConfig::osdYOffset() );
    setScreen( AmarokConfig::osdScreen() );
    setShadow( AmarokConfig::osdDrawShadow() );
    setFont( AmarokConfig::osdFont() );
    setCover( AmarokConfig::osdCover() );

    if( AmarokConfig::osdUseCustomColors() )
    {
        setTextColor( AmarokConfig::osdTextColor() );
        setBackgroundColor( AmarokConfig::osdBackgroundColor() );
    }
    else unsetColors();
}

#include "osd.moc"
