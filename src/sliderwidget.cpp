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

#include "sliderwidget.h"

#include <math.h>

#include <qapplication.h>
#include <qbrush.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qsize.h>

#include <kglobal.h>
#include <kiconeffect.h>
#include <kpixmap.h>
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
    const int pos = int(double((w-2) * Slider::value()) / maxValue());
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
{
    setWFlags( getWFlags() | WNoAutoErase );
    setFocusPolicy( QWidget::NoFocus );

    m_volumeslider_inset  = QPixmap( locate( "data","amarok/images/volumeslider-inset.png" ) );
    m_volumeslider_handle = QPixmap( locate( "data","amarok/images/volumeslider-handle.png" ) );
    generateGradient();

    setMinimumWidth( m_volumeslider_inset.width() );
}

void
amaroK::VolumeSlider::generateGradient()
{
    QImage temp( locate( "data","amarok/images/volumeslider-gradient.png" ) );
    KIconEffect::colorize( temp, colorGroup().highlight(), 1.0 );

    m_volumeslider_gradient.convertFromImage( temp );
}

void
amaroK::VolumeSlider::mousePressEvent( QMouseEvent *e )
{
    amaroK::Slider::mousePressEvent( e );

    slideEvent( e );
}

void
amaroK::VolumeSlider::slideEvent( QMouseEvent *e )
{
    const double x = kClamp( e->x() + 1, 1, width() ), h = height(), w = width();
    QSlider::setValue( int( maxValue() * ( ( x * ( ( h / w ) * x ) ) / ( h * w ) ) ) );
}

void
amaroK::VolumeSlider::wheelEvent( QWheelEvent *e )
{
    const uint step = e->delta() / 18;
    // Volume Slider
    QSlider::setValue( QSlider::value() + step );

    emit sliderReleased( value() );
}

void
amaroK::VolumeSlider::paintEvent( QPaintEvent * )
{
    QPixmap buf( size() );

    // Erase buffer pixmap
    QPainter p( &buf );
    p.fillRect( rect(), colorGroup().background() );
    p.end();

    const double w = width(), v = value(), mV = maxValue();
    const int h = m_volumeslider_inset.height();
    const int offset = int( sqrt( (w*w) * ( v / mV ) ) );

    bitBlt( &buf, 0, height() / 2 - h / 2, &m_volumeslider_inset, 0, 0 );
    bitBlt( &buf, 0, height() / 2 - h / 2, &m_volumeslider_gradient, 0, 0, offset );
    bitBlt( &buf, offset - m_volumeslider_handle.width() / 2, h - m_volumeslider_handle.height() / 2 - 3, &m_volumeslider_handle );

    // Draw percentage number
    p.begin( &buf );
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
amaroK::VolumeSlider::paletteChange( const QPalette& )
{
    generateGradient();
}

#include "sliderwidget.moc"
