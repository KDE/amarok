/***************************************************************************
 * copyright     : (C) 2004 Mark Kretschmann <markey@web.de>               *
                   (C) 2007 Dan Meltzer <parallelgrapefruit@gmail.com>     *
                   (C) 2009 Kevin Funk <krf@electrostorm.net               *
 **************************************************************************/

 /***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "VolumeWidget.h"

#include "Amarok.h"
#include "amarokconfig.h"
#include "EngineController.h"
#include "SliderWidget.h"

#include <KLocale>
#include <KGlobalSettings>
#include <KStandardDirs>

#include <QHBoxLayout>
#include <QSlider>

VolumeWidget::VolumeWidget( QWidget *parent )
    : Amarok::ToolBar( parent )
    , EngineObserver( The::engineController() )
{
    m_icons << KStandardDirs::locate( "data", "amarok/images/volume_icon.png" );
    m_icons << KStandardDirs::locate( "data", "amarok/images/volume_muted_icon.png" );
    m_action = new KAction( KIcon( m_icons[ AmarokConfig::muteState() ] ), i18n( "Mute" ), this );

    m_slider = new Amarok::VolumeSlider( Amarok::VOLUME_MAX, this );
    m_slider->setObjectName( "ToolBarVolume" );
    m_slider->setValue( AmarokConfig::masterVolume() );
    m_slider->setToolTip( i18n( "Volume Control" ) );

    m_label = new QLabel( QString::number( AmarokConfig::masterVolume() ) + '%' );
    m_label->setFixedWidth( 58 ); // HACK to align correctly with progress slider
    m_label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
    m_label->setFont( KGlobalSettings::fixedFont() );

    addAction( m_action );
    addWidget( m_slider );
    addWidget( m_label );

    EngineController* const ec = The::engineController();
    connect( m_action, SIGNAL( triggered( bool ) ), ec, SLOT( toggleMute() ) );
    connect( m_slider, SIGNAL( sliderMoved( int ) ), ec, SLOT( setVolume( int ) ) );
    connect( m_slider, SIGNAL( sliderReleased( int ) ), ec, SLOT( setVolume( int ) ) );

    connect( ec, SIGNAL( volumeChanged( int ) ), this, SLOT( engineVolumeChanged( int ) ) );
}

void
VolumeWidget::engineVolumeChanged( int value )
{
    if( value != m_slider->value() )
        m_slider->setValue( value );
    m_label->setText( QString::number( value ) + '%' );
}

void
VolumeWidget::engineMuteStateChanged( bool mute )
{
    m_action->setIcon( KIcon( m_icons[ (bool)mute ] ) );
}

#include "VolumeWidget.moc"
