/***************************************************************************
 Setup dialog for equalizer

 (c) 2004 Mark Kretschmann <markey@web.de>
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "enginebase.h"
#include "enginecontroller.h"
#include "equalizersetup.h"

#include <vector>

#include <qcheckbox.h>
#include <qlayout.h>

#include <kapplication.h>
#include <klocale.h>

EqualizerSetup* EqualizerSetup::s_instance = 0;


EqualizerSetup::EqualizerSetup()
    : QWidget( 0, 0, Qt::WType_TopLevel | Qt::WDestructiveClose )
{
    s_instance = this;
    setCaption( kapp->makeStdCaption( i18n( "Equalizer Setup" ) ) );

    QBoxLayout* layout = new QHBoxLayout( this );

    QCheckBox* checkBox_activate = new QCheckBox( this );
    connect( checkBox_activate, SIGNAL( toggled( bool ) ), SLOT( activateEqualizer( bool ) ) );
    layout->addWidget( checkBox_activate );

    for ( int i = 0; i < NUM_BANDS; i++ ) {
        QSlider* slider = new QSlider( Qt::Vertical, this );
        m_sliders.append( slider );
        slider->setTracking( true );
        slider->setMinValue( 0 );
        slider->setMaxValue( 100 );
        connect( slider, SIGNAL( valueChanged( int ) ), SLOT( sliderChanged( int ) ) );
        layout->addWidget( slider );
    }
}


EqualizerSetup::~EqualizerSetup()
{
    s_instance = 0;
}


/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC SLOTS
/////////////////////////////////////////////////////////////////////////////////////

void
EqualizerSetup::activateEqualizer( bool active ) //SLOT
{
    EngineController::engine()->setEqualizerActive( active );
}


void
EqualizerSetup::sliderChanged( int ) //SLOT
{
    std::vector<float> gains( NUM_BANDS );

    for ( int i = 0; i < NUM_BANDS; i++ )
        gains[i] = static_cast<float>( m_sliders.at( i )->value() ) * 0.01;

    EngineController::engine()->setEqualizerGains( &gains );
}


#include "equalizersetup.moc"

