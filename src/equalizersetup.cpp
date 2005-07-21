/***************************************************************************
 Setup dialog for equalizer

 (c) 2004 Mark Kretschmann <markey@web.de>
 (c) 2005 Seb Ruiz <me@sebruiz.net>
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
#include <qdom.h>
#include <qgroupbox.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qstringlist.h>
#include <qtextstream.h>   //presets
#include <qvbox.h>

#include <kapplication.h>
#include <kdebug.h>
#include <klocale.h>
#include <kpopupmenu.h>
#include <kstandarddirs.h> //locate()
#include <ktoolbar.h>      //presets
#include <kwin.h>

EqualizerSetup* EqualizerSetup::s_instance = 0;


EqualizerSetup::EqualizerSetup()
        : QVBox( amaroK::mainWindow(), 0, Qt::WType_Dialog | Qt::WDestructiveClose )
{
    using amaroK::Slider;

    // Gives the window a small title bar, and skips a taskbar entry
    KWin::setType( winId(), NET::Utility );
    KWin::setState( winId(), NET::SkipTaskbar );

    s_instance = this;

    setMargin( 8 );
    setSpacing( 8 );

    QHBox *header = new QHBox( this );
    // BEGIN Equalizer Graph Widget
    m_equalizerGraph = new EqualizerGraph( header );
    // END Graph Widget

    // BEGIN Presets
    m_equalizerPresets = new KPopupMenu( header );
    loadPresets();
    connect( m_equalizerPresets, SIGNAL( activated(int) ), SLOT( presetChanged(int) ) );

    KToolBar* toolBar = new KToolBar( header );
    toolBar->setIconText( KToolBar::IconTextRight );
    toolBar->setIconSize( 16 );
    toolBar->setBarPos( KToolBar::Right );
    toolBar->setFrameShape( QFrame::NoFrame );
    toolBar->insertButton( "configure", 0, m_equalizerPresets, true, i18n( "Presets" ) );
    // END Presets

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

void
EqualizerSetup::updateSliders(int preamp, QValueList<int> gains)
{
    m_slider_preamp->setValue(preamp);
    for ( uint i = 0; i < m_bandSliders.count(); i++ )
    {
        m_bandSliders.at(i)->setValue((*gains.at(i)));
    }
    m_equalizerGraph->update();
}

void
EqualizerSetup::loadPresets()
{
    QFile file( locate( "data","amarok/data/equalizer_presets.xml" ) );
    QTextStream stream( &file );
    stream.setEncoding( QTextStream::UnicodeUTF8 );

    QDomDocument d;

    if( !file.open( IO_ReadOnly ) || !d.setContent( stream.read() ) )
        return;

    QDomNode n = d.namedItem( "equalizerpresets" ).namedItem("preset");

    for( ; !n.isNull();  n = n.nextSibling(), m_totalPresets++ )
    {
        QDomElement e = n.toElement();
        QString title = e.attribute( "name" );

        QValueList<int> gains;
        gains << e.namedItem( "preamp" ).toElement().text().toInt();
        gains << e.namedItem( "b0" ).toElement().text().toInt();
        gains << e.namedItem( "b1" ).toElement().text().toInt();
        gains << e.namedItem( "b2" ).toElement().text().toInt();
        gains << e.namedItem( "b3" ).toElement().text().toInt();
        gains << e.namedItem( "b4" ).toElement().text().toInt();
        gains << e.namedItem( "b5" ).toElement().text().toInt();
        gains << e.namedItem( "b6" ).toElement().text().toInt();
        gains << e.namedItem( "b7" ).toElement().text().toInt();
        gains << e.namedItem( "b8" ).toElement().text().toInt();
        gains << e.namedItem( "b9" ).toElement().text().toInt();

        m_presets[ m_totalPresets ] = gains;
        m_equalizerPresets->insertItem( title, m_totalPresets );
    }
}


/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC SLOTS
/////////////////////////////////////////////////////////////////////////////////////

void
EqualizerSetup::presetChanged( int id ) //SLOT
{
    QValueList<int> gains = m_presets[ id ];
    int preamp = gains.first();
    gains.pop_front();

    updateSliders( preamp, gains );
}

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
