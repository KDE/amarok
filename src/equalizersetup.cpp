/***************************************************************************
 Setup dialog for the equalizer

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
#include "debug.h"
#include "enginebase.h"
#include "enginecontroller.h"
#include "equalizergraph.h"
#include "equalizersetup.h"
#include "sliderwidget.h"

#include <qcheckbox.h>
#include <qdom.h>
#include <qfile.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qstringlist.h>
#include <qtextstream.h>   //presets

#include <kapplication.h>
#include <kdebug.h>
#include <kinputdialog.h>  //presets
#include <klocale.h>
#include <kpopupmenu.h>
#include <kstandarddirs.h> //locate()
#include <ktoolbar.h>      //presets
#include <kwin.h>

EqualizerSetup* EqualizerSetup::s_instance = 0;


EqualizerSetup::EqualizerSetup()
        : QVBox( amaroK::mainWindow(), 0, Qt::WType_Dialog | Qt::WDestructiveClose )
        , m_currentPreset( -1 )
        , m_totalPresets( 0 )
{
    using amaroK::Slider;

    // Gives the window a small title bar, and skips a taskbar entry
    KWin::setType( winId(), NET::Utility );
    KWin::setState( winId(), NET::SkipTaskbar );

    s_instance = this;

    setMargin( 8 );
    setSpacing( 8 );

    QWidget* container = new QWidget( this );
    QHBoxLayout* headerLayout = new QHBoxLayout( container );

    // BEGIN Equalizer Graph Widget
    m_equalizerGraph = new EqualizerGraph( container );
    // END Graph Widget

    // BEGIN Presets
    m_presetPopup = new KPopupMenu( container );
    m_presetPopup->setCheckable( true );
    loadPresets();
    connect( m_presetPopup, SIGNAL( activated(int) ), SLOT( presetChanged(int) ) );

    KToolBar* toolBar = new KToolBar( container );
    toolBar->setIconText( KToolBar::IconTextRight );
    toolBar->setIconSize( 16 );
    toolBar->setFrameShape( QFrame::NoFrame );
    toolBar->insertButton( "configure", 0, m_presetPopup, true, i18n( "Presets" ) );
    // END Presets

    headerLayout->addWidget( m_equalizerGraph, 0, Qt::AlignLeft );
    headerLayout->addWidget( toolBar, 0, Qt::AlignRight  );

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
        connect( slider, SIGNAL( valueChanged( int ) ), SLOT( sliderChanged() ) );
    }
    // END

    kapp->setTopWidget( this );
    setCaption( kapp->makeStdCaption( i18n( "Equalizer" ) ) );
    show();
}


EqualizerSetup::~EqualizerSetup()
{
//     savePresets();
    s_instance = 0;
}


void
EqualizerSetup::updateSliders( int preamp, QValueList<int> gains )
{
    m_slider_preamp->setValue( preamp );

    for ( uint i = 0; i < m_bandSliders.count(); i++ )
        m_bandSliders.at(i)->setValue( ( *gains.at(i) ) );

    m_equalizerGraph->update();
}

/////////////////////////////////////////////////////////////////////////////////////
// EQUALIZER PRESETS
/////////////////////////////////////////////////////////////////////////////////////

QString
EqualizerSetup::presetsCache() const
{
    //returns the playlists stats cache file
    return amaroK::saveLocation() + "equalizerpresets_save.xml";
}


void
EqualizerSetup::loadPresets()
{
//     m_presetPopup->insertItem( i18n("Save"), 0 );
//     m_presetPopup->insertSeparator();
//     m_totalPresets++;

//     KPopupMenu *m_presetDefaultPopup = new KPopupMenu( this );


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
        m_presetPopup->insertItem( title, m_totalPresets );
    }
//     m_presetPopup->insertItem( "Defaults", m_presetDefaultPopup );

//     connect( m_presetDefaultPopup, SIGNAL( activated(int) ), SLOT( presetChanged(int) ) );
}


void
EqualizerSetup::savePresets()
{
    QFile file( presetsCache() );

    if( !file.open( IO_WriteOnly ) ) return;

    QDomDocument doc;
    QDomElement e;
    e.setAttribute( "name", "equalizerpresets" );
    e.setAttribute( "product", "amaroK" );
    e.setAttribute( "version", APP_VERSION );
    e.setAttribute( "formatversion", "1.1" );

    doc.appendChild( e );

    QValueList<int>               keys   = m_presets.keys();
    QValueList< QValueList<int> > values = m_presets.values();

    for( uint x = 1; x < keys.count(); x++ ) // dont save 'ZERO' or 'SAVE' presets
    {
        const QString title = m_presetPopup->text( x );
        QValueList<int> gains = values[x];

        QDomElement i = doc.createElement("preset");
        i.setAttribute( "name", title );
        debug() << "Title: " << title << endl;

        QStringList info;
        info << "b0" << "b1" << "b2" << "b3" << "b4"
             << "b5" << "b6" << "b7" << "b8" << "b9";

        QDomElement attr;
        QDomText t;
        for( uint y=0; y < info.count(); y++ )
        {
            attr = doc.createElement( info[y] );
            t    = doc.createTextNode( QString::number( gains.first() ) );
            attr.appendChild( t );
            i.appendChild( attr );
            debug() << "\t" << info[y] << ": " << gains.first() << endl;
            gains.pop_front();
        }
        e.appendChild( i );
    }

    QTextStream stream( &file );
    stream.setEncoding( QTextStream::UnicodeUTF8 );
    stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    stream << doc.toString();
}


/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC SLOTS
/////////////////////////////////////////////////////////////////////////////////////

void
EqualizerSetup::presetChanged( int id ) //SLOT
{
    if( id == 0 ) // Saving
    {
        bool ok;
        const QString title = KInputDialog::getText( i18n("Save Equalizer Preset"),
                                                     i18n("Enter preset name:"), i18n("Untitled"), &ok, this);

        if( ok )
        {
            QValueList<int> gains;

            for ( uint i = 0; i < m_bandSliders.count(); i++ )
                gains += m_bandSliders.at( i )->value();

            m_totalPresets++;
            m_presets[ m_totalPresets ] = gains;
            m_presetPopup->insertItem( title, m_totalPresets );
            m_presetPopup->setItemChecked( m_currentPreset, false );
            m_presetPopup->setItemChecked( m_totalPresets, true );
            m_currentPreset = m_totalPresets;
        }
        return;
    }


    const QValueList<int> gains = m_presets[ id ];
    updateSliders( m_slider_preamp->value(), gains );

    m_presetPopup->setItemChecked( m_currentPreset, false );
    m_presetPopup->setItemChecked( id, true );
    m_currentPreset = id;
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

    // Transfer values to the engine if the EQ is enabled
    if ( AmarokConfig::equalizerEnabled() )
        EngineController::engine()->setEqualizerParameters( m_slider_preamp->value(), gains );

    m_equalizerGraph->update();
}


void
EqualizerSetup::sliderChanged() //SLOT
{
    m_presetPopup->setItemChecked( m_currentPreset, false );
    m_currentPreset = -1;
}


#include "equalizersetup.moc"
