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

#include <qapplication.h> //globalStut() function
#include <qbrush.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qpointarray.h>
#include <qrangecontrol.h>
#include <qsize.h>

#include <kglobal.h>

#define THICKNESS 7
#define MARGIN 3

//---<init>---
amaroK::Slider::Slider( QWidget *parent, Qt::Orientation orient, VDirection dir )
        : QWidget( parent, "amaroK::Slider", Qt::WRepaintNoErase )
        , QRangeControl()
        , m_isPressed( false )
        , m_orientation( orient )
        , m_dir( dir )
{
    setFocusPolicy( QWidget::NoFocus );
    setSizePolicy( (orient == Horizontal)
                   ? QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed )
                   : QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Expanding ) );
}
//---</init>---


QSize amaroK::Slider::minimumSizeHint() const
{
    return sizeHint();
}


void amaroK::Slider::setValue( int val )
{
    if ( val != value() )
    {
        QRangeControl::setValue( val );
        update();
        emit valueChanged( val );
    }
}


QSize amaroK::Slider::sizeHint() const
{
    constPolish();

    if ( m_orientation == Horizontal )
        return QSize( 1, THICKNESS + MARGIN ).expandedTo( QApplication::globalStrut() );
    else
        return QSize( THICKNESS + MARGIN, 1 ).expandedTo( QApplication::globalStrut() );
}


//---<events>---
void amaroK::Slider::mouseMoveEvent( QMouseEvent *e )
{
    if ( m_isPressed )
    {
        if ( m_orientation == Qt::Horizontal )
            setValue( valueFromPosition( e->pos().x(), width() ) );
        else
            setValue( valueFromPosition( height() - 3 - e->pos().y(), height() - 3 ) );
    }
}


void amaroK::Slider::mousePressEvent( QMouseEvent *e )
{
    if( e->button() == Qt::LeftButton )
    {
        m_isPressed = true;

        mouseMoveEvent( e );

        emit sliderPressed();
    }
}


void amaroK::Slider::mouseReleaseEvent( QMouseEvent * )
{
    m_isPressed = false;
    emit sliderReleased();
}


void amaroK::Slider::paintEvent( QPaintEvent * )
{
    int length = ( m_orientation == Qt::Horizontal ) ? width() : height();
    int val = value();
    if (m_orientation == Vertical && m_dir == BottomUp)
        val = maxValue() - val;
    int pos = positionFromValue( val, length-2-1 );

    QPixmap pBufPixmap( width(), height() );
    //bitBlt( &pBufPixmap, 0, 0, parentWidget(), x(), y(), width(), height() );
    pBufPixmap.fill( parentWidget()->backgroundColor() );

    QPainter p( &pBufPixmap, this );

    if ( m_orientation == Qt::Vertical )
    {
        p.translate( 0, height()-1 );
        p.rotate( -90 );
        pos = length-2-1 - pos;
    }

    p.translate( 0, MARGIN );
    p.setPen( QColor( 0x80a0ff ) );
    p.drawRect( 0, 0, length-1, THICKNESS-1 );
    p.fillRect( 1, 1, pos,      THICKNESS-2-1,
                QBrush( QColor( 0x00, 0x20, 0x90 ), QBrush::SolidPattern ) );
    p.translate( 0, -MARGIN );

    //<Triangle Marker>
    QPointArray pa( 3 );
    pa.setPoint( 0, pos - 3, 1 );
    pa.setPoint( 1, pos + 3, 1 );
    pa.setPoint( 2, pos,     5 );
    p.setBrush( QBrush( paletteForegroundColor(), QBrush::SolidPattern ) );
    p.drawConvexPolygon( pa );
    //</Triangle Marker>

    p.end();
    bitBlt( this, 0, 0, &pBufPixmap );
}


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS PlaylistSlider
//////////////////////////////////////////////////////////////////////////////////////////

amaroK::PlaylistSlider::PlaylistSlider( QSlider::Orientation orientation, QWidget* parent, const char* name )
    : QSlider( orientation, parent, name )
{}


void amaroK::PlaylistSlider::mousePressEvent( QMouseEvent* e )
{
    if ( sliderRect().contains( e->pos() ) )
        // Clicks on the handle will be processed by QSlider
        QSlider::mousePressEvent( e );
    else {
        // We catch clicks outside of the handle, and reposition directly
        int newVal;
        
        if ( orientation() == Horizontal )
            newVal = static_cast<int>( (float) e->x() / (float) width() * (float) maxValue() );
        else
            newVal = static_cast<int>( (float) e->y() / (float) height() * (float) maxValue() );
        
        setValue( newVal );
        emit sliderMoved( newVal );
    }
}


void amaroK::PlaylistSlider::wheelEvent( QWheelEvent* e )
{
    // PlaylistSlider generates a sliderMoved event when using wheel. The statusbar handles
    // the 'scroll' by seeking in the track.
    
    // Invert delta --> Moving wheel forwards seeks forward.
    QWheelEvent event( e->pos(), ( -1 ) * e->delta(), e->state(), e->orientation() );
    
    QSlider::wheelEvent( &event );
    emit sliderMoved( value() );
}


#include "sliderwidget.moc"
