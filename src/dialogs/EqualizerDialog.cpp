/****************************************************************************************
 * Copyright (c) 2004-2009 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2009 Artur Szymiec <artur.szymiec@gmail.com>                           *
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

#include "EqualizerDialog.h"

#include "amarokconfig.h"
#include "core/support/Amarok.h"
#include "ActionClasses.h"
#include "EngineController.h"
#include "core/support/Debug.h"

#include <KMessageBox>

static int DEFAULT_PRESET_VALUES[][11] =
{
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // Manual
    {0, 0, 0, 0, 0, 0, 0, -40, -40, -40, -50}, // Classical
    {0, 0, 0, 20, 30, 30, 30, 20, 0, 0, 0}, // Club
    {-10, 50, 35, 10, 0, 0, -30, -40, -40, 0, 0}, // Dance
    {0, 29, 40, 23, 15, 0, 0, 0, 0, 0, 0}, // Full Bass
    {-83, -50, -50, -50, -25, 15, 55, 80, 80, 80, 85}, // Full Treble
    {-41, 35, 30, 0, -40, -25, 10, 45, 55, 60, 60}, // Full Bass + Treble
    {-16, 25, 50, 25, -20, 0, -30, -40, -40, 0, 0}, // Laptop/Headphones
    {-25, 50, 50, 30, 30, 0, -25, -25, -25, 0, 0}, // Large Hall
    {0, -25, 0, 20, 25, 30, 30, 20, 15, 15, 10}, // Live
    {0, 35, 35, 0, 0, 0, 0, 0, 0, 35, 35}, // Party
    {-15, -10, 25, 35, 40, 25, -5, -15, -15, -10, -10}, // Pop
    {0, 0, 0, -5, -30, 0, -35, -35, 0, 0, 0}, // Reggae
    {-28, 40, 25, -30, -40, -20, 20, 45, 55, 55, 55}, // Rock
    {-33, 25, 10, -5, -15, -5, 20, 45, 50, 55, 60}, // Soft
    {-29, -15, -25, -25, -5, 20, 30, 45, 50, 55, 50}, // Ska
    {0, 20, 20, 10, -5, -25, -30, -20, -5, 15, 45}, // Soft Rock
    {-26, 40, 30, 0, -30, -25, 0, 40, 50, 50, 45}, // Techno
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} // Zero
};


EqualizerDialog * EqualizerDialog::s_instance = 0;


EqualizerDialog * EqualizerDialog::instance()
{
    if( s_instance == 0 )
        s_instance = new EqualizerDialog();

    return s_instance;
}

EqualizerDialog::EqualizerDialog()
    : KDialog( 0 )
{
    DEBUG_BLOCK

    setCaption( i18n( "Configure Equalizer" ) );

    setupUi( this );

    // Set up equalizer items :
    eqSetupUI();

    adjustSize();
}

EqualizerDialog::~EqualizerDialog()
{
}

void EqualizerDialog::showOnce()
{
    DEBUG_BLOCK
    instance()->activateWindow();
    instance()->show();
    instance()->raise();
    instance()->eqRememberOriginalSettings();
}

void
EqualizerDialog::eqRememberOriginalSettings()
{
    mOriginalPreset = eqPresets->itemText(AmarokConfig::equalizerMode());
    mOriginalGains = AmarokConfig::equalizerGains();
}

void
EqualizerDialog::eqRestoreOriginalSettings()
{
    int originalPresetIndex = eqPresets->findText( mOriginalPreset );
    if( originalPresetIndex == -1 ) {
        // original preset seems to be deleted. will set eq to Off
        originalPresetIndex = 0;
    }
    AmarokConfig::setEqualizerMode( originalPresetIndex );
    AmarokConfig::setEqualizerGains( mOriginalGains );
    eqUpdateUI( originalPresetIndex );
    The::engineController()->eqUpdate();
}

void
EqualizerDialog::eqSetupUI()
{
    QGridLayout* layout = new QGridLayout( this );
    layout->addWidget( EqualizerGroupBox );

    setMainWidget( EqualizerGroupBox );

    // Check if equalizer is supported - disable controls if not
    if( !The::engineController()->isEqSupported() )
    {
        EqualizerGroupBox->setDisabled( true );
        EqualizerGroupBox->setTitle( i18n( "Sorry, your current Phonon backend version does not provide equalizer support." ) );
        eqPresetsGroupBox->hide();
        eqBandsGroupBox->hide();
        return;
    }

    connect(this, SIGNAL( cancelClicked() ), this, SLOT( eqRestoreOriginalSettings() ));

    // Assign slider items to vectors
    mBands.append( eqPreampSlider );
    mBands.append( eqBand0Slider );
    mBands.append( eqBand1Slider );
    mBands.append( eqBand2Slider );
    mBands.append( eqBand3Slider );
    mBands.append( eqBand4Slider );
    mBands.append( eqBand5Slider );
    mBands.append( eqBand6Slider );
    mBands.append( eqBand7Slider );
    mBands.append( eqBand8Slider );
    mBands.append( eqBand9Slider );
    mBandsValues.append( eqPreampValue );
    mBandsValues.append( eqBand0Value );
    mBandsValues.append( eqBand1Value );
    mBandsValues.append( eqBand2Value );
    mBandsValues.append( eqBand3Value );
    mBandsValues.append( eqBand4Value );
    mBandsValues.append( eqBand5Value );
    mBandsValues.append( eqBand6Value );
    mBandsValues.append( eqBand7Value );
    mBandsValues.append( eqBand8Value );
    mBandsValues.append( eqBand9Value );
    mBandsLabels.append( eqPreampLabel );
    mBandsLabels.append( eqBand0Label );
    mBandsLabels.append( eqBand1Label );
    mBandsLabels.append( eqBand2Label );
    mBandsLabels.append( eqBand3Label );
    mBandsLabels.append( eqBand4Label );
    mBandsLabels.append( eqBand5Label );
    mBandsLabels.append( eqBand6Label );
    mBandsLabels.append( eqBand7Label );
    mBandsLabels.append( eqBand8Label );
    mBandsLabels.append( eqBand9Label );
    // Configure signal and slots to handle presets
    connect( eqPresets, SIGNAL( currentIndexChanged( int ) ), SLOT ( eqPresetChanged( int ) ) );
    foreach( QSlider* mSlider, mBands )
        connect( mSlider, SIGNAL(  valueChanged( int ) ), SLOT ( eqBandsChanged() ) );
    connect( eqPresetSaveBtn, SIGNAL( clicked() ), SLOT( eqSavePreset() ) );
    connect( eqPresetDeleteBtn, SIGNAL( clicked() ), SLOT( eqDeletePreset() ) );
    connect( eqPresetResetBtn, SIGNAL( clicked() ), SLOT( eqRestorePreset() ) );
    // Signals exchange to keep both config dialog and eq action in sync
    connect( eqPresets, SIGNAL( currentIndexChanged( int ) ),
             Amarok::actionCollection()->action( "equalizer_mode" ), SLOT( updateContent() ) );
    connect( Amarok::actionCollection()->action( "equalizer_mode" ), SIGNAL( triggered(int) ),
             SLOT( eqUpdateUI( int ) ) );
    // Ask engine for maximum gain value and compute scale to display values
    mValueScale = The::engineController()->eqMaxGain();
    QString mlblText;
    mlblText = QString::number( mValueScale, 'f', 1 );
    mlblText.append( QString( "\ndB" ) );
    eqMaxEq->setText( QString("+") + mlblText );
    eqMinEq->setText( QString("-") + mlblText );
    // Ask engine for band frequencies and set labels
    QStringList meqBandFrq = The::engineController()->eqBandsFreq();
    QStringListIterator i( meqBandFrq );
    foreach( QLabel* mLabel, mBandsLabels )
        mLabel-> setText( i.hasNext() ?  i.next() : "N/A" );
    mBandsLabels.first()->setText( mBandsLabels.first()->text() + QString( "\ndB" ) );
    // Set initial preset to current with signal blocking to prevent circular loops
    eqPresets->blockSignals( true );
    eqPresets->addItem( i18nc( "Equalizer state, as in, disabled", "Off" ) );
    eqPresets->addItems( eqGlobalList() );
    eqPresets->blockSignals( false );
    eqUpdateUI( AmarokConfig::equalizerMode() );
}

void
EqualizerDialog::eqPresetChanged( int index ) //SLOT
{
    if( index < 0 )
        return;
    // new settings
    AmarokConfig::setEqualizerMode( index );
    AmarokConfig::setEqualizerGains( eqCfgGetPresetVal( eqPresets->currentText() ) );
    The::engineController()->eqUpdate();
    // update controls
    eqUpdateUI( index );
}

void
EqualizerDialog::eqBandsChanged() //SLOT
{
    // update values from sliders
    QList<int> eqGains;
    foreach( QSlider* mSlider, mBands )
        eqGains << mSlider->value();

    eqUpdateToolTips();
    eqUpdateLabels( eqGains );
    AmarokConfig::setEqualizerGains( eqGains );
    The::engineController()->eqUpdate();
    // Change preset to manual
    eqPresets->blockSignals( true );
    eqPresets->setCurrentIndex( 1 );
    eqPresets->blockSignals( false );
}

void
EqualizerDialog::eqUpdateToolTips()
{
    foreach( QSlider* mSlider, mBands )
        mSlider->setToolTip( QString::number( mSlider->value()*mValueScale/100.0, 'f', 1 ) );
}

void
EqualizerDialog::eqUpdateLabels( QList<int> & mEqGains )
{
    QListIterator<int> i( mEqGains );
    foreach( QLabel* mLabel, mBandsValues )
        mLabel->setText( i.hasNext() ? QString::number( i.next()*mValueScale/100.0, 'f', 1 ) : QString::number( 0 ) );
}

// SLOTS

void
EqualizerDialog::eqUpdateUI( int index ) // SLOT
{
    if( index < 0 )
        return;

    const bool mstate = index > 0 ? true : false;
    eqBandsGroupBox->setEnabled( mstate );
    eqPresetSaveBtn->setEnabled( mstate );
    eqPresetDeleteBtn->setEnabled( mstate );
    eqPresetResetBtn->setEnabled( mstate );
    QList<int> eqGains = AmarokConfig::equalizerGains();
    QListIterator<int> i( eqGains );
    // Update slider values with signal blocking to prevent circular loop
    foreach( QSlider* mSlider, mBands )
    {
        mSlider->blockSignals( true );
        mSlider->setValue( i.hasNext() ? i.next() : 0 );
        mSlider->blockSignals( false );
    }
    eqUpdateToolTips();
    eqUpdateLabels(eqGains);
    // Update preset list - with signal blocking to prevent circular loop
    eqPresets->blockSignals( true );
    eqPresets->setCurrentIndex( index );
    eqPresets->blockSignals( false) ;
}

void
EqualizerDialog::eqDeletePreset() //SLOT
{
    QString mPresetSelected = eqPresets->currentText();
    if( eqCfgDeletePreset( mPresetSelected ) )
    {
        eqPresets->blockSignals( true );
        eqPresets->clear();
        eqPresets->addItem( i18nc( "Equalizer state, as in, disabled", "Off" ) );
        eqPresets->addItems( eqGlobalList() );
        eqPresets->blockSignals( false );
        static_cast<Amarok::EqualizerAction*>( Amarok::actionCollection()->action( "equalizer_mode") )->newList();
        eqPresets->setCurrentIndex( 1 );
    }
    else
    {
        KMessageBox::detailedSorry( 0, i18n( "Cannot delete this preset" ),
                                       i18n( "Default presets can not be deleted" ),
                                       i18n( "Error deleting preset" ) );
    }
}

void
EqualizerDialog::eqRestorePreset() //SLOT
{
    const QString mPresetSelected = eqPresets->currentText();
    if( !eqCfgRestorePreset( mPresetSelected ) )
    {
        KMessageBox::detailedSorry( 0, i18n( "Cannot restore this preset" ),
                                       i18n( "Only default presets can be restored" ),
                                       i18n( "Error restoring preset" ) );
        return;
    }
    // new settings
    ///AmarokConfig::setEqualizerMode( eqPresets->currentIndex() );
    AmarokConfig::setEqualizerGains( eqCfgGetPresetVal( eqPresets->currentText() ) );
    The::engineController()->eqUpdate();
    // update controls
    eqUpdateUI( eqPresets->currentIndex() );
}

void
EqualizerDialog::eqSavePreset() //SLOT
{
    QString mPresetSelected = eqPresets->currentText();
    if( mPresetSelected == QLatin1String( "Manual" ) )
    {
        KMessageBox::detailedSorry( 0, i18n( "Cannot save this preset" ),
                                       i18n( "Preset 'Manual' is reserved for momentary settings.\n\
                                              Please choose different name and try again." ),
                                       i18n( "Error saving preset" ) );
        return;
    }

    QList<int> eqGains;
    foreach( QSlider* mSlider, mBands )
        eqGains << mSlider->value();
    eqCfgSetPresetVal( mPresetSelected, eqGains );
    eqPresets->blockSignals( true );
    eqPresets->clear();
    eqPresets->addItem( i18nc( "Equalizer state, as in, disabled", "Off" ) );
    eqPresets->addItems( eqGlobalList() );
    ( (Amarok::EqualizerAction*) Amarok::actionCollection()->action( "equalizer_mode") )->newList();
    eqPresets->blockSignals( false );
    eqPresets->setCurrentIndex( eqPresets->findText( mPresetSelected ) );
}

// Equalizer preset management helper functions
bool
EqualizerDialog::eqCfgDeletePreset( QString & mPresetName )
{
      // Idea is to delete the preset only if it is user preset:
      // present on user list & absent on default list
      const int idUsr = AmarokConfig::equalizerPresetsNames().indexOf( mPresetName );
      const int idDef = eqDefaultPresetsList().indexOf( mPresetName );

      if( idUsr >= 0 && idDef < 0 )
      {
          QStringList mNewNames = AmarokConfig::equalizerPresetsNames();
          QList<int> mNewValues = AmarokConfig::equalizerPresestValues();
          mNewNames.removeAt( idUsr );

          for( int it = 0; it <= 10; it++ )
              mNewValues.removeAt( 11*idUsr );

          AmarokConfig::setEqualizerPresetsNames( mNewNames );
          AmarokConfig::setEqualizerPresestValues( mNewValues );
          return true;
      }

      return false;
}

bool
EqualizerDialog::eqCfgRestorePreset( QString mPresetName )
{
      // Idea is to delete the preset if it found on both
      // user list and default list - delete from the latter if so
      const int idUsr = AmarokConfig::equalizerPresetsNames().indexOf( mPresetName );
      const int idDef = eqDefaultPresetsList().indexOf( mPresetName );

      if( idDef >= 0 )
      {
          QStringList mNewNames = AmarokConfig::equalizerPresetsNames();
          QList<int> mNewValues = AmarokConfig::equalizerPresestValues();
          mNewNames.removeAt( idUsr );

          for( int it = 0; it <= 10; it++ )
              mNewValues.removeAt( 11*idUsr );

          AmarokConfig::setEqualizerPresetsNames( mNewNames );
          AmarokConfig::setEqualizerPresestValues( mNewValues );
          return true;
      }

      return false;
}

void
EqualizerDialog::eqCfgSetPresetVal( QString & mPresetName, QList<int> & mPresetValues)
{
    // Idea is to insert new values into user list
    // if preset exist on the list - replace it values
    const int idUsr = AmarokConfig::equalizerPresetsNames().indexOf( mPresetName );
    QStringList mNewNames = AmarokConfig::equalizerPresetsNames();
    QList<int> mNewValues = AmarokConfig::equalizerPresestValues();

    if( idUsr < 0 )
    {
        mNewNames.append( mPresetName );
        mNewValues += mPresetValues;
    }
    else
    {
        for( int it = 0; it <= 10; it++ )
            mNewValues.replace( idUsr * 11 + it, mPresetValues.value(it) );
    }
    AmarokConfig::setEqualizerPresetsNames( mNewNames );
    AmarokConfig::setEqualizerPresestValues( mNewValues );
}

QList<int>
EqualizerDialog::eqCfgGetPresetVal( QString mPresetName )
{
      // Idea is to return user preset with request name first
      // if not look into into default preset names
      const int idUsr = AmarokConfig::equalizerPresetsNames().indexOf( mPresetName );
      const int idDef = eqDefaultPresetsList().indexOf( mPresetName );

      QList<int> mPresetVal;
      if( idUsr >= 0 )
          mPresetVal = AmarokConfig::equalizerPresestValues().mid( idUsr * 11, 11 );
      else if( idDef >= 0) {
          for (int i = 0; i < 11; ++i)
              mPresetVal << DEFAULT_PRESET_VALUES[idDef][i];
      }

      return mPresetVal;
}


QStringList
EqualizerDialog::eqGlobalList()
{
    // This function will build up a global list
    // first a default preset will comes
    // then user list is filtered to omit duplicates from default preset list
    QStringList mGlobalList;
    mGlobalList += EqualizerDialog::eqDefaultPresetsList();
    foreach( const QString &mUsrName, AmarokConfig::equalizerPresetsNames() )
    {
        if( mGlobalList.indexOf( mUsrName ) < 0 )
            mGlobalList.append( mUsrName );
    }
    return mGlobalList;
}

QStringList
EqualizerDialog::eqDefaultPresetsList()
{
    QStringList presets;
    presets << "Manual"
            << "Classical"
            << "Club"
            << "Dance"
            << "Full Bass"
            << "Full Treble"
            << "Full Bass"
            << "Treble"
            << "Laptop/Headphones"
            << "Large Hall"
            << "Live"
            << "Party"
            << "Pop"
            << "Reggae"
            << "Rock"
            << "Soft"
            << "Ska"
            << "Soft Rock"
            << "Techno"
            << "Zero";
    return presets;
}

QStringList
EqualizerDialog::eqDefaultTranslatedPresetsList()
{
    QStringList strings;
    strings << i18n( "Manual" );
    strings << i18n( "Classical" );
    strings << i18n( "Club" );
    strings << i18n( "Dance" );
    strings << i18n( "Full Bass" );
    strings << i18n( "Full Treble" );
    strings << i18n( "Treble" );
    strings << i18n( "Laptop/Headphones" );
    strings << i18n( "Large Hall" );
    strings << i18n( "Live" );
    strings << i18n( "Party" );
    strings << i18n( "Pop" );
    strings << i18n( "Reggae" );
    strings << i18n( "Rock" );
    strings << i18n( "Soft" );
    strings << i18n( "Ska" );
    strings << i18n( "Soft Rock" );
    strings << i18n( "Techno");
    strings << i18n( "Zero" );
    return strings;
}



namespace The {

    EqualizerDialog* equalizer()
    {
        return EqualizerDialog::instance();
    }
}

#include "EqualizerDialog.moc"
