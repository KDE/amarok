/****************************************************************************************
 * Copyright (c) 2004 Christian Muehlhaeuser <chris@chris.de>                           *
 * Copyright (c) 2004-2006 Seb Ruiz <ruiz@kde.org>                                      *
 * Copyright (c) 2004,2005 Max Howell <max.howell@methylblue.com>                       *
 * Copyright (c) 2005 Gabor Lehel <illissius@gmail.com>                                 *
 * Copyright (c) 2008-2013 Mark Kretschmann <kretschmann@kde.org>                       *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "OSD"

#include "Osd.h"

#include "EngineController.h"
#include "KNotificationBackend.h"
#include "PaletteHandler.h"
#include "SvgHandler.h"
#include "amarokconfig.h"
#include "core/meta/Meta.h"
#include "core/meta/Statistics.h"
#include "core/meta/support/MetaUtility.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "widgets/StarManager.h"

#include <QApplication>
#include <QIcon>
#include <KLocalizedString>
#include <KWindowSystem>
#include <KIconLoader>

#include <QDesktopWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QRegExp>
#include <QScreen>
#include <QTimeLine>
#include <QTimer>

namespace ShadowEngine
{
    QImage makeShadow( const QPixmap &textPixmap, const QColor &bgColor );
}

namespace Amarok
{
    inline QImage icon() { return QImage( KIconLoader::global()->iconPath( "amarok", -KIconLoader::SizeHuge ) ); }
}

OSDWidget::OSDWidget( QWidget *parent, const char *name )
        : QWidget( parent )
        , m_duration( 2000 )
        , m_timer( new QTimer( this ) )
        , m_alignment( Middle )
        , m_screen( 0 )
        , m_yOffset( MARGIN )
        , m_rating( 0 )
        , m_volume( The::engineController()->volume() )
        , m_showVolume( false )
        , m_hideWhenFullscreenWindowIsActive( false )
        , m_fadeTimeLine( new QTimeLine( FADING_DURATION, this ) )
{
    Qt::WindowFlags flags;
    flags = Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint;
    // The best of both worlds.  On Windows, setting the widget as a popup avoids a task manager entry.  On linux, a popup steals focus.
    // Therefore we go need to do it platform specific :(

    //This is no longer true. Qt::Window steals focus on X11, Qt:Tool does not. Not sure if we even need the ifdefs any more...
    #ifdef Q_OS_WIN
    flags |= Qt::Tool;
    #else
    flags |= Qt::Tool | Qt::X11BypassWindowManagerHint;
    #endif
    setWindowFlags( flags );
    setObjectName( name );
    setFocusPolicy( Qt::NoFocus );

    #ifdef Q_WS_X11
    KWindowSystem::setType( winId(), NET::Notification );
    #endif

    m_timer->setSingleShot( true );
    connect( m_timer, &QTimer::timeout, this, &OSDWidget::hide );

    m_fadeTimeLine->setUpdateInterval( 30 ); //~33 frames per second 
    connect( m_fadeTimeLine, &QTimeLine::valueChanged, this, &OSDWidget::setFadeOpacity );

    //or crashes, KWindowSystem bug I think, crashes in QWidget::icon()
    //kapp->setTopWidget( this );
}

OSDWidget::~OSDWidget()
{
    DEBUG_BLOCK
}

void
OSDWidget::show( const QString &text, const QImage &newImage )
{
    DEBUG_BLOCK
    m_showVolume = false;
    if ( !newImage.isNull() )
    {
        m_cover = newImage;
        int w = m_scaledCover.width();
        int h = m_scaledCover.height();
        m_scaledCover = QPixmap::fromImage( m_cover.scaled( w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation ) );
    }
    else
        m_cover = Amarok::icon();

    m_text = text;
    show();
}

void
OSDWidget::show()
{
    if ( !isTemporaryDisabled() )
    {
        QWidget::show();

        if( windowOpacity() == 0.0 && KWindowSystem::compositingActive() )
        {
            m_fadeTimeLine->setDirection( QTimeLine::Forward );
            m_fadeTimeLine->start();
        }
        // Skip fading if OSD is already visible or if compositing is disabled
        else
        {
            m_fadeTimeLine->stop();
            setWindowOpacity( maxOpacity() );
        }
    }
}

void
OSDWidget::hide()
{
    if( KWindowSystem::compositingActive() )
    {
        m_fadeTimeLine->setDirection( QTimeLine::Backward );
        m_fadeTimeLine->start();
    }
    else
    {
        QWidget::hide();
    }
}

bool
OSDWidget::isTemporaryDisabled() const
{
    // Check if the OSD should not be shown,
    // if a fullscreen window is focused.
    if ( m_hideWhenFullscreenWindowIsActive )
    {
        return Amarok::KNotificationBackend::instance()->isFullscreenWindowActive();
    }

    return false;
}

void
OSDWidget::ratingChanged( const QString& path, int rating )
{
    Meta::TrackPtr track = The::engineController()->currentTrack();
    if( !track )
        return;
    if( track->playableUrl().isLocalFile() && track->playableUrl().path() == path )
        ratingChanged( rating );
}

void
OSDWidget::ratingChanged( const short rating )
{
    m_text = QLatin1Char('\n') + i18n( "Rating changed" );
    setRating( rating ); //Checks isEnabled() before doing anything

    show();
}

void
OSDWidget::volumeChanged( int volume )
{
    m_volume = volume;

    if ( isEnabled() )
    {
        m_showVolume = true;
        m_text = The::engineController()->isMuted() ? i18n( "Volume: %1% (muted)", m_volume) : i18n( "Volume: %1%", m_volume);

        show();
    }
}

void
OSDWidget::setVisible( bool visible )
{
    if ( visible )
    {
        if ( !isEnabled() || m_text.isEmpty() )
            return;

        const uint margin = fontMetrics().horizontalAdvance( 'x' );

        const QRect newGeometry = determineMetrics( margin );

        if( newGeometry.width() > 0 && newGeometry.height() > 0 )
        {
            m_margin = margin;
            m_size = newGeometry.size();
            setGeometry( newGeometry );
            QWidget::setVisible( visible );

            if( m_duration ) //duration 0 -> stay forever
                m_timer->start( m_duration ); //calls hide()
        }
        else
            warning() << "Attempted to make an invalid sized OSD\n";

        update();
    }
    else
        QWidget::setVisible( visible );
}

QRect
OSDWidget::determineMetrics( const int M )
{
    // sometimes we only have a tiddly cover
    const QSize minImageSize = m_cover.size().boundedTo( QSize( 100, 100 ) );

    // determine a sensible maximum size, don't cover the whole desktop or cross the screen
    const QSize margin( ( M + MARGIN ) * 2, ( M + MARGIN ) * 2 ); //margins
    const QSize image = m_cover.isNull() ? QSize( 0, 0 ) : minImageSize;
    const QSize max = QApplication::screens()[ screen() ]->size() - margin;

    // If we don't do that, the boundingRect() might not be suitable for drawText() (Qt issue N67674)
    m_text.replace( QRegExp( " +\n" ), "\n" );
    // remove consecutive line breaks
    m_text.replace( QRegExp( "\n+" ), "\n" );

    // The osd cannot be larger than the screen
    QRect rect = fontMetrics().boundingRect( 0, 0, max.width() - image.width(), max.height(),
        Qt::AlignCenter, m_text );
    rect.adjust( 0, 0, SHADOW_SIZE * 2, SHADOW_SIZE * 2 ); // the shadow needs some space

    if( m_showVolume )
    {
        static const QString tmp = QString ("******").insert( 3,
            ( i18n("Volume: 100% (muted)" ) ) );

        QRect tmpRect = fontMetrics().boundingRect( 0, 0,
            max.width() - image.width(), max.height() - fontMetrics().height(),
            Qt::AlignCenter, tmp );
        tmpRect.setHeight( tmpRect.height() + fontMetrics().height() / 2 );

        rect = tmpRect;

        if ( The::engineController()->isMuted() )
            m_cover = The::svgHandler()->renderSvg( "Muted", 100, 100, "Muted" ).toImage();
        else if( m_volume > 66 )
            m_cover = The::svgHandler()->renderSvg( "Volume", 100, 100, "Volume" ).toImage();
        else if ( m_volume > 33 )
            m_cover = The::svgHandler()->renderSvg( "Volume_mid", 100, 100, "Volume_mid" ).toImage();
        else
            m_cover = The::svgHandler()->renderSvg( "Volume_low", 100, 100, "Volume_low" ).toImage();
    }
    // Don't show both volume and rating
    else if( m_rating )
    {
        QPixmap* star = StarManager::instance()->getStar( 1 );
        if( rect.width() < star->width() * 5 )
            rect.setWidth( star->width() * 5 ); //changes right edge position
        rect.setHeight( rect.height() + star->height() + M ); //changes bottom edge pos
    }

    if( !m_cover.isNull() )
    {
        const int availableWidth = max.width() - rect.width() - M; //WILL be >= (minImageSize.width() - M)

        m_scaledCover = QPixmap::fromImage(
                m_cover.scaled(
                    qMin( availableWidth, m_cover.width() ),
                    qMin( rect.height(), m_cover.height() ),
                    Qt::KeepAspectRatio, Qt::SmoothTransformation
                              )
                                          ); //this will force us to be with our bounds


        const int widthIncludingImage = rect.width()
                + m_scaledCover.width()
                + M; //margin between text + image

        rect.setWidth( widthIncludingImage );
    }

    // expand in all directions by M
    rect.adjust( -M, -M, M, M );

    const QSize newSize = rect.size();
    const QRect screenRect = QApplication::screens()[ screen() ]->geometry();
    QPoint newPos( MARGIN, m_yOffset );

    switch( m_alignment )
    {
        case Left:
            break;

        case Right:
            newPos.rx() = screenRect.width() - MARGIN - newSize.width();
            break;

        case Center:
            newPos.ry() = ( screenRect.height() - newSize.height() ) / 2;

            Q_FALLTHROUGH();

        case Middle:
            newPos.rx() = ( screenRect.width() - newSize.width() ) / 2;
            break;
    }

    //ensure we don't dip below the screen
    if ( newPos.y() + newSize.height() > screenRect.height() - MARGIN )
        newPos.ry() = screenRect.height() - MARGIN - newSize.height();

    // correct for screen position
    newPos += screenRect.topLeft();

    return QRect( newPos, rect.size() );
}

void
OSDWidget::paintEvent( QPaintEvent *e )
{
    QRect rect( QPoint(), m_size );

    QColor shadowColor;
    {
        int h, s, v;
        palette().color( QPalette::Normal, QPalette::WindowText ).getHsv( &h, &s, &v );
        shadowColor = v > 128 ? Qt::black : Qt::white;
    }

    const int align = Qt::AlignCenter;

    QPainter p( this );
    p.setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing );
    p.setClipRect( e->rect() );

    QPixmap background = The::svgHandler()->renderSvgWithDividers( "service_list_item", width(), height(), "service_list_item" );
    p.drawPixmap( 0, 0, background );

    //p.setPen( Qt::white ); // Revert this when the background can be colorized again.
    rect.adjust( m_margin, m_margin, -m_margin, -m_margin ); // subtract margins

    if( !m_cover.isNull() )
    {
        QRect r( rect );
        r.setTop( ( m_size.height() - m_scaledCover.height() ) / 2 );
        r.setSize( m_scaledCover.size() );

        p.drawPixmap( r.topLeft(), m_scaledCover );

        rect.setLeft( rect.left() + m_scaledCover.width() + m_margin );
    }

    int graphicsHeight = 0;

    if( !m_showVolume && m_rating > 0 && !m_paused )
    {
        // TODO: Check if we couldn't use a KRatingPainter instead
        QPixmap* star = StarManager::instance()->getStar( m_rating/2 );
        QRect r( rect );

        //Align to center...
        r.setLeft( ( rect.left() + rect.width() / 2 ) - star->width() * m_rating / 4 );
        r.setTop( rect.bottom() - star->height() );
        graphicsHeight += star->height() + m_margin;

        const bool half = m_rating % 2;

        if( half )
        {
            QPixmap* halfStar = StarManager::instance()->getHalfStar( m_rating / 2 + 1 );
            p.drawPixmap( r.left() + star->width() * ( m_rating / 2 ), r.top(), *halfStar );
            star = StarManager::instance()->getStar( m_rating / 2 + 1 );
        }

        for( int i = 0; i < m_rating / 2; i++ )
        {
            p.drawPixmap( r.left() + i * star->width(), r.top(), *star );
        }
    }

    rect.setBottom( rect.bottom() - graphicsHeight );

    // Draw "shadow" text effect (black outline) (currently it's up to five pixel in every dir.)
    QPixmap pixmap( rect.size() );
    pixmap.fill( Qt::black );

    QPainter p2( &pixmap );
    p2.setFont( font() );
    p2.setPen( Qt::white );
    p2.setBrush( Qt::white );
    p2.drawText( QRect( QPoint( SHADOW_SIZE, SHADOW_SIZE ),
                        QSize( rect.size().width() - SHADOW_SIZE * 2,
                               rect.size().height() - SHADOW_SIZE * 2 ) ),
                 align, m_text );
    p2.end();

    p.drawImage( rect.topLeft(), ShadowEngine::makeShadow( pixmap, shadowColor ) );

    p.setPen( palette().color( QPalette::Active, QPalette::WindowText ) );

    p.drawText( rect.adjusted( SHADOW_SIZE, SHADOW_SIZE,
                               -SHADOW_SIZE, -SHADOW_SIZE ), align, m_text );
}

void
OSDWidget::changeEvent( QEvent *event )
{
    QWidget::changeEvent( event );

    if( event->type() == QEvent::PaletteChange )
        if( !AmarokConfig::osdUseCustomColors() )
            unsetColors(); // Use new palette's colors
}

void
OSDWidget::mousePressEvent( QMouseEvent* )
{
    hide();
}

void
OSDWidget::unsetColors()
{
    setPalette( The::paletteHandler()->palette() );
}

void
OSDWidget::setTextColor(const QColor& color)
{
    QPalette palette = this->palette();
    palette.setColor( QPalette::Active, QPalette::WindowText, color );
    setPalette(palette);
}

void
OSDWidget::setScreen( int screen )
{
    const int n = QApplication::screens().size();
    m_screen = ( screen >= n ) ? n - 1 : screen;
}

void
OSDWidget::setFadeOpacity( qreal value )
{
    setWindowOpacity( value * maxOpacity() );

    if( value == 0.0 )
    {
        QWidget::hide();
    }
}

void
OSDWidget::setFontScale( int scale )
{
    double fontScale = static_cast<double>( scale ) / 100.0;

    // update font, reuse old one
    QFont newFont( font() );
    newFont.setPointSizeF( defaultPointSize() * fontScale );
    setFont( newFont );
}

void
OSDWidget::setHideWhenFullscreenWindowIsActive( bool hide )
{
    m_hideWhenFullscreenWindowIsActive = hide;
}


/////////////////////////////////////////////////////////////////////////////////////////
// Class OSDPreviewWidget
/////////////////////////////////////////////////////////////////////////////////////////

OSDPreviewWidget::OSDPreviewWidget( QWidget *parent )
        : OSDWidget( parent )
        , m_dragging( false )
{
    setObjectName( "osdpreview" );
    setDuration( 0 );
    setImage( Amarok::icon() );
    setTranslucent( AmarokConfig::osdUseTranslucency() );
    setText( i18n( "On-Screen-Display preview\nDrag to reposition" ) );
}

void
OSDPreviewWidget::mousePressEvent( QMouseEvent *event )
{
    m_dragYOffset = event->pos();

    if( event->button() == Qt::LeftButton && !m_dragging )
    {
        grabMouse( Qt::SizeAllCursor );
        m_dragging = true;
    }
}

void
OSDPreviewWidget::setUseCustomColors(const bool use, const QColor& fg)
{
    if( use )
        setTextColor( fg );
    else
        unsetColors();
}

void
OSDPreviewWidget::mouseReleaseEvent( QMouseEvent * /*event*/ )
{
    if( m_dragging )
    {
        m_dragging = false;
        releaseMouse();

        Q_EMIT positionChanged();
    }
}

void
OSDPreviewWidget::mouseMoveEvent( QMouseEvent *e )
{
    if( m_dragging && this == mouseGrabber() )
    {
        // Here we implement a "snap-to-grid" like positioning system for the preview widget

        const QRect screenRect  = QApplication::screens()[ screen() ]->geometry();
        const uint  hcenter     = screenRect.width() / 2;
        const uint  eGlobalPosX = e->globalPos().x() - screenRect.left();
        const uint  snapZone    = screenRect.width() / 24;

        QPoint destination = e->globalPos() - m_dragYOffset - screenRect.topLeft();
        int maxY = screenRect.height() - height() - MARGIN;
        if( destination.y() < MARGIN )
            destination.ry() = MARGIN;
        if( destination.y() > maxY )
            destination.ry() = maxY;

        if( eGlobalPosX < ( hcenter - snapZone ) )
        {
            setAlignment(Left);
            destination.rx() = MARGIN;
        }
        else if( eGlobalPosX > ( hcenter + snapZone ) )
        {
            setAlignment(Right);
            destination.rx() = screenRect.width() - MARGIN - width();
        }
        else {
            const uint eGlobalPosY = e->globalPos().y() - screenRect.top();
            const uint vcenter     = screenRect.height() / 2;

            destination.rx() = hcenter - width() / 2;

            if( eGlobalPosY >= ( vcenter - snapZone ) && eGlobalPosY <= ( vcenter + snapZone ) )
            {
                setAlignment(Center);
                destination.ry() = vcenter - height() / 2;
            }
            else
                setAlignment(Middle);
        }

        destination += screenRect.topLeft();
        move( destination );

        // compute current Position && Y-offset
        const int currentScreen = QGuiApplication::screens().indexOf( QGuiApplication::screenAt( pos() ) );

        // set new data
        OSDWidget::setScreen( currentScreen );
        setYOffset( y() );
    }
}


/////////////////////////////////////////////////////////////////////////////////////////
// Class OSD
/////////////////////////////////////////////////////////////////////////////////////////

Amarok::OSD* Amarok::OSD::s_instance = nullptr;

Amarok::OSD*
Amarok::OSD::instance()
{
    return s_instance ? s_instance : new OSD();
}

void
Amarok::OSD::destroy()
{
    if ( s_instance )
    {
        delete s_instance;
        s_instance = nullptr;
    }
}

Amarok::OSD::OSD()
    : OSDWidget( nullptr )
{
    s_instance = this;

    EngineController* const engine = The::engineController();

    if( engine->isPlaying() )
        trackPlaying( engine->currentTrack() );

    connect( engine, &EngineController::trackPlaying,
             this, &Amarok::OSD::trackPlaying );
    connect( engine, &EngineController::stopped,
             this, &Amarok::OSD::stopped );
    connect( engine, &EngineController::paused,
             this, &Amarok::OSD::paused );

    connect( engine, &EngineController::trackMetadataChanged,
             this, &Amarok::OSD::metadataChanged );
    connect( engine, &EngineController::albumMetadataChanged,
             this, &Amarok::OSD::metadataChanged );

    connect( engine, &EngineController::volumeChanged,
             this, &Amarok::OSD::volumeChanged );

    connect( engine, &EngineController::muteStateChanged,
             this, &Amarok::OSD::muteStateChanged );

}

Amarok::OSD::~OSD()
{}

void
Amarok::OSD::show( Meta::TrackPtr track ) //slot
{
    setAlignment( static_cast<OSDWidget::Alignment>( AmarokConfig::osdAlignment() ) );
    setYOffset( AmarokConfig::osdYOffset() );

    QString text;
    if( !track || track->playableUrl().isEmpty() )
    {
        text = i18n( "No track playing" );
        setRating( 0 ); // otherwise stars from last rating change are visible
    }
    else
    {
        setRating( track->statistics()->rating() );
        text = track->prettyName();
        if( track->artist() && !track->artist()->prettyName().isEmpty() )
            text = track->artist()->prettyName() + " - " + text;
        if( track->album() && !track->album()->prettyName().isEmpty() )
            text += "\n (" + track->album()->prettyName() + ") ";
        else
            text += '\n';
        if( track->length() > 0 )
            text += Meta::msToPrettyTime( track->length() );
    }

    if( text.isEmpty() )
        text =  track->playableUrl().fileName();

    if( text.startsWith( "- " ) ) //When we only have a title tag, _something_ prepends a fucking hyphen. Remove that.
        text = text.mid( 2 );

    if( text.isEmpty() ) //still
        text = i18n("No information available for this track");

    QImage image;
    if( track && track->album() )
        image = The::svgHandler()->imageWithBorder( track->album(), 100, 5 ).toImage();

    OSDWidget::show( text, image );
}

void
Amarok::OSD::applySettings()
{
    DEBUG_BLOCK

    setAlignment( static_cast<OSDWidget::Alignment>( AmarokConfig::osdAlignment() ) );
    setDuration( AmarokConfig::osdDuration() );
    setEnabled( AmarokConfig::osdEnabled() );
    setYOffset( AmarokConfig::osdYOffset() );
    setScreen( AmarokConfig::osdScreen() );
    setFontScale( AmarokConfig::osdFontScaling() );
    setHideWhenFullscreenWindowIsActive( AmarokConfig::osdHideOnFullscreen() );

    if( AmarokConfig::osdUseCustomColors() )
        setTextColor( AmarokConfig::osdTextColor() );
    else
        unsetColors();

    setTranslucent( AmarokConfig::osdUseTranslucency() );
}

void
Amarok::OSD::forceToggleOSD()
{
    if ( !isVisible() )
    {
        const bool b = isEnabled();
        setEnabled( true );
        show( The::engineController()->currentTrack() );
        setEnabled( b );
    }
    else
        hide();
}

void
Amarok::OSD::muteStateChanged( bool mute )
{
    Q_UNUSED( mute )

    volumeChanged( The::engineController()->volume() );
}

void
Amarok::OSD::trackPlaying( const Meta::TrackPtr &track )
{
    m_currentTrack = track;

    setPaused(false);
    show( m_currentTrack );
}

void
Amarok::OSD::stopped()
{
    setImage( QImage( KIconLoader::global()->iconPath( "amarok", -KIconLoader::SizeHuge ) ) );
    setRating( 0 ); // otherwise stars from last rating change are visible
    OSDWidget::show( i18n( "Stopped" ) );
    setPaused(false);
}

void
Amarok::OSD::paused()
{
    setImage( QImage( KIconLoader::global()->iconPath( "amarok", -KIconLoader::SizeHuge ) ) );
    setRating( 0 ); // otherwise stars from last rating change are visible
    OSDWidget::show( i18n( "Paused" ) );
    setPaused(true);
}

void
Amarok::OSD::metadataChanged()
{
    // this also covers all cases where a stream get's new metadata.
    show( m_currentTrack );
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
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
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
        const int w   = textPixmap.width();
        const int h   = textPixmap.height();
        const int bgr = bgColor.red();
        const int bgg = bgColor.green();
        const int bgb = bgColor.blue();

        int alphaShadow;

        // This is the source pixmap
        QImage img = textPixmap.toImage();

        QImage result( w, h, QImage::Format_ARGB32 );
        result.fill( 0 ); // fill with black

        static const int M = OSDWidget::SHADOW_SIZE;
        for( int i = M; i < w - M; i++) {
            for( int j = M; j < h - M; j++ )
            {
                alphaShadow = (int) decay( img, i, j );

                result.setPixel( i,j, qRgba( bgr, bgg , bgb, qMin( MAX_OPACITY, alphaShadow ) ) );
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

