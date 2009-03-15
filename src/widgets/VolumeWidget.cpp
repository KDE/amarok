/***************************************************************************
 * copyright     : (C) 2004 Mark Kretschmann <markey@web.de>               *
                   (C) 2007 Dan Meltzer <parallelgrapefruit@gmail.com>   *
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
    const KIcon volumeIcon( KStandardDirs::locate( "data", "amarok/images/volume_icon.png" ) );
    m_button = new KAction( volumeIcon, i18n( "Mute" ), this );

    m_slider = new Amarok::VolumeSlider( this, Amarok::VOLUME_MAX );
    m_slider->setObjectName( "ToolBarVolume" );
    m_slider->setValue( AmarokConfig::masterVolume() );
    //m_slider->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum );
    m_slider->setToolTip( i18n( "Volume Control" ) );

    m_label = new QLabel();
    m_label->setFixedWidth( 40 ); // HACK to align correctly with progress slider
    m_label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
    m_label->setFont( KGlobalSettings::fixedFont() );

    addAction( m_button );
    addWidget( m_slider );
    addWidget( m_label );

    EngineController* const ec = The::engineController();
    connect( m_slider, SIGNAL( sliderMoved( int )    ), ec, SLOT( setVolume( int ) ) );
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

#include "VolumeWidget.moc"
