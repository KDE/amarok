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
    QString mlblText = i18n( "%0\ndB" ).arg( QString::number( mValueScale, 'f', 1 ) );
    eqMaxEq->setText( QString("+") + mlblText );
    eqMinEq->setText( QString("-") + mlblText );
    // Ask engine for band frequencies and set labels
    QStringList meqBandFrq = The::engineController()->eqBandsFreq();
    QStringListIterator i( meqBandFrq );
    foreach( QLabel* mLabel, mBandsLabels )
        mLabel-> setText( i.hasNext() ?  i.next() : "N/A" );

    mBandsLabels.first()->setText( i18n( "%0\ndB" ).arg( mBandsLabels.first()->text() ) );
    // Set initial preset to current with signal blocking to prevent circular loops
    eqRepopulateUi();
    eqUpdateUI( AmarokConfig::equalizerMode() );
}

void
EqualizerDialog::eqPresetChanged( int index ) //SLOT
{
    DEBUG_BLOCK

    if( index < 0 )
        return;
    // new settings
    AmarokConfig::setEqualizerMode( index );
    AmarokConfig::setEqualizerGains( mPresets.eqCfgGetPresetVal( eqSelectedPresetName() ) );
    The::engineController()->eqUpdate();
    // update controls
    eqUpdateUI( index );
}

void
EqualizerDialog::eqBandsChanged() //SLOT
{
    DEBUG_BLOCK

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
EqualizerDialog::eqRepopulateUi()
{
    eqPresets->blockSignals( true );
    eqPresets->clear();
    eqPresets->addItem( i18nc( "Equalizer state, as in, disabled", "Off" ) );
    eqPresets->addItems( mPresets.eqGlobalTranslatedList() );
    eqPresets->blockSignals( false );
    static_cast<Amarok::EqualizerAction*>( Amarok::actionCollection()->action( "equalizer_mode") )->newList();
}

void
EqualizerDialog::eqDeletePreset() //SLOT
{
    QString mPresetSelected = eqSelectedPresetName();
    if( mPresets.eqCfgDeletePreset( mPresetSelected ) )
    {
        eqRepopulateUi();
        eqPresets->setCurrentIndex( 1 );
    }
    else
    {
        KMessageBox::detailedSorry( 0, i18n( "Cannot delete this preset" ),
                                       i18n( "Default presets can not be deleted" ),
                                       i18n( "Error deleting preset" ) );
    }
}

QString
EqualizerDialog::eqSelectedPresetName() const
{
    const int index = eqPresets->currentIndex();
    if (index < 0)
        return QString();

    // use offset by one since the first entry ("Off") is not part of the global list
    return mPresets.eqGlobalList().at(index - 1);
}

void
EqualizerDialog::eqRestorePreset() //SLOT
{
    DEBUG_BLOCK

    const QString mPresetSelected = eqSelectedPresetName();
    if( !mPresets.eqCfgRestorePreset( mPresetSelected ) )
    {
        KMessageBox::detailedSorry( 0, i18n( "Cannot restore this preset" ),
                                       i18n( "Only default presets can be restored" ),
                                       i18n( "Error restoring preset" ) );
        return;
    }
    // new settings
    ///AmarokConfig::setEqualizerMode( eqPresets->currentIndex() );
    AmarokConfig::setEqualizerGains( mPresets.eqCfgGetPresetVal( mPresetSelected ) );
    The::engineController()->eqUpdate();
    // update controls
    eqUpdateUI( eqPresets->currentIndex() );
}

void
EqualizerDialog::eqSavePreset() //SLOT
{
    DEBUG_BLOCK

    const QString mPresetSelected = eqSelectedPresetName();
    const QString mPresetName = eqPresets->currentText();
    if( mPresetSelected == QLatin1String( "Manual" ) && mPresetName == QLatin1String("Manual") )
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
    mPresets.eqCfgSetPresetVal( mPresetName, eqGains );
    eqRepopulateUi();
    eqPresets->setCurrentIndex( eqPresets->findText( mPresetName ) );
}

namespace The {

    EqualizerDialog* equalizer()
    {
        return EqualizerDialog::instance();
    }
}

#include "EqualizerDialog.moc"
