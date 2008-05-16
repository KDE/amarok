/***************************************************************************
                        amarokslider.cpp  -  description
                           -------------------
  begin                : Dec 15 2003
  copyright            : (C) 2003 by Mark Kretschmann
  email                : markey@web.de
  copyright            : (C) 2005 by Gábor Lehel
  email                : illissius@gmail.com
  copyright            : (C) 2008 by Dan Meltzer
  email                : hydrogen@notyetimplemented.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "SliderWidget.h"

#include <config-amarok.h>

#include "Amarok.h"
#include "amarokconfig.h"
#include "App.h"
#include "Debug.h"
#include "EngineController.h"
#include "SvgHandler.h"
#include "SvgTinter.h"
#include "TheInstances.h"

#include <QBitmap>
#include <QBrush>
#include <QContextMenuEvent>
#include <QImage>
#include <QMenu>
#include <QPainter>
#include <QPixmapCache>
#include <QStyle>
#include <QStyleOptionComplex>
#include <QSvgRenderer>
#include <QTimer>

#include <KIcon>
#include <KLocale>
#include <KMenu>
#include <KStandardDirs>

Amarok::Slider::Slider( Qt::Orientation orientation, QWidget *parent, uint max )
    : QSlider( orientation, parent )
    , m_sliding( false )
    , m_outside( false )
    , m_prevValue( 0 )
{
    setRange( 0, max );
}

void
Amarok::Slider::wheelEvent( QWheelEvent *e )
{
    DEBUG_BLOCK

    if( orientation() == Qt::Vertical )
    {
        // Will be handled by the parent widget
        e->ignore();
        return;
    }

    // Position Slider (horizontal)
    int step = e->delta() / VOLUME_SENSITIVITY;
    int nval = QSlider::value() + step;
    nval = qMax(nval, minimum());
    nval = qMin(nval, maximum());

    QSlider::setValue( nval );

    emit sliderReleased( value() );
}

void
Amarok::Slider::mouseMoveEvent( QMouseEvent *e )
{
    if ( m_sliding )
    {
        //feels better, but using set value of 20 is bad of course
        QRect rect( -20, -20, width()+40, height()+40 );

        if ( orientation() == Qt::Horizontal && !rect.contains( e->pos() ) )
        {
            if ( !m_outside )
                QSlider::setValue( m_prevValue );
            m_outside = true;
        }
        else
        {
            m_outside = false;
            slideEvent( e );
            emit sliderMoved( value() );
        }
    }
    else
        QSlider::mouseMoveEvent( e );
}

void
Amarok::Slider::slideEvent( QMouseEvent *e )
{
    QStyleOptionComplex complex;
    QRect rect = style()->subControlRect( QStyle::CC_Slider, &complex , QStyle::SC_SliderHandle, this );

    QSlider::setValue( orientation() == Qt::Horizontal
        ? ((QApplication::isRightToLeft())
          ? QStyle::sliderValueFromPosition( minimum(), maximum(), width() - (e->pos().x() - rect.width()/2),  width()  + rect.width() )
          : QStyle::sliderValueFromPosition( minimum(), maximum(), e->pos().x() - rect.width()/2,  width()  - rect.width() ) )
        : QStyle::sliderValueFromPosition( minimum(), maximum(), e->pos().y() - rect.height()/2, height() - rect.height() ) );
}

void
Amarok::Slider::mousePressEvent( QMouseEvent *e )
{
    QStyleOptionComplex complex;
    QRect rect = style()->subControlRect( QStyle::CC_Slider, &complex , QStyle::SC_SliderHandle, this );
    m_sliding   = true;
    m_prevValue = QSlider::value();

    if ( !rect.contains( e->pos() ) )
        mouseMoveEvent( e );
}

void
Amarok::Slider::mouseReleaseEvent( QMouseEvent* )
{
    if( !m_outside && QSlider::value() != m_prevValue )
       emit sliderReleased( value() );

    m_sliding = false;
    m_outside = false;
}

void
Amarok::Slider::setValue( int newValue )
{
    //don't adjust the slider while the user is dragging it!
    if ( !m_sliding || m_outside )
        QSlider::setValue( adjustValue( newValue ) );
    else
        m_prevValue = newValue;
}


void Amarok::Slider::paintCustomSlider( QPainter *p, int x, int y, int width, int height, double pos )
{

    static const short side = 5; // Size of the rounded parts.

    double knobX;
    if( pos < 0 )
        knobX = ( width - height ) * ( ( double ) value() / 100.0 );
    else
        knobX = pos;
    
    const double fillLength = knobX + ( height / 2 );

    p->drawPixmap( x + side, y, The::svgHandler()->renderSvg( "slider_center", width - side * 2, height, "slider_center" ) );
    p->drawPixmap( x, y, The::svgHandler()->renderSvg( "slider_left", side, height, "slider_left" ) );
    p->drawPixmap( x, y, The::svgHandler()->renderSvg( "slider_left_highlight", side, height, "slider_left_highlight" ) );
    p->drawPixmap( x + width - side , y, The::svgHandler()->renderSvg( "slider_right", side, height, "slider_right" ) );

    //tile this to make it look good!
    int tileWidth = 16;
    QPixmap sliderTile = The::svgHandler()->renderSvg( "slider_center_highlight", tileWidth, height, "slider_center_highlight" );

    int offset = side;
    int xMax =  knobX + height / 2;
    while( ( offset + tileWidth ) <= xMax ) {
        p->drawPixmap( x + offset , y, sliderTile );
        offset += tileWidth;
    }

//paint as much of the last tile as needed
    int leftover = xMax - offset;
    if ( leftover > 0 )
        p->drawPixmap( x + offset, y, sliderTile, 0, 0, leftover, height );

    p->drawPixmap( x + knobX, y, The::svgHandler()->renderSvg( "volume_slider_position", height, height, "slider_position" ) );
}


//////////////////////////////////////////////////////////////////////////////////////////
/// CLASS VolumeSlider
//////////////////////////////////////////////////////////////////////////////////////////

Amarok::VolumeSlider::VolumeSlider( QWidget *parent, uint max )
    : Amarok::Slider( Qt::Horizontal, parent, max )
{
    setFocusPolicy( Qt::NoFocus );
    m_margin = 4;
}

void
Amarok::VolumeSlider::mousePressEvent( QMouseEvent *e )
{
    if( e->button() != Qt::RightButton )
    {
        Amarok::Slider::mousePressEvent( e );
        slideEvent( e );
    }
}

void
Amarok::VolumeSlider::contextMenuEvent( QContextMenuEvent *e )
{
    QMenu menu;
    menu.setTitle(   i18n( "Volume" ) );
    menu.addAction(  i18n(   "100%" ) )->setData( 100 );
    menu.addAction(  i18n(    "80%" ) )->setData(  80 );
    menu.addAction(  i18n(    "60%" ) )->setData(  60 );
    menu.addAction(  i18n(    "40%" ) )->setData(  40 );
    menu.addAction(  i18n(    "20%" ) )->setData(  20 );
    menu.addAction(  i18n(     "0%" ) )->setData(   0 );

    if( false ) //TODO phonon
    {
        menu.addSeparator();
        menu.addAction( KIcon( "view-media-equalizer-amarok" ), i18n( "&Equalizer" ), kapp, SLOT( slotConfigEqualizer() ) )
            ->setData( -1 );
    }

    QAction* a = menu.exec( mapToGlobal( e->pos() ) );
    if( a )
    {
        const int n = a->data().toInt();
        if( n >= 0 )
        {
            QSlider::setValue( n );
            emit sliderReleased( n );
        }
    }
}

void
Amarok::VolumeSlider::slideEvent( QMouseEvent *e )
{
    int x = e->pos().x();

    //is event witin slider bounds?
    if ( ( x >= m_sliderX ) && ( x <= m_sliderX + m_sliderWidth ) )
    {
        QSlider::setValue( QStyle::sliderValueFromPosition( minimum(), maximum(), e->pos().x() - m_sliderX, m_sliderWidth-2 ) );
    }
}

void
Amarok::VolumeSlider::wheelEvent( QWheelEvent *e )
{
    const uint step = e->delta() / Amarok::VOLUME_SENSITIVITY;
    QSlider::setValue( QSlider::value() + step );

    emit sliderReleased( value() );
}

void
Amarok::VolumeSlider::paintEvent( QPaintEvent * )
{
    QPainter *p = new QPainter( this );

    paintCustomSlider( p, m_sliderX, ( m_iconHeight -m_sliderHeight ) / 2 , m_sliderWidth, m_sliderHeight );
    p->drawPixmap( 0, 0, The::svgHandler()->renderSvg( "volume_icon", m_iconWidth, m_iconHeight, "volume_icon" ) ) ;

    if( underMouse() )
    {
        // Draw percentage number
        p->setPen( palette().color( QPalette::Active, QColorGroup::Text ) );
        QFont font;
        font.setPixelSize( 12 );
        p->setFont( font );
        const QRect rect( m_iconWidth + m_sliderWidth, ( int ) ( height() - 15 ) / 2, 40, 15 );
        p->drawText( rect, Qt::AlignRight | Qt::AlignVCenter, QString::number( value() ) + '%' );
    }

    delete p;
}

void
Amarok::VolumeSlider::paletteChange( const QPalette& )
{
    The::svgHandler()->reTint();
}

void Amarok::VolumeSlider::resizeEvent(QResizeEvent * event)
{
    Q_UNUSED( event );
    m_iconHeight = static_cast<int>( height() * 0.66 );
    m_iconWidth  = static_cast<int>( m_iconHeight * 1.33 );
    m_textWidth  = 40;
    m_sliderWidth = width() - ( m_iconWidth + m_textWidth + m_margin  );
    m_sliderHeight = (int)m_sliderWidth / 7; //maintain sane aspect ratio
    if ( m_sliderHeight > height() )
        m_sliderHeight = height();

    m_sliderX = m_iconWidth + m_margin;
}



//////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// TIMESLIDER ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

Amarok::TimeSlider::TimeSlider( QWidget *parent )
    : Amarok::Slider( Qt::Horizontal, parent )
    , m_animTimer( new QTimer( this ) )
    , m_frame( 0 )
    , m_knobX( 0.0 )
    , m_positionChange( 0.0 )
    , m_oldValue( 0 )
{
    setFocusPolicy( Qt::NoFocus );
    connect( m_animTimer, SIGNAL( timeout() ), SLOT( slotUpdateAnim() ) );
}

void
Amarok::TimeSlider::setSliderValue( int value )
{
    if( value > 0 && value > m_oldValue /*don't animate if we go backwards..*/ )
    {
        m_frame = 1; // Reset the frame, as it animates between values.
        m_knobX = QStyle::sliderPositionFromValue( minimum(), maximum(), value, width() );
        double oldKnobX = QStyle::sliderPositionFromValue( minimum(), maximum(), m_oldValue, width() );
        m_positionChange = ( m_knobX - oldKnobX ) / FRAME_RATE;
        m_animTimer->start( TICK_INTERVAL / FRAME_RATE );
    }
    else
    {
        m_knobX = QStyle::sliderPositionFromValue( minimum(), maximum(), value, width() );
    }
    m_oldValue = value;
    repaint();
    Amarok::Slider::setValue( value );
}

void
Amarok::TimeSlider::slotUpdateAnim()
{
    if( m_frame < FRAME_RATE - 1 )
    {
        m_frame++;
        m_knobX += m_positionChange;
        repaint();
    }
    else
        m_animTimer->stop();
}

void
Amarok::TimeSlider::paintEvent( QPaintEvent * )
{

    QPainter *p = new QPainter( this );
    paintCustomSlider( p, 0, ( height() - m_sliderHeight ) / 2, width(), m_sliderHeight, m_knobX );
    delete p;
}

void
Amarok::TimeSlider::paletteChange( const QPalette& )
{
    The::svgHandler()->reTint();
}

void Amarok::TimeSlider::resizeEvent(QResizeEvent * event)
{
    Q_UNUSED( event );
    m_sliderHeight = (int)width() / 25; //maintain sane aspect ratio
    if ( m_sliderHeight > height() )
        m_sliderHeight = height();
}

void Amarok::Slider::paletteChange(const QPalette & oldPalette)
{
    Q_UNUSED( oldPalette );
    The::svgHandler()->reTint();
    repaint( 0, 0, -1, -1 );
}



#include "SliderWidget.moc"
