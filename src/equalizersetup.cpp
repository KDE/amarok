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
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qstringlist.h>
#include <qvbox.h>

#include <kapplication.h>
#include <klocale.h>


EqualizerSetup* EqualizerSetup::s_instance = 0;


EqualizerSetup::EqualizerSetup()
{
    s_instance = this;
    setCaption( kapp->makeStdCaption( i18n( "Equalizer Settings" ) ) );
    setWFlags( Qt::WType_TopLevel | Qt::WDestructiveClose );

    QBoxLayout* vmainlayout = new QVBoxLayout( this );

    // BEGIN CheckBox Enable Equalizer
    m_checkBox_enableEqualizer = new QCheckBox( this );
    m_checkBox_enableEqualizer->setText( i18n( "Enable Equalizer" ) );
    vmainlayout->addWidget( m_checkBox_enableEqualizer );
    //END

    QGroupBox* groupBox_sliders = new QGroupBox( 11, Qt::Horizontal, this );
    groupBox_sliders->setEnabled( false );
    vmainlayout->addWidget( groupBox_sliders );
    connect( m_checkBox_enableEqualizer, SIGNAL( toggled( bool ) ), SLOT( activateEqualizer( bool ) ) );
    connect( m_checkBox_enableEqualizer, SIGNAL( toggled( bool ) ), groupBox_sliders, SLOT( setEnabled( bool ) ) );
    m_checkBox_enableEqualizer->setChecked( AmarokConfig::useEqualizer() );

    // BEGIN Preamp slider
    QVBox* vBox_preamp = new QVBox( groupBox_sliders );
    m_slider_preamp = new Slider( Qt::Vertical, vBox_preamp, 100 );
    m_slider_preamp->setValue( AmarokConfig::equalizerPreamp() );
    connect( m_slider_preamp, SIGNAL( valueChanged( int ) ), SLOT( preampChanged() ) );
    QLabel* label = new QLabel( i18n( "Pre-Amp" ), vBox_preamp );
    // END

    // BEGIN Band Sliders
    QStringList bandLabels;
    bandLabels << "60" << "170" << "310" << "600" << "1k" << "3k" << "6k"
               << "12k" << "14k" << "16k";

    for ( int i = 0; i < 10; i++ ) {
        QVBox* vBox = new QVBox( groupBox_sliders );
        Slider* slider = new Slider( Qt::Vertical, vBox, 100 );
        m_bandSliders.append( slider );
        slider->setValue( *AmarokConfig::equalizerGains().at( i ) );
        connect( slider, SIGNAL( valueChanged( int ) ), SLOT( bandChanged() ) );
        QLabel* label = new QLabel( bandLabels[i], vBox );
    }
    // END
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
    EngineController::engine()->setEqualizerPreamp( m_slider_preamp->value() );
    AmarokConfig::setEqualizerPreamp( m_slider_preamp->value() );
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

