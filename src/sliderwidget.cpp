/***************************************************************************
                        amarokslider.cpp  -  description
                           -------------------
  begin                : Dec 15 2003
  copyright            : (C) 2003 by Mark Kretschmann
  email                : markey@web.de
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
#include <qbrush.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qsize.h>


amaroK::Slider::Slider( Qt::Orientation orientation, QWidget *parent, uint max )
        : QSlider( orientation, parent )
        , m_sliding( false )
{
    setRange( 0, max );
}

void
amaroK::Slider::wheelEvent( QWheelEvent *e )
{
    uint step = e->delta() / 18;
    if( orientation() == Vertical ) step = -step;
    QSlider::setValue( QSlider::value() + step );

    emit sliderReleased( value() );
}

void
amaroK::Slider::mouseMoveEvent( QMouseEvent *e )
{
    if ( m_sliding )
    {
        QSlider::setValue( orientation() == Horizontal
          ? QRangeControl::valueFromPosition( e->pos().x() - sliderRect().width()/2, width() - sliderRect().width() )
          : QRangeControl::valueFromPosition( e->pos().y() - sliderRect().height()/2, height() - sliderRect().height() ) );

        emit sliderMoved( value() );
    }
    else QSlider::mouseMoveEvent( e );
}

void
amaroK::Slider::mousePressEvent( QMouseEvent *e )
{
    m_sliding = true;

    if ( !sliderRect().contains( e->pos() ) )
        mouseMoveEvent( e );
}

void
amaroK::Slider::mouseReleaseEvent( QMouseEvent* )
{
    m_sliding = false;

    emit sliderReleased( value() );
}

void
amaroK::Slider::setValue( int newValue )
{
    if ( !m_sliding )
         QSlider::setValue( adjustValue( newValue ) );

    //don't adjust the slider while the user is dragging it!
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
amaroK::PrettySlider::mouseMoveEvent( QMouseEvent *e )
{
    if ( m_sliding )
    {
        QSlider::setValue( orientation() == Horizontal
          ? QRangeControl::valueFromPosition( e->pos().x(), width()-2 )
          : QRangeControl::valueFromPosition( e->pos().y(), height()-2 ) );

        emit sliderMoved( value() );
    }
}

void
amaroK::PrettySlider::paintEvent( QPaintEvent* )
{
    const int w   = orientation() == Qt::Horizontal ? width() : height();
    const int pos = double((w-2) * Slider::value()) / maxValue();
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
      p.setPen( 0x80a0ff );
      p.fillRect( 0, 0, pos, h, QColor( 0x002090 ) );
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

#include "sliderwidget.moc"
