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
    if ( !isEnabled() || m_text.isEmpty() )
        return;

    const uint M = fontMetrics().width( 'x' );

    const QRect oldGeometry = QRect( pos(), size() );
    const QRect newGeometry = determineMetrics( M );

    if( m_translucency && !isShown() || !newGeometry.intersects( oldGeometry ) ) {
        m_screenshot = QPixmap::grabWindow( qt_xrootwin(), newGeometry.x(), newGeometry.y(), newGeometry.width(), newGeometry.height() );
        setGeometry( newGeometry );
        render( M );
        QWidget::show();
    }

    else if( m_translucency ) {
        const QRect unite = oldGeometry.unite( newGeometry );
        KPixmap pix = QPixmap::grabWindow( qt_xrootwin(), unite.x(), unite.y(), unite.width(), unite.height() );
        QPoint p = oldGeometry.topLeft() - unite.topLeft();

        bitBlt( &pix, p, &m_screenshot );

        m_screenshot.resize( newGeometry.size() );

        p = newGeometry.topLeft() - unite.topLeft();
        bitBlt( &m_screenshot, 0, 0, &pix, p.x(), p.y() );

        setGeometry( newGeometry );
        render( M );
    }

    if( newGeometry.width() > 0 && newGeometry.height() > 0 )
    {
        setGeometry( newGeometry );
        render( M );
        QWidget::show();

        if( m_duration ) //duration 0 -> stay forever
            m_timer->start( m_duration, true ); //calls hide()
    }
    else
        warning() << "Attempted to make an invalid sized OSD\n";
}

QRect
OSDWidget::determineMetrics( const uint M )
{
    // determine a sensible maximum size, don't cover the whole desktop or cross the screen
    const QSize margin( (M + MARGIN) * 2, (M + MARGIN) * 2 ); //margins
    const QSize image = m_cover.isNull() ? QSize( 0, 0 ) : QSize( 80, 80 ); //80x80 is minimum image size
    const QSize max = QApplication::desktop()->screen( m_screen )->size() - margin;

    // The osd cannot be larger than the screen
    QSize text = max - image;
    QRect rect = fontMetrics().boundingRect( 0, 0, text.width(), text.height(), AlignLeft | WordBreak, m_text );

    if( !m_cover.isNull() ) {
        const int availableWidth = max.width() - rect.width(); //WILL be >= 80
        const int imageMetric = QMIN( availableWidth, rect.height() );

        m_scaledCover = m_cover.smoothScale( imageMetric, imageMetric, QImage::ScaleMin );

        const int widthIncludingImage = rect.width()
                + m_scaledCover.width()
                + M; //margin between text + image

        rect.setWidth( QMIN( widthIncludingImage, max.width() ) );
    }

    // size and move us
    rect.addCoords( -M, -M, M, M );

    return QRect( reposition( rect.size() ), rect.size() );
}

QPoint
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

    return newPos;
}

void
OSDWidget::render( const uint M )
{
    const uint xround = (M * 200) / width();
    const uint yround = (M * 200) / height();

    {   /// apply the mask
        static QBitmap mask;

        mask.resize( size() );
        mask.fill( Qt::black );

        QPainter p( &mask );
        p.setBrush( Qt::white );
        p.drawRoundRect( rect(), xround, yround );
        setMask( mask );
    }

    QColor shadowColor;
    {
        int h,s,v;
        foregroundColor().getHsv( &h, &s, &v );
        shadowColor = v > 128 ? Qt::black : Qt::white;
    }

    Qt::AlignmentFlags align;
    switch( m_alignment ) {
        case Left:  align = Qt::AlignLeft; break;
        case Right: align = Qt::AlignRight; break;
        default:    align = Qt::AlignHCenter; break;
    }

    QPixmap buffer( size() );
    QPainter p( &buffer );

    if( m_translucency ) {
        KPixmap background( m_screenshot );
        KPixmapEffect::fade( background, 0.80, backgroundColor() );
        p.drawPixmap( 0, 0, background );
    }
    else
        p.eraseRect( rect() );

    p.setPen( backgroundColor().dark() );
    p.drawRoundRect( rect(), xround, yround );

    QRect rect = this->rect();
    rect.addCoords( M, M, -M, -M );

    if( false && m_drawShadow )
    {
        QRect r = rect;
        QPixmap pixmap( size() );

        pixmap.fill( Qt::black );
        pixmap.setMask( pixmap.createHeuristicMask( true ) );

        QPainter p2( &pixmap );
        p2.setFont( font() );
        p2.setPen( Qt::white );
        p2.setBrush( Qt::white );

        if( !m_cover.isNull() )
            r.rLeft() += m_cover.width() + M;

        p2.drawText( r, align | WordBreak, m_text );
        p2.end();

        p.drawImage( 0, 0, ShadowEngine::makeShadow( pixmap, shadowColor ) );
    }

    if( !m_cover.isNull() ) {
        const uint y = (height() - m_scaledCover.height()) / 2;
        p.drawPixmap( M, y, m_scaledCover );

        if( !m_scaledCover.hasAlpha() ) {
            // don't draw a border for eg, the amaroK icon
            p.setPen( shadowColor );
            p.drawRect( QRect(QPoint(M-1,M-1), m_scaledCover.size() + QSize(2,2)) );
        }
        rect.rLeft() += m_scaledCover.width() + M;
    }

    p.setPen( foregroundColor() );
    p.setFont( font() );
    p.drawText( rect, align | WordBreak, m_text );
    p.end();

    bitBlt( this, 0, 0, &buffer );
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
    move( reposition() );
}




//////  OSDPreviewWidget below /////////////////////

#include <kcursor.h>
#include <kiconloader.h>
#include <klocale.h>

namespace amaroK
{
    QImage icon() { return QImage( KIconLoader().iconPath( "amarok", -KIcon::SizeHuge ) ); }
}

OSDPreviewWidget::OSDPreviewWidget( QWidget *parent )
        : OSDWidget( parent, "osdpreview" )
        , m_dragging( false )
{
    m_text = i18n( "OSD Preview - drag to reposition" );
    m_duration = 0;
    m_cover = amaroK::icon();
    m_translucency = false; //doesn't work well when you move it about etc.
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
    QString text = AmarokConfig::osdText();
    QStringList tags, tokens;

    // we special case prettyTitle and put it first
    // so that we handle things like streams better
    tokens << i18n("%artist - %title")
           << i18n("%artist") << i18n("%album") << i18n("%title")   << i18n("%genre")
           << i18n("%year")   << i18n("%track") << i18n("%bitrate") << i18n("%length")
           << i18n("%file");

    tags   << bundle.prettyTitle()
           << bundle.artist() << bundle.album() << bundle.title() << bundle.genre()
           << bundle.year() << bundle.track() << bundle.prettyBitrate()
           << (bundle.length() > 0 ? bundle.prettyLength() : QString::null) //ignore '-' or '?'
           << bundle.url().fileName();

    for( QStringList::ConstIterator tok = tokens.begin(), end = tokens.end(), tag = tags.begin(); tok != end; ++tok, ++tag )
    {
        if( (*tag).isEmpty() ) {
            text.remove( QRegExp(QString("\\{[^}]*%1[^{]*\\}").arg( *tok )) );
            text.remove( *tok ); //above only works if {} surround the token
        }
        else
            //NOTE leaves the {} braces
            text.replace( *tok, *tag );
    }

    text.remove( '{' );
    text.remove( '}' );
    text.replace( QRegExp("\n{2,}"), "\n" ); //multiple \n characters may remain
    text.replace( "&lt;",  "<" );
    text.replace( "&gt;",  ">" );
    text.replace( "&amp;", "&" ); //replace some common HTML type stuff

    // KDE 3.3 rejects \n in the .kcfg file, and KConfig turns \n into \\n, so...
    text.replace( "\\n", "\n" );

    if ( AmarokConfig::osdCover() ) {
        //avoid showing the generic cover.  we can overwrite this by passing an arg.
        //get large cover for scaling if big cover needed
        QString location = CollectionDB::instance()->albumImage( bundle, 0 );

        if ( location.find( "nocover" ) != -1 )
            setImage( amaroK::icon() );
        else
            setImage( location );
    }

    m_text = text.stripWhiteSpace();

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
    setTranslucency( AmarokConfig::osdUseFakeTranslucency() );

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
