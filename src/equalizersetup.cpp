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

#include "amarokconfig.h"
#include "enginebase.h"
#include "enginecontroller.h"
#include "equalizersetup.h"

#include <qcheckbox.h>
#include <qlayout.h>

#include <kapplication.h>
#include <klocale.h>

EqualizerSetup* EqualizerSetup::s_instance = 0;


EqualizerSetup::EqualizerSetup()
{
    s_instance = this;
    setCaption( kapp->makeStdCaption( i18n( "Equalizer Settings" ) ) );
    setWFlags( Qt::WType_TopLevel | Qt::WDestructiveClose );

    connect( checkBox_enableEqualizer, SIGNAL( toggled( bool ) ), SLOT( activateEqualizer( bool ) ) );
    checkBox_enableEqualizer->setChecked( AmarokConfig::useEqualizer() );

    slider_preamp->setValue( AmarokConfig::equalizerPreamp() );
    connect( slider_preamp, SIGNAL( valueChanged( int ) ), SLOT( preampChanged() ) );

    m_bandSliders.append( slider_band1 );
    m_bandSliders.append( slider_band2 );
    m_bandSliders.append( slider_band3 );
    m_bandSliders.append( slider_band4 );
    m_bandSliders.append( slider_band5 );
    m_bandSliders.append( slider_band6 );
    m_bandSliders.append( slider_band7 );
    m_bandSliders.append( slider_band8 );
    m_bandSliders.append( slider_band9 );
    m_bandSliders.append( slider_band10 );

    for ( int i = 0; i < m_bandSliders.count(); i++ ) {
        QSlider* slider = m_bandSliders.at( i );
        slider->setValue( *AmarokConfig::equalizerGains().at( i ) );
        connect( slider, SIGNAL( valueChanged( int ) ), SLOT( bandChanged() ) );
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
    AmarokConfig::setUseEqualizer( active );
}


void
EqualizerSetup::preampChanged() //SLOT
{
    EngineController::engine()->setEqualizerPreamp( slider_preamp->value() );
    AmarokConfig::setEqualizerPreamp( slider_preamp->value() );
}


void
EqualizerSetup::bandChanged() //SLOT
{
    QValueList<int> gains;

    for ( int i = 0; i < m_bandSliders.count(); i++ )
        gains.push_back( m_bandSliders.at( i )->value() );

    EngineController::engine()->setEqualizerGains( gains );
    AmarokConfig::setEqualizerGains( gains );
}


#include "equalizersetup.moc"

