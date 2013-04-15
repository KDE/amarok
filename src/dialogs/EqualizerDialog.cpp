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

    connect(this, SIGNAL(cancelClicked()), this, SLOT(restoreOriginalSettings()));

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
    mValueScale = The::engineController()->eqMaxGain();
    QString mlblText = i18n( "%0\ndB" ).arg( QString::number( mValueScale, 'f', 1 ) );
    eqMaxEq->setText( QString("+") + mlblText );
    eqMinEq->setText( QString("-") + mlblText );

    // Ask engine for band frequencies and set labels
    QStringList equalizerBandFreq = The::engineController()->eqBandsFreq();
    QStringListIterator i( equalizerBandFreq );
    // Checking if preamp is present
    if( equalizerBandFreq.size() == s_equalizerBandsCount )
        eqPreampSlider->setDisabled( true );
    else if( i.hasNext() ) // If preamp is present then skip its label as it is hard-coded in UI
        i.next();
    foreach( QLabel* mLabel, m_bandLabels )
        if( mLabel->objectName() != "eqPreampLabel" )
            mLabel->setText( i.hasNext() ?  i.next() : "N/A" );

    updatePresets();
    activeCheckBox->setChecked( AmarokConfig::equalizerMode() > 0 );
    setPreset( AmarokConfig::equalizerMode() - 1 );
    setGains( AmarokConfig::equalizerGains() );

    // Configure signal and slots to handle presets
    connect( activeCheckBox, SIGNAL(toggled(bool)), SLOT(setActive(bool)) );
    connect( eqPresets, SIGNAL(currentIndexChanged(int)), SLOT(setPreset(int)) );
    connect( eqPresets, SIGNAL(editTextChanged(QString)), SLOT(updateUi()) );
    foreach( QSlider* mSlider, m_bands )
        connect( mSlider, SIGNAL(valueChanged(int)), SLOT(bandsChanged()) );

    eqPresetSaveBtn->setIcon( KIcon( "document-save" ) );
    connect( eqPresetSaveBtn, SIGNAL(clicked()), SLOT(savePreset()) );

    eqPresetDeleteBtn->setIcon( KIcon( "edit-delete" ) );
    connect( eqPresetDeleteBtn, SIGNAL(clicked()), SLOT(deletePreset()) );

    eqPresetResetBtn->setIcon( KIcon( "edit-undo" ) );
    connect( eqPresetResetBtn, SIGNAL(clicked()), SLOT(restorePreset()) );
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
    foreach( QSlider* mSlider, m_bands )
        result << mSlider->value();
    return result;
}

void
EqualizerDialog::setGains( QList<int> eqGains )
{
    for( int i = 0; i < m_bands.count() && i < eqGains.count(); i++ )
    {
        // Update slider values with signal blocking to prevent circular loop
        m_bands[i]->blockSignals( true );
        m_bands[i]->setValue( eqGains[ i ] );
        m_bands[i]->blockSignals( false );
    }

    bandsChanged();
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
    activeCheckBox->setChecked( m_originalActivated );
    int originalPresetIndex = EqualizerPresets::eqGlobalList().indexOf( m_originalPreset );
    setPreset( originalPresetIndex );
    eqPresets->setEditText( m_originalPreset );
    setGains( m_originalGains );
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
    foreach( QSlider* mSlider, m_bands )
        mSlider->setToolTip( QString::number( mSlider->value()*mValueScale/100.0, 'f', 1 ) );
}

void
EqualizerDialog::updateLabels()
{
    for( int i = 0; i < m_bandValues.count() && i < m_bands.count(); i++ )
        m_bandValues[i]->setText( QString::number( m_bands[i]->value() * mValueScale / 100.0, 'f', 1 ) );
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
