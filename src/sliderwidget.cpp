/***************************************************************************
                        amarokslider.cpp  -  description
                           -------------------
  begin                : Dec 15 2003
  copyright            : (C) 2003 by Mark Kretschmann
  email                : markey@web.de
  copyright            : (C) 2005 by Gábor Lehel
  email                : illissius@gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <config.h>

#include "amarok.h"
#include "app.h"
#include "enginecontroller.h"
#include "sliderwidget.h"

#include <qapplication.h>
#include <qbitmap.h>
#include <qbrush.h>
#include <qimage.h>
#include <qpainter.h>
#include <qsize.h>
#include <qtimer.h>

#include <kiconloader.h>
#include <kimageeffect.h>
#include <klocale.h>
#include <kpixmapeffect.h>
#include <kpopupmenu.h>
#include <kstandarddirs.h>

amaroK::Slider::Slider( Qt::Orientation orientation, QWidget *parent, uint max )
        : QSlider( orientation, parent )
        , m_sliding( false )
        , m_outside( false )
        , m_prevValue( 0 )
{
    setRange( 0, max );
}

void
amaroK::Slider::wheelEvent( QWheelEvent *e )
{
    uint step = e->delta() / 18;
    // Volume Slider
    if( orientation() == Vertical ) step = -step;
    // Position Slider
    else step = step * 1500;
    QSlider::setValue( QSlider::value() + step );

    emit sliderReleased( value() );
}

void
amaroK::Slider::mouseMoveEvent( QMouseEvent *e )
{
    if ( m_sliding )
    {
        //feels better, but using set value of 20 is bad of course
        QRect rect( -20, -20, width()+40, height()+40 );

        if ( orientation() == Horizontal && !rect.contains( e->pos() ) ) {
            if ( !m_outside )
                QSlider::setValue( m_prevValue );
            m_outside = true;
        } else {
            m_outside = false;
            slideEvent( e );
            emit sliderMoved( value() );
        }
    }
    else QSlider::mouseMoveEvent( e );
}

void
amaroK::Slider::slideEvent( QMouseEvent *e )
{
    QSlider::setValue( orientation() == Horizontal
        ? ((QApplication::reverseLayout())
          ? QRangeControl::valueFromPosition( width() - (e->pos().x() - sliderRect().width()/2),  width()  + sliderRect().width() )
          : QRangeControl::valueFromPosition( e->pos().x() - sliderRect().width()/2,  width()  - sliderRect().width() ) )
        : QRangeControl::valueFromPosition( e->pos().y() - sliderRect().height()/2, height() - sliderRect().height() ) );
}

void
amaroK::Slider::mousePressEvent( QMouseEvent *e )
{
    m_sliding   = true;
    m_prevValue = QSlider::value();

    if ( !sliderRect().contains( e->pos() ) )
        mouseMoveEvent( e );
}

void
amaroK::Slider::mouseReleaseEvent( QMouseEvent* )
{
    if( !m_outside && QSlider::value() != m_prevValue )
       emit sliderReleased( value() );

    m_sliding = false;
    m_outside = false;
}

void
amaroK::Slider::setValue( int newValue )
{
    //don't adjust the slider while the user is dragging it!

    if ( !m_sliding || m_outside )
        QSlider::setValue( adjustValue( newValue ) );
    else
        m_prevValue = newValue;
}


//////////////////////////////////////////////////////////////////////////////////////////
/// CLASS PrettySlider
//////////////////////////////////////////////////////////////////////////////////////////

#define THICKNESS 7
#define MARGIN 3

amaroK::PrettySlider::PrettySlider( Qt::Orientation orientation, QWidget *parent, uint max )
    : amaroK::Slider( orientation, parent, max )
{
    setWFlags( Qt::WNoAutoErase );
    setFocusPolicy( QWidget::NoFocus );
}

void
amaroK::PrettySlider::mousePressEvent( QMouseEvent *e )
{
    amaroK::Slider::mousePressEvent( e );

    slideEvent( e );
}

void
amaroK::PrettySlider::slideEvent( QMouseEvent *e )
{
    QSlider::setValue( orientation() == Horizontal
        ? QRangeControl::valueFromPosition( e->pos().x(), width()-2 )
        : QRangeControl::valueFromPosition( e->pos().y(), height()-2 ) );
}

namespace amaroK {
    namespace ColorScheme {
        extern QColor Background;
        extern QColor Foreground;
    }
}

void
amaroK::PrettySlider::paintEvent( QPaintEvent* )
{
    const int w   = orientation() == Qt::Horizontal ? width() : height();
    const int pos = int( double( w-2 ) / maxValue() * Slider::value() );
    const int h   = THICKNESS;

    QPixmap  buf( size() );
    QPainter p( &buf, this );

    buf.fill( parentWidget()->backgroundColor() );

    if ( orientation() == Qt::Vertical )
    {
        p.translate( 0, height()-1 );
        p.rotate( -90 ); //90 degrees clockwise
    }

    p.translate( 0, MARGIN );
      p.setPen( amaroK::ColorScheme::Foreground );
      p.fillRect( 0, 0, pos, h, QColor( amaroK::ColorScheme::Background ) );
      p.drawRect( 0, 0, w, h );
    p.translate( 0, -MARGIN );

    //<Triangle Marker>
    QPointArray pa( 3 );
    pa.setPoint( 0, pos - 3, 1 );
    pa.setPoint( 1, pos + 3, 1 );
    pa.setPoint( 2, pos,     5 );
    p.setBrush( paletteForegroundColor() );
    p.drawConvexPolygon( pa );
    //</Triangle Marker>

    p.end();

    bitBlt( this, 0, 0, &buf );
}

#if 0
/** these functions aren't required in our fixed size world,
    but they may become useful one day **/

QSize
amaroK::PrettySlider::minimumSizeHint() const
{
    return sizeHint();
}

QSize
amaroK::PrettySlider::sizeHint() const
{
    constPolish();

    return (orientation() == Horizontal
             ? QSize( maxValue(), THICKNESS + MARGIN )
             : QSize( THICKNESS + MARGIN, maxValue() )).expandedTo( QApplit ication::globalStrut() );
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////
/// CLASS VolumeSlider
//////////////////////////////////////////////////////////////////////////////////////////

amaroK::VolumeSlider::VolumeSlider( QWidget *parent, uint max )
    : amaroK::Slider( Qt::Horizontal, parent, max )
    , m_animCount( 0 )
    , m_animTimer( new QTimer( this ) )
    , m_pixmapInset( QPixmap( locate( "data","amarok/images/volumeslider-inset.png" ) ) )
{
    setWFlags( getWFlags() | WNoAutoErase );
    setFocusPolicy( QWidget::NoFocus );

    // BEGIN Calculate handle animation pixmaps for mouse-over effect
    QImage pixmapHandle    ( locate( "data","amarok/images/volumeslider-handle.png" ) );
    QImage pixmapHandleGlow( locate( "data","amarok/images/volumeslider-handle_glow.png" ) );

    float opacity = 0.0;
    const float step = 1.0 / ANIM_MAX;
    QImage dst;
    for ( int i = 0; i < ANIM_MAX; ++i ) {
        dst = pixmapHandle;
        KImageEffect::blend( pixmapHandleGlow, dst, opacity );
        m_handlePixmaps.append( QPixmap( dst ) );
        opacity += step;
    }
    // END

    generateGradient();

    setMinimumWidth( m_pixmapInset.width() );
    setMinimumHeight( m_pixmapInset.height() );

    connect( m_animTimer, SIGNAL( timeout() ), this, SLOT( slotAnimTimer() ) );
}

void
amaroK::VolumeSlider::generateGradient()
{
    //QImage temp( locate( "data","amarok/images/volumeslider-gradient.png" ) );
    //KIconEffect::colorize( temp, colorGroup().highlight(), 1.0 );

    const QPixmap temp( locate( "data","amarok/images/volumeslider-gradient.png" ) );
    const QBitmap mask( temp.createHeuristicMask() );

    m_pixmapGradient = QPixmap( m_pixmapInset.size() );
    KPixmapEffect::gradient( m_pixmapGradient, colorGroup().background(), colorGroup().highlight(),
                             KPixmapEffect::HorizontalGradient );
    m_pixmapGradient.setMask( mask );
}

void
amaroK::VolumeSlider::slotAnimTimer() //SLOT
{
    if ( m_animEnter ) {
        m_animCount++;
        repaint( false );
        if ( m_animCount == ANIM_MAX - 1 )
            m_animTimer->stop();
    } else {
        m_animCount--;
        repaint( false );
        if ( m_animCount == 0 )
            m_animTimer->stop();
    }
}

void
amaroK::VolumeSlider::mousePressEvent( QMouseEvent *e )
{
    if( e->button() == RightButton )
    {
        KPopupMenu menu;
        menu.insertTitle( i18n( "Volume" ) );
        menu.insertItem(  i18n(   "100%" ), 100 );
        menu.insertItem(  i18n(    "80%" ),  80 );
        menu.insertItem(  i18n(    "60%" ),  60 );
        menu.insertItem(  i18n(    "40%" ),  40 );
        menu.insertItem(  i18n(    "20%" ),  20 );
        menu.insertItem(  i18n(     "0%" ),   0 );

        if( EngineController::hasEngineProperty( "HasEqualizer" ) )
        {
            menu.insertSeparator();
            menu.insertItem( SmallIconSet( "equalizer" ), i18n( "&Equalizer" ),
                kapp, SLOT( slotConfigEqualizer() ) );
        }

        const int n = menu.exec( mapToGlobal( e->pos() ) );
        if( n >= 0 )
        {
            QSlider::setValue( n );
            emit sliderReleased( n );
        }
    }
    else
    {
        amaroK::Slider::mousePressEvent( e );

        slideEvent( e );
    }
}

void
amaroK::VolumeSlider::slideEvent( QMouseEvent *e )
{
    QSlider::setValue( QRangeControl::valueFromPosition( e->pos().x(), width()-2 ) );
}

void
amaroK::VolumeSlider::wheelEvent( QWheelEvent *e )
{
    const uint step = e->delta() / amaroK::VOLUME_SENSITIVITY;
    QSlider::setValue( QSlider::value() + step );

    emit sliderReleased( value() );
}

void
amaroK::VolumeSlider::paintEvent( QPaintEvent * )
{
    QPixmap buf( size() );

    // Erase background
    if( parentWidget()->backgroundPixmap() )
        buf.fill( parentWidget(), pos() );
    else {
        buf.fill( colorGroup().background() );
//         QPainter p( &buf );
//         p.fillRect( rect(), qApp->palette().brush( QPalette::Active, QColorGroup::Background ) );
    }

    const int padding = 7;
    const int offset = int( double( ( width() - 2 * padding ) * value() ) / maxValue() );

    bitBlt( &buf, 0, 0, &m_pixmapGradient, 0, 0, offset + padding );
    bitBlt( &buf, 0, 0, &m_pixmapInset );
    bitBlt( &buf, offset - m_handlePixmaps[0].width() / 2 + padding, 0, &m_handlePixmaps[m_animCount] );

    // Draw percentage number
    QPainter p( &buf );
    p.setPen( palette().color( QPalette::Disabled, QColorGroup::Text ).dark() );
    QFont font;
    font.setPixelSize( 9 );
    p.setFont( font );
    const QRect rect( 0, 0, 34, 15 );
    p.drawText( rect, Qt::AlignRight | Qt::AlignVCenter, QString::number( value() ) + "%" );
    p.end();

    bitBlt( this, 0, 0, &buf );
}

void
amaroK::VolumeSlider::hideEvent( QHideEvent* )
{
    setBackgroundMode( PaletteBackground ); // Required to prevent erasing
}

void
amaroK::VolumeSlider::showEvent( QShowEvent* )
{
    // HACK To prevent ugly uninitialised background when the window is shown,
    //      needed because we use NoBackground to prevent flickering while painting
    setBackgroundMode( NoBackground );
}

void
amaroK::VolumeSlider::enterEvent( QEvent* )
{
    m_animEnter = true;
    m_animCount = 0;

    m_animTimer->start( ANIM_INTERVAL );
}

void
amaroK::VolumeSlider::leaveEvent( QEvent* )
{
    // This can happen if you enter and leave the widget quickly
    if ( m_animCount == 0 )
        m_animCount = 1;

    m_animEnter = false;
    m_animTimer->start( ANIM_INTERVAL );
}

void
amaroK::VolumeSlider::paletteChange( const QPalette& )
{
    generateGradient();
}

#include "sliderwidget.moc"
