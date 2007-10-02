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

#include "config-amarok.h"

#include "amarok.h"
#include "amarokconfig.h"
#include "app.h"
#include "debug.h"
#include "enginecontroller.h"
#include "sliderwidget.h"

#include <QBitmap>
#include <QBrush>
#include <QContextMenuEvent>
#include <QImage>
#include <QMenu>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionComplex>
#include <QTimer>

#include <kicon.h>
#include <klocale.h>
#include <kmenu.h>
#include <kstandarddirs.h>

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
    if( orientation() == Qt::Vertical ) {
        // Will be handled by the parent widget
        e->ignore();
        return;
    }

    // Position Slider (horizontal)
    int step = e->delta() * 1500 / 18;
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

        if ( orientation() == Qt::Horizontal && !rect.contains( e->pos() ) ) {
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

//////////////////////////////////////////////////////////////////////////////////////////
/// CLASS VolumeSlider
//////////////////////////////////////////////////////////////////////////////////////////

Amarok::VolumeSlider::VolumeSlider( QWidget *parent, uint max )
    : Amarok::Slider( Qt::Horizontal, parent, max )
    , m_animCount( 0 )
    , m_animTimer( new QTimer( this ) )
    , m_pixmapInset( QPixmap( KStandardDirs::locate( "data","amarok/images/volumeslider-inset.png" ) ) )
{
    setFocusPolicy( Qt::NoFocus );

    // BEGIN Calculate handle animation pixmaps for mouse-over effect
    QImage imgHandle    ( KStandardDirs::locate( "data","amarok/images/volumeslider-handle.png" ) );
    QImage imgHandleGlow( KStandardDirs::locate( "data","amarok/images/volumeslider-handle_glow.png" ) );

    float opacity = 0.0;
    const float step = 1.0 / ANIM_MAX;
    QImage dst;
    QColor color = Qt::black;
    for ( int i = 0; i < ANIM_MAX; ++i ) {
        dst = imgHandle;
        QPainter p( &dst );
        p.setCompositionMode( QPainter::CompositionMode_DestinationOut );
        color.setAlphaF( opacity );
        p.fillRect( dst.rect(), color );
        p.setCompositionMode( QPainter::CompositionMode_Plus );
        p.setOpacity( opacity );
        p.drawImage( 0, 0, imgHandleGlow );
        p.end();
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
Amarok::VolumeSlider::generateGradient()
{
    //QImage temp( KStandardDirs::locate( "data","amarok/images/volumeslider-gradient.png" ) );
    //KIconEffect::colorize( temp, colorGroup().highlight(), 1.0 );

    const QPixmap temp( KStandardDirs::locate( "data","amarok/images/volumeslider-gradient.png" ) );
    const QBitmap mask( temp.createHeuristicMask() );

    m_pixmapGradient = QPixmap( m_pixmapInset.size() );
    QPainter p( &m_pixmapGradient );
    QLinearGradient gradient( 0, 0, m_pixmapGradient.width(), 0 );
    gradient.setColorAt( 0.0, colorGroup().alternateBase() );
    gradient.setColorAt( 1.0, colorGroup().highlight() );
    p.setPen( Qt::NoPen );
    p.setBrush( gradient );
    p.drawRect( m_pixmapGradient.rect() );
    p.end();
    m_pixmapGradient.setMask( mask );
}

void
Amarok::VolumeSlider::slotAnimTimer() //SLOT
{
    if ( m_animEnter ) {
        m_animCount++;
        repaint( );
        if ( m_animCount == ANIM_MAX - 1 )
            m_animTimer->stop();
    } else {
        m_animCount--;
        repaint();
        if ( m_animCount == 0 )
            m_animTimer->stop();
    }
}

void
Amarok::VolumeSlider::mousePressEvent( QMouseEvent *e )
{
    if( e->button() != Qt::RightButton ) {
        Amarok::Slider::mousePressEvent( e );
        slideEvent( e );
    }
}

void
Amarok::VolumeSlider::contextMenuEvent( QContextMenuEvent *e )
{
    QMenu menu;
    menu.setTitle( i18n( "Volume" ) );
    menu.addAction(  i18n(   "100%" ) )->setData( 100 );
    menu.addAction(  i18n(    "80%" ) )->setData(  80 );
    menu.addAction(  i18n(    "60%" ) )->setData(  60 );
    menu.addAction(  i18n(    "40%" ) )->setData(  40 );
    menu.addAction(  i18n(    "20%" ) )->setData(  20 );
    menu.addAction(  i18n(     "0%" ) )->setData(   0 );

    if( EngineController::hasEngineProperty( "HasEqualizer" ) )
    {
        menu.addSeparator();
        menu.addAction( KIcon( "equalizer" ), i18n( "&Equalizer" ), kapp, SLOT( slotConfigEqualizer() ) )
            ->setData( -1 );
    }

    QAction* a = menu.exec( mapToGlobal( e->pos() ) );
    if( a ) {
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
    QSlider::setValue( QStyle::sliderValueFromPosition( minimum(), maximum(), e->pos().x(), width()-2 ) );
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
    QPainter p( this );

    const int padding = 7;
    const int offset = int( double( ( width() - 2 * padding ) * value() ) / maximum() );

    const QRectF boundsG( 0, 0, offset + padding, m_pixmapGradient.height() );
    p.drawPixmap( boundsG, m_pixmapGradient, boundsG );

    const QRectF boundsI( 0, 0, m_pixmapInset.width(), m_pixmapInset.height() );
    p.drawPixmap( boundsI, m_pixmapInset, boundsI );

    const QRectF targetBounds( offset - m_handlePixmaps[0].width() / 2 + padding, 0, m_handlePixmaps[m_animCount].width(), m_handlePixmaps[m_animCount].height() );
    const QRectF srcBounds( 0, 0, m_handlePixmaps[m_animCount].width(), m_handlePixmaps[m_animCount].height() );
    p.drawPixmap( targetBounds, m_handlePixmaps[m_animCount], srcBounds );

    // Draw percentage number
    p.setPen( palette().color( QPalette::Disabled, QColorGroup::Text ).dark() );
    QFont font;
    font.setPixelSize( 9 );
    p.setFont( font );
    const QRect rect( 0, 0, 34, 15 );
    p.drawText( rect, Qt::AlignRight | Qt::AlignVCenter, QString::number( value() ) + '%' );
}

void
Amarok::VolumeSlider::enterEvent( QEvent* )
{
    m_animEnter = true;
    m_animCount = 0;

    m_animTimer->start( ANIM_INTERVAL );
}

void
Amarok::VolumeSlider::leaveEvent( QEvent* )
{
    // This can happen if you enter and leave the widget quickly
    if ( m_animCount == 0 )
        m_animCount = 1;

    m_animEnter = false;
    m_animTimer->start( ANIM_INTERVAL );
}

void
Amarok::VolumeSlider::paletteChange( const QPalette& )
{
    generateGradient();
}

#include "sliderwidget.moc"
