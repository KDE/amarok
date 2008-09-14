/***************************************************************************
                        amarokslider.cpp  -  description
                           -------------------
  begin                : Dec 15 2003
  copyright            : (C) 2003-2008 by Mark Kretschmann
  email                : kretschmann@kde.org 
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

#include <QContextMenuEvent>
#include <QFontMetrics>
#include <QPainter>
#include <QPixmapCache>
#include <QStyle>
#include <QStyleOptionComplex>
#include <QSvgRenderer>

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


void Amarok::Slider::paintCustomSlider( QPainter *p, int x, int y, int width, int height, double /*pos*/ )
{
    const int borderWidth = 6;
    const int borderHeight = 6;

    const int sliderInsertX = 5;
    const int sliderInsertY = 5;
    
    QString prefix = "slider_bg_";

    QImage topLeft = The::svgHandler()->renderSvg( prefix + "topleft", borderWidth, borderHeight, prefix + "topleft" ).toImage();
    p->drawImage( x, y, topLeft );

    QImage top = The::svgHandler()->renderSvg( prefix + "top", width - ( 2 * borderWidth ), borderHeight, prefix + "top" ).toImage();
    p->drawImage( x + borderWidth, y, top );

    QImage topRight = The::svgHandler()->renderSvg( prefix + "topright", borderWidth, borderHeight, prefix + "topright" ).toImage();
    p->drawImage( x + ( width - borderWidth ), y, topRight );

    QImage right = The::svgHandler()->renderSvg( prefix + "right", borderWidth, height - ( 2 * borderHeight ), prefix + "right" ).toImage();
    p->drawImage( x + ( width - borderWidth ), y + borderHeight, right );

    QImage bottomRight = The::svgHandler()->renderSvg( prefix + "bottomright", borderWidth, borderHeight, prefix + "bottomright" ).toImage();
    p->drawImage( x + ( width - borderWidth ), y + ( height - borderHeight ), bottomRight );

    QImage bottom = The::svgHandler()->renderSvg( prefix + "bottom", width - 2 * borderWidth, borderHeight, prefix + "bottom" ).toImage();
    p->drawImage( x + borderWidth, y + ( height - borderHeight ), bottom );

    QImage bottomLeft = The::svgHandler()->renderSvg( prefix + "bottomleft", borderWidth, borderHeight, prefix + "bottomleft" ).toImage();
    p->drawImage( x, y + ( height - borderHeight ) , bottomLeft );

    QImage left = The::svgHandler()->renderSvg( prefix + "left", borderWidth, height - 2 * borderHeight, prefix + "left" ).toImage();
    p->drawImage( x, y + borderHeight, left );

    if ( value() != minimum() )
    {
        const int sliderHeight = height - ( sliderInsertY * 2 );
        const int sliderLeftWidth = sliderHeight / 3;
        const int sliderRigthWidth = sliderLeftWidth;

        int knobX = ( ( ( double ) value() - ( double ) minimum() ) / ( maximum() - minimum() ) ) * ( width - ( sliderLeftWidth + sliderRigthWidth + sliderInsertX * 2 ) );

        p->drawPixmap( x + sliderInsertX, y + sliderInsertY, The::svgHandler()->renderSvg( "slider_bar_left",sliderLeftWidth , sliderHeight, "slider_bar_left" ) );
        p->drawPixmap( x + sliderInsertX + sliderLeftWidth, y + sliderInsertY, The::svgHandler()->renderSvg( "slider_bar_center", knobX, sliderHeight, "slider_bar_center" ) );
        p->drawPixmap( x + sliderInsertX + knobX + sliderLeftWidth, y + sliderInsertY, The::svgHandler()->renderSvg( "slider_bar_right", sliderRigthWidth, sliderHeight, "slider_bar_right" ) );
    }
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
    const QRect iconBox( 0, 0, m_iconHeight, m_iconWidth );
    if( iconBox.contains( e->pos() ) )
    {
        emit mute();
        return;
    }
    
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
    const int x = e->pos().x();

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

    const int sliderY =  ( m_iconHeight -m_sliderHeight ) / 2;

    paintCustomSlider( p, m_sliderX, sliderY, m_sliderWidth, m_sliderHeight );
    
    //Using pre-rendered SVG for now, cause QtSvg renders the icon wrong
    //p->drawPixmap( 0, 0, The::svgHandler()->renderSvg( "volume_icon", m_iconWidth, m_iconHeight, "volume_icon" ) ) ;
    
    const QImage volumeIcon( KStandardDirs::locate( "data", "amarok/images/volume_icon.png" ) );
    p->drawImage( 0, 0, volumeIcon.scaled( m_iconWidth, m_iconHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation ) );

    if( underMouse() )
    {
        // Draw percentage number
        p->setPen( palette().color( QPalette::Active, QPalette::WindowText ) );
        //QFont font;
        //font.setPixelSize( 12 );
        //p->setFont( font );

        QFontMetrics fm( font() );
        const int pixelsHigh = fm.height();

        const int yOffset =  sliderY + ( m_sliderHeight - pixelsHigh ) / 2;

        const QRect rect( m_iconWidth + m_sliderWidth + 4, yOffset, 40, pixelsHigh );
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
    m_sliderHeight = 20;
    if ( m_sliderHeight > height() )
        m_sliderHeight = height();

    m_sliderX = m_iconWidth + m_margin;
}



//////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// TIMESLIDER ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

Amarok::TimeSlider::TimeSlider( QWidget *parent )
    : Amarok::Slider( Qt::Horizontal, parent )
    , m_knobX( 0.0 )
{
    setFocusPolicy( Qt::NoFocus );
    m_sliderHeight = 20;
}

void
Amarok::TimeSlider::setSliderValue( int value )
{
    Amarok::Slider::setValue( value );
    m_knobX = QStyle::sliderPositionFromValue( minimum(), maximum(), value, width() );
    update();
}

void
Amarok::TimeSlider::paintEvent( QPaintEvent * )
{
    QPainter p( this );
    paintCustomSlider( &p, 0, ( height() - m_sliderHeight ) / 2, width(), m_sliderHeight, m_knobX );
}

void
Amarok::TimeSlider::paletteChange( const QPalette& )
{
    The::svgHandler()->reTint();
}

void Amarok::TimeSlider::resizeEvent(QResizeEvent * event)
{
    Q_UNUSED( event );
    //m_sliderHeight = (int)width() / 25; //maintain sane aspect ratio
    m_sliderHeight = 20;
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

