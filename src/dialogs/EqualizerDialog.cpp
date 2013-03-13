/****************************************************************************************
 * Copyright (c) 2004-2009 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2009 Artur Szymiec <artur.szymiec@gmail.com>                           *
 * Copyright (c) 2013 Ralf Engels <ralf-engels@gmx.de>                                  *
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
#include "EngineController.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"

EqualizerDialog * EqualizerDialog::s_instance = 0;

EqualizerDialog::EqualizerDialog( QWidget* parent )
    : KDialog( parent )
{
    DEBUG_BLOCK

    setCaption( i18n( "Configure Equalizer" ) );

    setupUi( this );

    // again the ui file does not define the dialog but a widget.
    // Since we inherit from KDialog we have to do the following three
    // lines.
    QGridLayout* layout = new QGridLayout( this );
    layout->addWidget( EqualizerWidget );
    setMainWidget( EqualizerWidget );

    // Check if equalizer is supported - disable controls if not
    if( !The::engineController()->isEqSupported() )
    {
        EqualizerWidget->setDisabled( true );
        activeCheckBox->setEnabled( false );
        activeCheckBox->setChecked( false );
    }

    connect(this, SIGNAL( cancelClicked() ), this, SLOT( restoreOriginalSettings() ));

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

    // Ask engine for maximum gain value and compute scale to display values
    mValueScale = The::engineController()->eqMaxGain();
    QString mlblText = i18n( "%0\ndB" ).arg( QString::number( mValueScale, 'f', 1 ) );
    eqMaxEq->setText( QString("+") + mlblText );
    eqMinEq->setText( QString("-") + mlblText );

    // Ask engine for band frequencies and set labels
    QStringList meqBandFrq = The::engineController()->eqBandsFreq();
    QStringListIterator i( meqBandFrq );
    foreach( QLabel* mLabel, mBandsLabels )
        mLabel->setText( i.hasNext() ?  i.next() : "N/A" );

    mBandsLabels.first()->setText( i18n( "%0\ndB" ).arg( mBandsLabels.first()->text() ) );

    updatePresets();
    activeCheckBox->setChecked( AmarokConfig::equalizerMode() > 0 );
    setPreset( AmarokConfig::equalizerMode() - 1 );
    setGains( AmarokConfig::equalizerGains() );

    // Configure signal and slots to handle presets
    connect( activeCheckBox, SIGNAL( toggled( bool ) ), SLOT( setActive( bool ) ) );
    connect( eqPresets, SIGNAL( currentIndexChanged( int ) ), SLOT( setPreset( int ) ) );
    connect( eqPresets, SIGNAL( editTextChanged( QString ) ), SLOT( updateUi() ) );
    foreach( QSlider* mSlider, mBands )
        connect( mSlider, SIGNAL(  valueChanged( int ) ), SLOT( bandsChanged() ) );

    eqPresetSaveBtn->setIcon( KIcon( "document-save" ) );
    connect( eqPresetSaveBtn, SIGNAL( clicked() ), SLOT( savePreset() ) );

    eqPresetDeleteBtn->setIcon( KIcon( "edit-delete" ) );
    connect( eqPresetDeleteBtn, SIGNAL( clicked() ), SLOT( deletePreset() ) );

    eqPresetResetBtn->setIcon( KIcon( "edit-undo" ) );
    connect( eqPresetResetBtn, SIGNAL( clicked() ), SLOT( restorePreset() ) );
}

EqualizerDialog::~EqualizerDialog()
{ }

void EqualizerDialog::showOnce( QWidget *parent )
{
    DEBUG_BLOCK

    if( s_instance == 0 )
        s_instance = new EqualizerDialog( parent );

    s_instance->activateWindow();
    s_instance->show();
    s_instance->raise();
    s_instance->storeOriginalSettings();
}

QList<int>
EqualizerDialog::gains() const
{
    QList<int> result;
    foreach( QSlider* mSlider, mBands )
        result << mSlider->value();
    return result;
}

void
EqualizerDialog::setGains( QList<int> eqGains )
{
    for( int i = 0; i < mBands.count() && i < eqGains.count(); i++ )
    {
        // Update slider values with signal blocking to prevent circular loop
        mBands[i]->blockSignals( true );
        mBands[i]->setValue( eqGains[ i ] );
        mBands[i]->blockSignals( false );
    }

    bandsChanged();
}

void
EqualizerDialog::storeOriginalSettings()
{
    mOriginalActivated = activeCheckBox->isChecked();
    mOriginalPreset = selectedPresetName();
    mOriginalGains = gains();
}

void
EqualizerDialog::restoreOriginalSettings()
{
    activeCheckBox->setChecked( mOriginalActivated );
    int originalPresetIndex = EqualizerPresets::eqGlobalList().indexOf( mOriginalPreset );
    setPreset( originalPresetIndex );
    eqPresets->setEditText( mOriginalPreset );
    setGains( mOriginalGains );
}

void
EqualizerDialog::setActive( bool active ) //SLOT
{
    Q_UNUSED( active );

    updateUi();
    updateEngine();
}

void
EqualizerDialog::setPreset( int index ) //SLOT
{
    if( index < 0 )
        index = 0;

    // if not called from the eqPreset->indexChanged signal we need
    // to update the combo box too.
    if( eqPresets->currentIndex() != index )
    {
        eqPresets->blockSignals( true );
        eqPresets->setCurrentIndex( index );
        eqPresets->blockSignals( false );
    }

    setGains( EqualizerPresets::eqCfgGetPresetVal( selectedPresetName() ) ); // this also does updatUi and updateEngine
}

void
EqualizerDialog::bandsChanged() //SLOT
{
    updateToolTips();
    updateLabels();
    updateUi();
    updateEngine();
}

void
EqualizerDialog::updateUi() // SLOT
{
    const QString currentName = selectedPresetName();

    const bool enabledState = activeCheckBox->isChecked();
    const bool userState = EqualizerPresets::eqUserList().contains( currentName );
    const bool modified = ( EqualizerPresets::eqCfgGetPresetVal( currentName ) != gains() );
    const bool nameModified = ! EqualizerPresets::eqGlobalList().contains( currentName );
    const bool resetable = EqualizerPresets::eqCfgCanRestorePreset( currentName );

    eqPresets->setEnabled( enabledState );
    eqBandsGroupBox->setEnabled( enabledState );
    eqPresetSaveBtn->setEnabled( enabledState && ( modified || nameModified ) );
    eqPresetDeleteBtn->setEnabled( enabledState && userState );
    eqPresetResetBtn->setEnabled( enabledState && ( resetable || modified ) );
}

void
EqualizerDialog::updatePresets()
{
    const QString currentName = selectedPresetName();

    eqPresets->blockSignals( true );
    eqPresets->clear();
    eqPresets->addItems( EqualizerPresets::eqGlobalTranslatedList() );
    const int index = EqualizerPresets::eqGlobalList().indexOf( currentName );
    if( index >= 0 )
        eqPresets->setCurrentIndex( index );
    eqPresets->blockSignals( false );
}

void
EqualizerDialog::savePreset() //SLOT
{
    DEBUG_BLOCK

    EqualizerPresets::eqCfgSetPresetVal( selectedPresetName(), gains() );

    updatePresets(); // we might have a new one
    updateUi();
}

void
EqualizerDialog::deletePreset() //SLOT
{
    if( EqualizerPresets::eqCfgDeletePreset( selectedPresetName() ) )
    {
        updatePresets();
        setPreset( 0 );
    }
}

QString
EqualizerDialog::selectedPresetName() const
{
    const QString currentText = eqPresets->currentText();
    const int index = EqualizerPresets::eqGlobalTranslatedList().indexOf( currentText );
    if( index < 0 )
        return currentText;

    return EqualizerPresets::eqGlobalList().at( index );
}

void
EqualizerDialog::restorePreset() //SLOT
{
    DEBUG_BLOCK

    EqualizerPresets::eqCfgRestorePreset( selectedPresetName() );
    setGains( EqualizerPresets::eqCfgGetPresetVal( selectedPresetName() ) );
}

void
EqualizerDialog::updateToolTips()
{
    foreach( QSlider* mSlider, mBands )
        mSlider->setToolTip( QString::number( mSlider->value()*mValueScale/100.0, 'f', 1 ) );
}

void
EqualizerDialog::updateLabels()
{
    for( int i = 0; i < mBandsValues.count() && i < mBands.count(); i++ )
        mBandsValues[i]->setText( QString::number( mBands[i]->value() * mValueScale / 100.0, 'f', 1 ) );
}

void
EqualizerDialog::updateEngine() //SLOT
{
    DEBUG_BLOCK

    AmarokConfig::setEqualizerMode( activeCheckBox->isChecked() ?
                                    eqPresets->currentIndex() + 1 : 0 ); // equalizer mode 0 is off
    AmarokConfig::setEqualizerGains( gains() );
    The::engineController()->eqUpdate();
}


#include "EqualizerDialog.moc"
