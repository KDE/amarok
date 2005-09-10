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

#include <qapplication.h>
#include <qbitmap.h>
#include <qbrush.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qsize.h>
#include <kpixmap.h>
#include <kpixmapeffect.h>


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

    drawGradients();
}

void
amaroK::VolumeSlider::drawGradients()
{
    m_lightGradient.resize( size() );
    m_darkGradient.resize( size() );
    m_mask.resize( size() );

    int h, s, v;
    QColor c1, c2;
    colorGroup().highlight().getHsv( h, s, v );
    c1.setHsv( h, 255/4, v );
    c2.setHsv( h, 255, v );

    KPixmapEffect::gradient( m_lightGradient, colorGroup().background(), c1, KPixmapEffect::HorizontalGradient );
    KPixmapEffect::gradient( m_darkGradient, colorGroup().background(), c2, KPixmapEffect::HorizontalGradient );

    m_mask.fill( Qt::color0 );

    QPainter p( &m_mask );
    p.setPen( Qt::color1 );
    p.setBrush( Qt::color1 );
    QPointArray pa( 3 );
    pa.setPoint( 0, 0, height() / 2 + drawHeight() / 2 - 1 );
    pa.setPoint( 1, width() - 1, height() / 2 + drawHeight() / 2 - 1 );
    pa.setPoint( 2, width() - 1, height() / 2 - drawHeight() / 2 - 1  );
    p.drawConvexPolygon( pa );
    p.end();

    m_lightGradient.setMask( m_mask );
    m_darkGradient.setMask( m_mask );
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
    QSlider::setValue( QRangeControl::valueFromPosition( e->pos().x(), width()-2 ) );
}

void
amaroK::VolumeSlider::wheelEvent( QWheelEvent *e )
{
    uint step = e->delta() / 18;
    // Volume Slider
    setValue( QSlider::value() + step );

    emit sliderReleased( value() );
}

void
amaroK::VolumeSlider::paintEvent( QPaintEvent * )
{
    QPixmap buf( size() );

    QPainter p( &buf );
    p.fillRect( rect(), colorGroup().background() );
    p.end();

    const int offset = int(double(width() * value()) / maxValue());

    bitBlt( &buf, offset, 0, &m_lightGradient, offset, 0, width() - offset  );
    bitBlt( &buf, 0, 0, &m_darkGradient, 0, 0, offset );

    bitBlt( this, 0, 0, &buf );
}

void
amaroK::VolumeSlider::resizeEvent( QResizeEvent * )
{
    drawGradients();
}

#include "sliderwidget.moc"
