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

#include "amarok.h"
#include "amarokconfig.h"
#include "enginebase.h"
#include "enginecontroller.h"
#include "equalizergraph.h"
#include "equalizersetup.h"
#include "sliderwidget.h"

#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qstringlist.h>
#include <qvbox.h>

#include <kapplication.h>
#include <kdebug.h>
#include <klocale.h>


EqualizerSetup* EqualizerSetup::s_instance = 0;


EqualizerSetup::EqualizerSetup()
        : QVBox( amaroK::mainWindow(), 0, Qt::WType_Dialog | Qt::WDestructiveClose )
{
    using amaroK::Slider;

    s_instance = this;

    setMargin( 8 );
    setSpacing( 8 );

    // BEGIN Equalizer Graph Widget
    m_equalizerGraph = new EqualizerGraph( new QHBox( this ) );
    //END

    // BEGIN GroupBox
    QGroupBox* groupBox_sliders = new QGroupBox( 11, Qt::Horizontal, i18n("Enable Equalizer"), this );
    groupBox_sliders->setCheckable( true );
    groupBox_sliders->setChecked( AmarokConfig::equalizerEnabled() );
    groupBox_sliders->setInsideMargin( 8 );
    connect( groupBox_sliders, SIGNAL( toggled( bool ) ), SLOT( setEqualizerEnabled( bool ) ) );
    // END GroupBox

    // BEGIN Preamp slider
    QVBox* box = new QVBox( groupBox_sliders );
    m_slider_preamp = new Slider( Qt::Vertical, box, 100 );
    m_slider_preamp->setMinValue( -100 );
    m_slider_preamp->setValue( AmarokConfig::equalizerPreamp() );
    connect( m_slider_preamp, SIGNAL( valueChanged( int ) ), SLOT( setEqualizerParameters() ) );
    new QLabel( i18n("Pre-amp"), box );
    // END

    // BEGIN Band Sliders
    const char *bandLabels[] = { "60", "170", "310", "600", "1k", "3k", "6k", "12k", "14k", "16k" };

    int minWidth = 0;
    QFontMetrics fm = fontMetrics(); //apparently it's an expensive call
    for ( int i = 0; i < 10; i++ ) {
         int w = fm.width( bandLabels[i] );
         if ( w > minWidth )
             minWidth = w;
    }

    for ( int i = 0; i < 10; i++ ) {
        QVBox  *box    = new QVBox( groupBox_sliders );
        Slider *slider = new Slider( Qt::Vertical, box );
                         new QLabel( bandLabels[i], box );

        slider->setMinValue( -100 );
        slider->setMaxValue( +100 );
        slider->setValue( AmarokConfig::equalizerGains()[i] );
        slider->setMinimumWidth( minWidth );
        m_bandSliders.append( slider );

        connect( slider, SIGNAL( valueChanged( int ) ), SLOT( setEqualizerParameters() ) );
    }
    // END

    kapp->setTopWidget( this );
    setCaption( kapp->makeStdCaption( i18n( "Equalizer" ) ) );
    show();
}


EqualizerSetup::~EqualizerSetup()
{
    s_instance = 0;
}


/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC SLOTS
/////////////////////////////////////////////////////////////////////////////////////

void
EqualizerSetup::setEqualizerEnabled( bool active ) //SLOT
{
    EngineController::engine()->setEqualizerEnabled( active );
    AmarokConfig::setEqualizerEnabled( active );

    if( active )
       //this way the developer of the eq doesn't have to cache the eq values
       setEqualizerParameters();
}


void
EqualizerSetup::setEqualizerParameters() //SLOT
{
    QValueList<int> gains;
    for ( uint i = 0; i < m_bandSliders.count(); i++ )
        gains += m_bandSliders.at( i )->value();

    AmarokConfig::setEqualizerPreamp( m_slider_preamp->value() );
    AmarokConfig::setEqualizerGains( gains );

    EngineController::engine()->setEqualizerParameters( m_slider_preamp->value(), gains );

    m_equalizerGraph->update();
}

#include "equalizersetup.moc"
