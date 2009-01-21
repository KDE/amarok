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
#include "BookmarkTriangle.h"
#include "Debug.h"
#include "EngineController.h"
#include "SvgHandler.h"
#include "SvgTinter.h"

#include <KIcon>
#include <KLocale>
#include <KStandardDirs>
#include <KGlobalSettings>

#include <QAction>
#include <QContextMenuEvent>
#include <QFontMetrics>
#include <QMenu>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionComplex>


Amarok::Slider::Slider( Qt::Orientation orientation, QWidget *parent, uint max )
    : QSlider( orientation, parent )
    , m_sliding( false )
    , m_outside( false )
    , m_prevValue( 0 )
    , m_needsResize( true )
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

    const QString prefix = "slider_bg_";

    if( m_needsResize )
    {
        m_topLeft = The::svgHandler()->renderSvg( prefix + "topleft", m_borderWidth, m_borderHeight, prefix + "topleft" );
        m_top = The::svgHandler()->renderSvg( prefix + "top", width - ( 2 * m_borderWidth ), m_borderHeight, prefix + "top" );
        m_topRight = The::svgHandler()->renderSvg( prefix + "topright", m_borderWidth, m_borderHeight, prefix + "topright" );
        m_right = The::svgHandler()->renderSvg( prefix + "right", m_borderWidth, height - ( 2 * m_borderHeight ), prefix + "right" );
        m_bottomRight = The::svgHandler()->renderSvg( prefix + "bottomright", m_borderWidth, m_borderHeight, prefix + "bottomright" );
        m_bottom = The::svgHandler()->renderSvg( prefix + "bottom", width - 2 * m_borderWidth, m_borderHeight, prefix + "bottom" );
        m_bottomLeft = The::svgHandler()->renderSvg( prefix + "bottomleft", m_borderWidth, m_borderHeight, prefix + "bottomleft" );
        m_left = The::svgHandler()->renderSvg( prefix + "left", m_borderWidth, height - 2 * m_borderHeight, prefix + "left" );
        m_needsResize = false;
    }
    p->drawPixmap( x, y, m_topLeft );
    p->drawPixmap( x + m_borderWidth, y, m_top );
    p->drawPixmap( x + ( width - m_borderWidth ), y, m_topRight );
    p->drawPixmap( x + ( width - m_borderWidth ), y + m_borderHeight, m_right );
    p->drawPixmap( x + ( width - m_borderWidth ), y + ( height - m_borderHeight ), m_bottomRight );
    p->drawPixmap( x + m_borderWidth, y + ( height - m_borderHeight ), m_bottom );
    p->drawPixmap( x, y + ( height - m_borderHeight ) , m_bottomLeft );
    p->drawPixmap( x, y + m_borderHeight, m_left );

    if( value() != minimum() )
    {
        const int sliderHeight = height - ( m_sliderInsertY * 2 );
        const int sliderLeftWidth = sliderHeight / 3;
        const int sliderRightWidth = sliderLeftWidth;

        int knobX = ( ( ( double ) value() - ( double ) minimum() ) / ( maximum() - minimum() ) ) * ( width - ( sliderLeftWidth + sliderRightWidth + m_sliderInsertX * 2 ) );

        p->drawPixmap( x + m_sliderInsertX, y + m_sliderInsertY, The::svgHandler()->renderSvg( "slider_bar_left",sliderLeftWidth , sliderHeight, "slider_bar_left" ) );
        p->drawPixmap( x + m_sliderInsertX + sliderLeftWidth, y + m_sliderInsertY, The::svgHandler()->renderSvg( "slider_bar_center", knobX, sliderHeight, "slider_bar_center" ) );
        p->drawPixmap( x + m_sliderInsertX + knobX + sliderLeftWidth, y + m_sliderInsertY, The::svgHandler()->renderSvg( "slider_bar_right", sliderRightWidth, sliderHeight, "slider_bar_right" ) );
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

    QFontMetrics fm( KGlobalSettings::fixedFont() );
    m_textHeight = fm.height();
    
}

void
Amarok::VolumeSlider::mousePressEvent( QMouseEvent *e )
{
    const QRect iconBox( 0, 0, m_iconHeight, m_iconWidth );
    if( iconBox.contains( e->pos() ) && e->button() != Qt::RightButton )
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
        p->setFont( KGlobalSettings::fixedFont() );

        p->setRenderHint( QPainter::TextAntialiasing, true );


        const int yOffset =  sliderY + ( m_sliderHeight - m_textHeight ) / 2;

        const QRect rect( m_iconWidth + m_sliderWidth + 4, yOffset, 40, m_textHeight );
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
    Amarok::Slider::resizeEvent( event );
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
    , m_triangles()
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
    Amarok::Slider::resizeEvent( event );
    //m_sliderHeight = (int)width() / 25; //maintain sane aspect ratio
    m_sliderHeight = 20;
    if ( m_sliderHeight > height() )
        m_sliderHeight = height();
}

void Amarok::TimeSlider::drawTriangle( int seconds )
{
    DEBUG_BLOCK
    int ms = seconds * 1000;
//     int x_pos = QStyle::sliderPositionFromValue( minimum(), maximum(), ms, width() );
    int sliderHeight = height() - ( m_sliderInsertY * 2 );
    int sliderLeftWidth = sliderHeight / 3;
    int x_pos = ( ( ( double ) ms - ( double ) minimum() ) / ( maximum() - minimum() ) ) * ( width() - ( sliderLeftWidth + sliderLeftWidth + m_sliderInsertX * 2 ) );
    debug() << "drawing triangle at " << x_pos;
    BookmarkTriangle * tri = new BookmarkTriangle( this );
    m_triangles << tri;
    tri->setGeometry(x_pos + 6 /* to center the point */, 5 /*y*/, 11, 11 ); // 6 = hard coded border width
    tri->show();
}

void Amarok::TimeSlider::clearTriangles()
{
    qDeleteAll( m_triangles );
    m_triangles.clear();
}

void Amarok::Slider::paletteChange(const QPalette & oldPalette)
{
    Q_UNUSED( oldPalette );
    The::svgHandler()->reTint();
    repaint( 0, 0, -1, -1 );
}


#include "SliderWidget.moc"

