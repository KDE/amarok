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

EqualizerDialog * EqualizerDialog::s_instance = nullptr;

EqualizerDialog::EqualizerDialog( QWidget* parent )
    : QDialog( parent )
{
    DEBUG_BLOCK

    setWindowTitle( i18n( "Configure Equalizer" ) );

    setupUi( this );

    EqualizerController *equalizer = The::engineController()->equalizerController();
    // Check if equalizer is supported - disable controls if not
    if( !equalizer->isEqSupported() )
    {
        EqualizerWidget->setDisabled( true );
        activeCheckBox->setEnabled( false );
        activeCheckBox->setChecked( false );
    }

    connect(buttonBox, &QDialogButtonBox::accepted, this, &EqualizerDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &EqualizerDialog::restoreOriginalSettings);

    // Assign slider items to vectors
    m_bands.append( eqPreampSlider );
    m_bands.append( eqBand0Slider );
    m_bands.append( eqBand1Slider );
    m_bands.append( eqBand2Slider );
    m_bands.append( eqBand3Slider );
    m_bands.append( eqBand4Slider );
    m_bands.append( eqBand5Slider );
    m_bands.append( eqBand6Slider );
    m_bands.append( eqBand7Slider );
    m_bands.append( eqBand8Slider );
    m_bands.append( eqBand9Slider );
    m_bandValues.append( eqPreampValue );
    m_bandValues.append( eqBand0Value );
    m_bandValues.append( eqBand1Value );
    m_bandValues.append( eqBand2Value );
    m_bandValues.append( eqBand3Value );
    m_bandValues.append( eqBand4Value );
    m_bandValues.append( eqBand5Value );
    m_bandValues.append( eqBand6Value );
    m_bandValues.append( eqBand7Value );
    m_bandValues.append( eqBand8Value );
    m_bandValues.append( eqBand9Value );
    m_bandLabels.append( eqPreampLabel );
    m_bandLabels.append( eqBand0Label );
    m_bandLabels.append( eqBand1Label );
    m_bandLabels.append( eqBand2Label );
    m_bandLabels.append( eqBand3Label );
    m_bandLabels.append( eqBand4Label );
    m_bandLabels.append( eqBand5Label );
    m_bandLabels.append( eqBand6Label );
    m_bandLabels.append( eqBand7Label );
    m_bandLabels.append( eqBand8Label );
    m_bandLabels.append( eqBand9Label );

    // Ask engine for maximum gain value and compute scale to display values
    mValueScale = equalizer->eqMaxGain();
    const QString mlblText = i18nc( "label for equalizer gain and suffix", "%1\ndB", QString::number( mValueScale, 'f', 1 ) );
    eqMaxEq->setText( QStringLiteral("+") + mlblText );
    eqMinEq->setText( QStringLiteral("-") + mlblText );

    // Ask engine for band frequencies and set labels
    const QStringList equalizerBandFreq = equalizer->eqBandsFreq();
    QStringListIterator i( equalizerBandFreq );

    // Check if preamp is supported by Phonon backend
    if( equalizerBandFreq.size() == s_equalizerBandsNum ) {
        // Preamp not supported, so hide its slider
        eqPreampLabel->hide();
        eqPreampSlider->hide();
        eqPreampValue->hide();
    }
    else if( i.hasNext() ) // If preamp is present then skip its label as it is hard-coded in UI
        i.next();
    for( QLabel* mLabel : m_bandLabels )
        if( mLabel->objectName() != QStringLiteral("eqPreampLabel") )
            mLabel->setText( i.hasNext() ?  i.next() : QStringLiteral("N/A") );

    updatePresets();
    activeCheckBox->setChecked( equalizer->enabled() );

    equalizer->applyEqualizerPresetByIndex( AmarokConfig::equalizerMode() - 1 );
    equalizer->setGains( equalizer->gains() );
    updateUi();

    connect( equalizer, &EqualizerController::presetsChanged, this, &EqualizerDialog::presetsChanged );
    connect( equalizer, &EqualizerController::gainsChanged, this, &EqualizerDialog::gainsChanged );
    connect( equalizer, &EqualizerController::presetApplied, this, &EqualizerDialog::presetApplied );

    // Configure signal and slots to handle presets
    connect( activeCheckBox, &QCheckBox::toggled, this, &EqualizerDialog::toggleEqualizer );
    connect( eqPresets, QOverload<int>::of(&QComboBox::currentIndexChanged),
             equalizer, &EqualizerController::applyEqualizerPresetByIndex );
    connect( eqPresets, &QComboBox::editTextChanged, this, &EqualizerDialog::updateUi );
    for( QSlider* mSlider : m_bands )
        connect( mSlider, &QSlider::valueChanged, this, &EqualizerDialog::bandsChanged );

    eqPresetSaveBtn->setIcon( QIcon::fromTheme( QStringLiteral("document-save") ) );
    connect( eqPresetSaveBtn, &QAbstractButton::clicked, this, &EqualizerDialog::savePreset );

    eqPresetDeleteBtn->setIcon( QIcon::fromTheme( QStringLiteral("edit-delete") ) );
    connect( eqPresetDeleteBtn, &QAbstractButton::clicked, this, &EqualizerDialog::deletePreset );

    eqPresetResetBtn->setIcon( QIcon::fromTheme( QStringLiteral("edit-undo") ) );
    connect( eqPresetResetBtn, &QAbstractButton::clicked, this, &EqualizerDialog::restorePreset );
}

EqualizerDialog::~EqualizerDialog()
{ }

void EqualizerDialog::showOnce( QWidget *parent )
{
    DEBUG_BLOCK

    if( s_instance == nullptr )
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
    for( QSlider* mSlider : m_bands )
        result << mSlider->value();
    return result;
}

void
EqualizerDialog::gainsChanged( const QList<int> &eqGains )
{
    for( int i = 0; i < m_bands.count() && i < eqGains.count(); i++ )
    {
        // Update slider values with signal blocking to prevent circular loop
        m_bands[i]->blockSignals( true );
        m_bands[i]->setValue( eqGains[ i ] );
        m_bands[i]->blockSignals( false );
    }
    updateToolTips();
    updateLabels();
    updateUi();
}

void
EqualizerDialog::storeOriginalSettings()
{
    m_originalActivated = activeCheckBox->isChecked();
    m_originalPreset = selectedPresetName();
    m_originalGains = gains();
}

void
EqualizerDialog::restoreOriginalSettings()
{
    // Only restore original settings if the equalizer was originally enabled
    // or if the equalizer is currently enabled. This prevents a reset of the
    // equalizer when cancel button is clicked with equalizer toggle off.
    if( m_originalActivated || activeCheckBox->isChecked() )
    {
        activeCheckBox->setChecked( m_originalActivated );
        int originalPresetIndex = EqualizerPresets::eqGlobalList().indexOf( m_originalPreset );
        The::engineController()->equalizerController()->applyEqualizerPresetByIndex( originalPresetIndex );
        eqPresets->setEditText( m_originalPreset );
        The::engineController()->equalizerController()->setGains( m_originalGains );
    }
    this->reject();
}

void
EqualizerDialog::presetApplied( int index ) //SLOT
{
    if( index < 0 )
        return;

    // if not called from the eqPreset->indexChanged signal we need
    // to update the combo box too.
    if( eqPresets->currentIndex() != index )
    {
        eqPresets->blockSignals( true );
        eqPresets->setCurrentIndex( index );
        eqPresets->blockSignals( false );
    }
}

void
EqualizerDialog::bandsChanged() //SLOT
{
    updateToolTips();
    updateLabels();
    updateUi();
    // The::engineController()->equalizerController()->blockSignals( true );
    The::engineController()->equalizerController()->setGains( gains() );
    // The::engineController()->equalizerController()->blockSignals( false );
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
EqualizerDialog::presetsChanged( const QString &name )
{
    Q_UNUSED( name )
    updatePresets();
    if( EqualizerPresets::eqGlobalList().indexOf( selectedPresetName() ) == -1 )
        presetApplied( 0 );
    updateUi();
}

void
EqualizerDialog::savePreset() //SLOT
{
    The::engineController()->equalizerController()->savePreset( selectedPresetName(), gains() );
}

void
EqualizerDialog::deletePreset() //SLOT
{
    The::engineController()->equalizerController()->deletePreset( selectedPresetName() );
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
    The::engineController()->equalizerController()->setGains( EqualizerPresets::eqCfgGetPresetVal( selectedPresetName() ) );
}

void
EqualizerDialog::updateToolTips()
{
    for( QSlider* mSlider : m_bands )
        mSlider->setToolTip( QString::number( mSlider->value()*mValueScale/100.0, 'f', 1 ) );
}

void
EqualizerDialog::updateLabels()
{
    for( int i = 0; i < m_bandValues.count() && i < m_bands.count(); i++ )
        m_bandValues[i]->setText( QString::number( m_bands[i]->value() * mValueScale / 100.0, 'f', 1 ) );
}

void
EqualizerDialog::toggleEqualizer( bool enabled )
{
    DEBUG_BLOCK

    EqualizerController *eq = The::engineController()->equalizerController();
    if( !enabled )
        eq->applyEqualizerPresetByIndex( -1 );
    else
        eq->applyEqualizerPresetByIndex( eqPresets->currentIndex() );
}

