/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * osd.cpp:   Shows some text in a pretty way independent to the WM
 * begin:     Fre Sep 26 2003
 * copyright: (C) 2004 Christian Muehlhaeuser <chris@chris.de>
 *            (C) 2004 Seb Ruiz <seb100@optusnet.com.au>
 *            (C) 2004, 2005 Max Howell
 */

#include "amarokconfig.h"
#include "collectiondb.h"    //for albumCover location
#include "debug.h"
#include <kapplication.h>
#include <kpixmap.h>
#include <kpixmapeffect.h>
#include "osd.h"
#include <qbitmap.h>
#include <qpainter.h>
#include <qtimer.h>

namespace ShadowEngine
{
    QImage makeShadow( const QPixmap &textPixmap, const QColor &bgColor );
}

OSDWidget::OSDWidget( QWidget *parent, const char *name )
        : QWidget( parent, name, WType_TopLevel | WNoAutoErase | WStyle_Customize | WX11BypassWM | WStyle_StaysOnTop )
        , m_duration( 2000 )
        , m_timer( new QTimer( this ) )
        , m_alignment( Middle )
        , m_screen( 0 )
        , m_y( MARGIN )
        , m_drawShadow( true )
{
    setFocusPolicy( NoFocus );
    setBackgroundMode( NoBackground );
    unsetColors();

    connect( m_timer, SIGNAL( timeout() ), SLOT( hide() ) );

    //or crashes, KWin bug I think, crashes in QWidget::icon()
    kapp->setTopWidget( this );
}

void
OSDWidget::show() //virtual
{
    class Grabber : public QWidget {
    public:
        Grabber( const QRect &r, const QColor &color ) : QWidget( 0, 0 ) {
            move( 0, 0 );
            screen = QPixmap::grabWindow( winId(), r.x(), r.y(), r.width(), r.height() );
            KPixmapEffect::fade( screen, 0.80, color );
        }
        KPixmap screen;
    };

    if ( !isEnabled() )
        return;

    const QRect oldGeometry = QRect( pos(), size() );

    determineMetrics();

    const QRect newGeometry = QRect( pos(), size() );

    //TODO handle case when already shown properly
    if( !isShown() ) {
        // obtain snapshot of the screen where we are about to appear
        Grabber g( newGeometry, backgroundColor() );
        m_screenshot = g.screen;

        QWidget::show();
    }
    else
        paintEvent( 0 );

    if( m_duration ) //duration 0 -> stay forever
       m_timer->start( m_duration, true ); //calls hide()
}

void
OSDWidget::determineMetrics()
{
    static const uint HMARGIN = 20;
    static const uint VMARGIN = 10;

    // determine a sensible maximum size, don't cover the whole desktop or cross the screen
    const QSize margin( (HMARGIN + MARGIN) * 2, (VMARGIN + MARGIN) * 2 ); //margins
    const QSize image = m_image.isNull() ? QSize( 0, 0 ) : QSize( 80, 80 ); //80x80 is minimum image size
    const QSize max = QApplication::desktop()->screen( m_screen )->size() - margin;

    // The osd cannot be larger than the screen
    QSize text = max - image;
    QRect rect = fontMetrics().boundingRect( 0, 0, text.width(), text.height(), AlignLeft | WordBreak, m_text );

    if( !m_image.isNull() ) {
        const int availableWidth = max.width() - rect.width(); //WILL be >= 80

        int imageMetric;
        imageMetric = QMIN( availableWidth, rect.height() );
        imageMetric = QMIN( imageMetric, m_image.width() );

        const int widthIncludingImage = rect.width()
                + imageMetric
                + VMARGIN; //margin between text + image

        rect.setWidth( QMIN( widthIncludingImage, max.width() ) );

        m_image = m_image.smoothScale( imageMetric, imageMetric );
    }

    // size and move us
    rect.addCoords( -HMARGIN, -VMARGIN, HMARGIN, VMARGIN );
    reposition( rect.size() );
}

void
OSDWidget::reposition( QSize newSize )
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

    resize( newSize );
    move( newPos );
}

void
OSDWidget::paintEvent( QPaintEvent* )
{
    //TODO double buffer? but is slow...

    if( AmarokConfig::osdUseFakeTranslucency() )
       bitBlt( this, 0, 0, &m_screenshot );

    QPainter p;
    QRect rect = this->rect();
    QImage shadow;
    QFontMetrics metrics = fontMetrics();
    Qt::AlignmentFlags align;

    switch( m_alignment ) {
        case Left:  align = Qt::AlignLeft; break;
        case Right: align = Qt::AlignRight; break;
        default:    align = Qt::AlignHCenter; break;
    }

    rect.addCoords( 20, 10, -20, -10 );

    if( m_drawShadow )
    {
        QRect r = rect;
        QPixmap pixmap( size() );

        pixmap.fill( Qt::black );
        pixmap.setMask( pixmap.createHeuristicMask( true ) );

        p.begin( &pixmap );
        p.setFont( font() );
        p.setPen( Qt::white );
        p.setBrush( Qt::white );

        if( !m_image.isNull() ) {
            p.drawRect( 20, 10, m_image.width(), m_image.height() );
            r.rLeft() += m_image.width() + 10;
        }

        p.drawText( r, align | WordBreak, m_text );
        p.end();

        int h,s,v;
        foregroundColor().getHsv( &h, &s, &v );

        shadow = ShadowEngine::makeShadow( pixmap, h < 128 ? Qt::black : Qt::white );
    }

    p.begin( this );
    p.drawImage( 0, 0, shadow );
    p.setPen( foregroundColor() );

    if( !m_image.isNull() ) {
        p.drawImage( 20, 10, m_image );
        rect.rLeft() += m_image.width() + 10;
    }

    p.drawText( rect, align | WordBreak, m_text );
    p.setPen( backgroundColor() );
    p.drawRect( this->rect() );
    p.end();
}

bool
OSDWidget::event( QEvent *e )
{
    switch( e->type() )
    {
        case QEvent::ApplicationPaletteChange:
            if ( !AmarokConfig::osdUseCustomColors() )
                unsetColors(); //use new palette's colours
            return true;
        default:
            return QWidget::event( e );
    }
}

void
OSDWidget::mousePressEvent( QMouseEvent* )
{
    hide();
}

void
OSDWidget::unsetColors()
{
    const QColorGroup c = QApplication::palette().active();

    setPaletteForegroundColor( c.highlightedText() );
    setPaletteBackgroundColor( c.highlight() );
}

void
OSDWidget::setScreen( int screen )
{
    const int n = QApplication::desktop()->numScreens();
    m_screen = (screen >= n) ? n-1 : screen;
    reposition();
}




//////  OSDPreviewWidget below /////////////////////

#include <kcursor.h>         //previewWidget
#include <klocale.h>

OSDPreviewWidget::OSDPreviewWidget( QWidget *parent )
    : OSDWidget( parent, "osdpreview" )
    , m_dragging( false )
{
    m_text = i18n( "OSD Preview - drag to reposition" );
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

void
amaroK::OSD::show( const MetaBundle &bundle ) //slot
{
    // set text to the value in the config file.
    QString text = AmarokConfig::osdText();

    // Use reg exps to remove anything within braces if the tag element is empty.
    // eg: text.replace( QRegExp( "\\{[^}]*%album[^}]*\\}" ), QString::null );
    // OSD: {Album: %album }
    // will not display if bundle.album() is empty.

    QString replaceMe = "\\{[^}]*%1[^}]*\\}";
    QStringList elements, identifiers;
    QString length, bitrate;

    if( bundle.length())
       length = QString ("%1").arg(bundle.prettyLength());
    if( bundle.bitrate() )
       bitrate = QString ("%1").arg(bundle.prettyBitrate());

    // NOTE: Order is important, the items will be evaluated first. Thus, prettyTitle must be last.
    elements    << bundle.artist() << bundle.album() << bundle.title() << bundle.genre()
                << bundle.year() << bundle.track()<< length << bitrate << bundle.prettyTitle();

    identifiers << i18n("%artist") << i18n("%album") << i18n("%title") << i18n("%genre")
                << i18n("%year") << i18n("%track") << i18n( "%length" ) << i18n( "%bitrate" ) << i18n( "%artist - %title" );

    // This loop will go through the two lists and replace each identifier by the appropriate bundle
    // information.

    for( QStringList::ConstIterator id = identifiers.begin(), end = identifiers.end(), el = elements.begin(); id != end; ++id, ++el )
    {
        QString element = *el;
        QString identifier = *id;

        if ( !element.isEmpty() )
            text.replace( identifier, element, FALSE );
        else
        {
            text.replace( QRegExp (replaceMe.arg( identifier ) ), element );
            text.replace( identifier, QString::null, FALSE );
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

    m_text = text.stripWhiteSpace();

    if ( AmarokConfig::osdCover() ) {
        //avoid showing the generic cover.  we can overwrite this by passing an arg.
        //get large cover for scaling if big cover needed
        QString location = CollectionDB::instance()->albumImage( bundle, 0 );

        if ( location.find( "nocover" ) != -1 )
            setImage( QImage() );
        else
            setImage( location );
    }

    OSDWidget::show( m_text );
}

void
amaroK::OSD::applySettings()
{
    setAlignment( (OSDWidget::Alignment)AmarokConfig::osdAlignment() );
    setDuration( AmarokConfig::osdDuration() );
    setEnabled( AmarokConfig::osdEnabled() );
    setOffset( AmarokConfig::osdYOffset() );
    setScreen( AmarokConfig::osdScreen() );
    setFont( AmarokConfig::osdFont() );
    setDrawShadow( AmarokConfig::osdDrawShadow() );

    if( AmarokConfig::osdUseCustomColors() )
    {
        setTextColor( AmarokConfig::osdTextColor() );
        setBackgroundColor( AmarokConfig::osdBackgroundColor() );
    }
    else unsetColors();
}

void
amaroK::OSD::forceToggleOSD()
{
    if ( !isShown() ) {
        const bool b = isEnabled();
        setEnabled( true );
        OSDWidget::show( m_text );
        setEnabled( b );
    }
    else
        hide();
}


/* Code copied from kshadowengine.cpp
 *
 * Copyright (C) 2003 Laur Ivan <laurivan@eircom.net>
 *
 * Many thanks to:
 *  - Bernardo Hung <deciare@gta.igs.net> for the enhanced shadow
 *    algorithm (currently used)
 *  - Tim Jansen <tim@tjansen.de> for the API updates and fixes.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

namespace ShadowEngine
{
    // Not sure, doesn't work above 10
    static const int    MULTIPLICATION_FACTOR = 3;
    // Multiplication factor for pixels directly above, under, or next to the text
    static const double AXIS_FACTOR = 2.0;
    // Multiplication factor for pixels diagonal to the text
    static const double DIAGONAL_FACTOR = 0.1;
    // Self explanatory
    static const int    MAX_OPACITY = 200;

    double decay( QImage&, int, int );

    QImage makeShadow( const QPixmap& textPixmap, const QColor &bgColor )
    {
        QImage result;

        const int w   = textPixmap.width();
        const int h   = textPixmap.height();
        const int bgr = bgColor.red();
        const int bgg = bgColor.green();
        const int bgb = bgColor.blue();

        int alphaShadow;

        // This is the source pixmap
        QImage img = textPixmap.convertToImage().convertDepth( 32 );

        result.create( w, h, 32 );
        result.fill( 0 ); // fill with black
        result.setAlphaBuffer( true );

        static const int M = 5;
        for( int i = M; i < w - M; i++) {
            for( int j = M; j < h - M; j++ )
            {
                alphaShadow = (int) decay( img, i, j );

                result.setPixel( i,j, qRgba( bgr, bgg , bgb, QMIN( MAX_OPACITY, alphaShadow ) ) );
            }
        }

        return result;
    }

    double decay( QImage& source, int i, int j )
    {
        //if ((i < 1) || (j < 1) || (i > source.width() - 2) || (j > source.height() - 2))
        //    return 0;

        double alphaShadow;
        alphaShadow =(qGray(source.pixel(i-1,j-1)) * DIAGONAL_FACTOR +
                qGray(source.pixel(i-1,j  )) * AXIS_FACTOR +
                qGray(source.pixel(i-1,j+1)) * DIAGONAL_FACTOR +
                qGray(source.pixel(i  ,j-1)) * AXIS_FACTOR +
                0                         +
                qGray(source.pixel(i  ,j+1)) * AXIS_FACTOR +
                qGray(source.pixel(i+1,j-1)) * DIAGONAL_FACTOR +
                qGray(source.pixel(i+1,j  )) * AXIS_FACTOR +
                qGray(source.pixel(i+1,j+1)) * DIAGONAL_FACTOR) / MULTIPLICATION_FACTOR;

        return alphaShadow;
    }
}

#include "osd.moc"
