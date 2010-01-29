/****************************************************************************************
 * Copyright (c) 2004-2010 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2005 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2005 Markus Brueffer <markus@brueffer.de>                              *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "equalizersetup.h"

#include "Amarok.h"
#include "amarokconfig.h"
#include "EngineController.h"
#include "equalizergraph.h"
#include "equalizerpresetmanager.h"
#include "widgets/SliderWidget.h"

#include <KComboBox>
#include <KInputDialog>  //presets
#include <KLocale>
#include <KMessageBox>
#include <KStandardDirs> //locate()
#include <KVBox>
#include <KWindowSystem>

#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>
#include <QFile>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QTextStream>   //presets


EqualizerSetup* EqualizerSetup::s_instance = 0;


EqualizerSetup::EqualizerSetup()
    : KDialog( Amarok::mainWindow() )
{
    setModal( false );
    setButtons( Ok );
    showButtonSeparator( false );

    using Amarok::Slider;

    s_instance = this;

    kapp->setTopWidget( this );
    setCaption( KDialog::makeStandardCaption( i18n( "Equalizer" ) ) );

    // Gives the window a small title bar, and skips a taskbar entry
#ifdef Q_WS_X11
    KWindowSystem::setType( winId(), NET::Utility );
    KWindowSystem::setState( winId(), NET::SkipTaskbar );
#endif

    QWidget* vbox = new QWidget( this );
    setMainWidget( vbox );
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    vbox->setLayout( vboxLayout );
    vboxLayout->setSpacing( KDialog::spacingHint() );

    // BEGIN Presets
    QWidget* presetBox = new QWidget( vbox );
    vboxLayout->addWidget( presetBox );
    QHBoxLayout *presetLayout = new QHBoxLayout;
    presetLayout->setSpacing( KDialog::spacingHint() );
    presetBox->setLayout( presetLayout );

    presetLayout->addWidget( new QLabel( i18n("Presets:") ) );

    m_presetCombo = new KComboBox;
    presetLayout->addWidget( m_presetCombo );
    m_presetCombo->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred ) );

    QPushButton* presetAdd = new QPushButton;
    presetLayout->addWidget( presetAdd );
    presetAdd->setIcon( KIcon( "list-add-amarok" ) );
    presetAdd->setToolTip( i18n("Add new preset") );
    connect( presetAdd, SIGNAL( clicked() ), SLOT( addPreset() ) );

    QPushButton* presetConf = new QPushButton;
    presetLayout->addWidget( presetConf );
    presetConf->setIcon( KIcon( "configure-amarok" ) );
    presetConf->setToolTip( i18n("Manage presets") );
    connect( presetConf, SIGNAL( clicked() ), SLOT( editPresets() ) );

    loadPresets();
    connect( m_presetCombo, SIGNAL( activated(int) ), SLOT( presetChanged(int) ) );
    // END Presets

    // BEGIN GroupBox
    m_groupBoxSliders = new QGroupBox( vbox );
    vboxLayout->addWidget( m_groupBoxSliders );
    m_groupBoxSliders->setTitle( i18n("Enable Equalizer") );
    m_groupBoxSliders->setCheckable( true );
    m_groupBoxSliders->setChecked( AmarokConfig::equalizerEnabled() );
    connect( m_groupBoxSliders, SIGNAL( toggled( bool ) ), SLOT( setEqualizerEnabled( bool ) ) );
    QVBoxLayout *v = new QVBoxLayout( m_groupBoxSliders );

    // Helper widget for layouting inside the groupbox
    QWidget* slidersLayoutWidget = new QWidget( m_groupBoxSliders );
    v->addWidget( slidersLayoutWidget );
    slidersLayoutWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QGridLayout* slidersGridLayout = new QGridLayout( slidersLayoutWidget );
    slidersGridLayout->setSpacing( KDialog::spacingHint() );
    // END GroupBox

    // BEGIN Preamp slider
    m_slider_preamp = new Slider( Qt::Vertical, slidersLayoutWidget, 100 );
    m_slider_preamp->setMinimum( -100 );
    m_slider_preamp->setTickPosition( QSlider::TicksRight );
    m_slider_preamp->setTickInterval( 100 );
    connect( m_slider_preamp, SIGNAL( valueChanged( int ) ), SLOT( setEqualizerParameters() ) );
    slidersGridLayout->addMultiCellWidget(m_slider_preamp, 0, 0, 0, 1, Qt::AlignHCenter );

    QLabel* preampLabel = new QLabel( i18n("Pre-amp"), slidersLayoutWidget );
    slidersGridLayout->addMultiCellWidget(preampLabel, 1, 1 , 0, 1, Qt::AlignHCenter );
    // END

    // BEGIN Band Sliders
    const char *bandLabels[] = { "30", "60", "125", "250", "500", "1k", "2k", "4k", "8k", "16k" };

    int minWidth = 0;
    QFontMetrics fm = fontMetrics(); //apparently it's an expensive call
    for ( int i = 0; i < 10; i++ )
    {
         int w = fm.width( bandLabels[i] );
         if ( w > minWidth )
             minWidth = w;
    }

    for ( int i = 0; i < 10; i++ )
    {
        Slider *slider = new Slider( Qt::Vertical, slidersLayoutWidget );
        QLabel *label  = new QLabel( slidersLayoutWidget );
        label->setText( bandLabels[i] );

        slider->setMinimum( -100 );
        slider->setMaximum( +100 );
        slider->setMinimumWidth( minWidth );
        slidersGridLayout->addMultiCellWidget(slider, 0, 0, 2 * i + 2, 2 * i + 3, Qt::AlignHCenter );
        slidersGridLayout->addMultiCellWidget(label, 1, 1, 2 * i + 2, 2 * i + 3, Qt::AlignHCenter );
        m_bandSliders.append( slider );

        connect( slider, SIGNAL( valueChanged( int ) ), SLOT( setEqualizerParameters() ) );
        connect( slider, SIGNAL( valueChanged( int ) ), SLOT( sliderChanged() ) );
    }
    // END

    // BEGIN Equalizer Graph Widget
    QGroupBox* graphGBox = new QGroupBox();
    vboxLayout->addWidget( graphGBox );
    QHBoxLayout* h = new QHBoxLayout( graphGBox );

    KVBox* graphVBox = new KVBox();
    h->addWidget( graphVBox );
    QLabel* graphLabel1 = new QLabel(graphVBox);
    graphLabel1->setText("+20 db");
    QLabel* graphLabel2 = new QLabel(graphVBox);
    graphLabel2->setText("0 db");
    QLabel* graphLabel3 = new QLabel(graphVBox);
    graphLabel3->setText("-20 db");
    graphLabel1->setAlignment( Qt::AlignRight | Qt::AlignTop );
    graphLabel2->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
    graphLabel3->setAlignment( Qt::AlignRight | Qt::AlignBottom );

    m_equalizerGraph = new EqualizerGraph( graphGBox );
    h->addWidget( m_equalizerGraph );
    m_equalizerGraph->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    // END Graph Widget

    // Fill the combobox
    updatePresets( AmarokConfig::equalizerPreset() );

    // make sure to restore the current preamp value
    m_slider_preamp->setValue( AmarokConfig::equalizerPreamp() );

    // Init sliders
    presetChanged( AmarokConfig::equalizerPreset() );
}


EqualizerSetup::~EqualizerSetup()
{
    savePresets();
    s_instance = 0;
}

void
EqualizerSetup::setActive( bool active )
{
    m_groupBoxSliders->setChecked( active );
}

void
EqualizerSetup::setBands( int preamp, QList<int> gains )
{
    m_slider_preamp->setValue( preamp );

    // Note: As a side effect, this automatically switches the
    //       preset to 'Manual', which is by intention
    for ( int i = 0; i < m_bandSliders.count(); i++ )
        m_bandSliders.at(i)->setValue( gains.at(i) );

    setEqualizerParameters();
}

void
EqualizerSetup::setPreset( QString name )
{
    // Look for the preset id and verify name
    int i, count = m_presetCombo->count();
    bool found = false;
    for( i = 0; i < count; i++ ) {
        if ( m_presetCombo->itemText( i ) == name ) {
            found = true;
            break;
        }
    }

    if ( found )
    {
        m_presetCombo->setCurrentIndex( i );
        presetChanged( name );
    }
}

/////////////////////////////////////////////////////////////////////////////////////
// EQUALIZER PRESETS
/////////////////////////////////////////////////////////////////////////////////////

QString
EqualizerSetup::presetsCache() const
{
    // returns the playlists stats cache file
    return Amarok::saveLocation() + "equalizerpresets_save.xml";
}


void
EqualizerSetup::loadPresets()
{
    // Create predefined presets 'Zero' and 'Manual'
    QList<int> zeroGains;
    zeroGains << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0;
    m_presets[ i18n("Manual") ] = zeroGains;
    m_presets[ i18n("Zero") ] = zeroGains;

    QFile file( presetsCache() );
    if ( !file.exists() )
        file.setFileName( KStandardDirs::locate( "data", "amarok/data/equalizer_presets.xml" ) );

    QTextStream stream( &file );
    stream.setCodec( "UTF8" );

    QDomDocument d;

    if( !file.open( QIODevice::ReadOnly ) || !d.setContent( stream.readAll() ) )
    {
        // Everything went wrong, so at least provide the two predefined presets
        updatePresets( AmarokConfig::equalizerPreset() );
        return;
    }

    QDomNode n = d.namedItem( "equalizerpresets" ).namedItem("preset");

    for( ; !n.isNull();  n = n.nextSibling() )
    {
        QDomElement e = n.toElement();
        QString title = e.attribute( "name" );

        QList<int> gains;
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

        m_presets[ title ] = gains;
    }

    file.close();
}


void
EqualizerSetup::savePresets()
{
    QFile file( presetsCache() );

    if( !file.open( QIODevice::WriteOnly ) ) return;

    QDomDocument doc;
    QDomElement e = doc.createElement("equalizerpresets");
    e.setAttribute( "product", "Amarok" );
    e.setAttribute( "version", AMAROK_VERSION );
    e.setAttribute( "formatversion", "1.1" );

    doc.appendChild( e );

    QStringList info;
    info << "b0" << "b1" << "b2" << "b3" << "b4"
         << "b5" << "b6" << "b7" << "b8" << "b9";

    for( int x = 0; x < m_presets.count(); x++ )
    {
        const QString title = m_presetCombo->itemText( x );

        // don't save the 'Zero' preset
        if ( title == i18n("Zero") )
            continue;

        QList<int> gains = m_presets[ title ];

        QDomElement i = doc.createElement("preset");
        i.setAttribute( "name", title );

        QDomElement attr;
        QDomText t;
        for( int y=0; y < info.count(); y++ )
        {
            attr = doc.createElement( info[y] );
            t    = doc.createTextNode( QString::number( gains.first() ) );
            attr.appendChild( t );
            i.appendChild( attr );
            gains.pop_front();
        }
        e.appendChild( i );
    }

    QTextStream stream( &file );
    stream.setCodec( "UTF8" );
    stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    stream << doc.toString();
    file.close();
}

void
EqualizerSetup::editPresets()
{
    EqualizerPresetManager editor;
    editor.setPresets( m_presets );

    if( editor.exec() )
    {
        QMap< QString, QList<int> > presets = editor.presets();

        QString currentTitle = m_presetCombo->currentText();
        QList<int> currentGains= m_presets[ currentTitle ];

        QString newTitle = currentTitle;

        // Check if the selected item was renamed
        if ( presets.find( currentTitle ) == presets.end() || currentGains != presets[ currentTitle ] ) {

            // Find the new name
            foreach( const QString &key, presets.keys() ) {
                if ( presets[key] == currentGains ) {
                    newTitle = key;
                    break;
                }
            }
        }

        m_presets = presets;
        updatePresets( newTitle );
    }
}

void
EqualizerSetup::addPreset()
{
    bool ok;
    const QString title = KInputDialog::getText( i18n("Add Equalizer Preset"),
                                                 i18n("Enter preset name:"), i18n("Untitled"), &ok, this);

    if (ok) {
        // Check if the new preset title exists
        if ( m_presets.find( title ) != m_presets.end() )
        {
            int button = KMessageBox::warningYesNo( this, i18n( "A preset with the name %1 already exists. Overwrite?", title ) );

            if ( button != KMessageBox::Yes )
                return;
        }

        // Add the new preset based on the current slider positions
        QList <int> gains;
        for ( int i = 0; i < m_bandSliders.count(); i++ )
            gains += m_bandSliders.at( i )->value();
        m_presets[ title ] = gains;

        // Rebuild the combobox
        updatePresets(title);

        // Save
        setEqualizerParameters();
    }
}

void
EqualizerSetup::updatePresets(QString selectTitle)
{
    // Save the selected item
    if ( selectTitle.isEmpty() )
        selectTitle = m_presetCombo->currentText();

    // Sort titles
    QStringList titles;
    QMap< QString, QList<int> >::ConstIterator end = m_presets.constEnd();
    for ( QMap< QString, QList<int> >::ConstIterator it = m_presets.constBegin(); it != end; ++it )
        titles << it.key();

    titles.sort();

    // rebuild preset combobox and look for the previously selected title
    int i = 0;
    int newIndex = -1;
    m_presetCombo->clear();
    QStringList::ConstIterator titlesEnd = titles.constEnd();
    for ( QStringList::ConstIterator it = titles.constBegin(); it != titlesEnd; ++it )
    {
        m_presetCombo->addItem( *it );
        if ( *it == selectTitle )
            newIndex = i;
        if ( *it == i18n("Manual") )
            m_manualPos = i;
        i++;
    }

    if ( newIndex == -1 )
        newIndex = m_manualPos;

    m_presetCombo->setCurrentIndex( newIndex );
}

/////////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
/////////////////////////////////////////////////////////////////////////////////////

void
EqualizerSetup::presetChanged( int id ) //SLOT
{
    presetChanged( m_presetCombo->itemText(id) );
}

void
EqualizerSetup::presetChanged( QString title ) //SLOT
{
    const QList<int> gains = m_presets[ title ];

    for ( int i = 0; i < m_bandSliders.count(); i++ ) {
        // Block signals to prevent unwanted setting to 'Manual'
        m_bandSliders.at(i)->blockSignals(true);
        m_bandSliders.at(i)->setValue( gains.at(i) );
        m_bandSliders.at(i)->blockSignals(false);
    }

    setEqualizerParameters();
}

void
EqualizerSetup::setEqualizerEnabled( bool active ) //SLOT
{
//TODO Phonon Equalizer    EngineController::engine()->setEqualizerEnabled( active );
    AmarokConfig::setEqualizerEnabled( active );

    if( active )
        //this way the developer of the eq doesn't have to cache the eq values
        setEqualizerParameters();
    else
        // zero the graph
        m_equalizerGraph->update();
}


void
EqualizerSetup::setEqualizerParameters() //SLOT
{
    AmarokConfig::setEqualizerPreamp( m_slider_preamp->value() );
    AmarokConfig::setEqualizerPreset( m_presetCombo->currentText() );
    AmarokConfig::setEqualizerGains ( m_presets[ m_presetCombo->currentText() ] );

    // Transfer values to the engine if the EQ is enabled
 //TODO PhononEqualizer    if ( AmarokConfig::equalizerEnabled() )
//       EngineController::engine()->setEqualizerParameters( m_slider_preamp->value(), m_presets[ m_presetCombo->currentText() ] );

    m_equalizerGraph->update();
}


void
EqualizerSetup::sliderChanged() //SLOT
{
    m_presetCombo->setCurrentIndex( m_manualPos );

    QList<int> gains;
    for ( int i = 0; i < m_bandSliders.count(); i++ )
        gains += m_bandSliders.at( i )->value();

    m_presets[ i18n("Manual") ] = gains;
}

#include "equalizersetup.moc"

