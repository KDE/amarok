/***************************************************************************
begin                : 2004/02/07
copyright            : (C) Mark Kretschmann
email                : markey@web.de
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "Options1.h"
#include "Options2.h"
#include "Options3.h"
#include "Options4.h"
#include "Options5.h"
#include "amarokconfig.h"
#include "app.h"
#include "configdialog.h"
#include "enginecontroller.h"
#include "pluginmanager.h"

#include <qlineedit.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qcheckbox.h>

#include <kconfigdialog.h>
#include <kdebug.h>
#include <klocale.h>


AmarokConfigDialog::AmarokConfigDialog( QWidget *parent, const char* name, KConfigSkeleton *config )
        : KConfigDialog( parent, name, config )
{
    //we must manage some widgets manually, since KConfigDialogManager can't
    //handle dynamic parameters (at least I don't know how to do it)

    m_opt4 = new Options4( 0, "Playback" );
    m_pSoundSystem = m_opt4->sound_system;
    m_pSoundOutput = m_opt4->sound_output;
    m_pSoundDevice = m_opt4->sound_device;
    m_pCustomSoundDevice = m_opt4->kcfg_CustomSoundDevice;

    // Sound System
    QStringList systems;
    KTrader::OfferList offers = PluginManager::query( "[X-KDE-amaroK-plugintype] == 'engine'" );

    for ( KTrader::OfferList::ConstIterator it = offers.begin(); it != offers.end(); ++it )
        systems << ( *it ) ->name();

    m_pSoundSystem->insertStringList( systems );

    connect( m_pSoundSystem, SIGNAL( activated( int ) ), this, SLOT( settingsChangedSlot() ) );
    connect( m_pSoundOutput, SIGNAL( activated( int ) ), this, SLOT( settingsChangedSlot() ) );
    connect( m_pSoundDevice, SIGNAL( textChanged( const QString& ) ), this, SLOT( settingsChangedSlot() ) );
    connect( m_pCustomSoundDevice, SIGNAL( toggled( bool ) ), this, SLOT( settingsChangedSlot() ) );

    connect( m_pSoundSystem, SIGNAL( activated( int ) ), this, SLOT( soundSystemChanged() ) );
    connect( m_pCustomSoundDevice, SIGNAL( toggled( bool ) ), this, SLOT( soundSystemChanged() ) );

    // add pages
    addPage( new Options1( 0, "General" ), i18n( "General" ), "misc", i18n( "Configure General Options" ) );
    addPage( new Options2( 0, "Fonts" ), i18n( "Fonts" ), "fonts", i18n( "Configure Fonts" ) );
    addPage( new Options3( 0, "Colors" ), i18n( "Colors" ), "colors", i18n( "Configure Colors" ) );
    addPage( m_opt4, i18n( "Playback" ), "kmix", i18n( "Configure Playback" ) );
    addPage( new Options5( 0, "OSD" ), i18n( "OSD" ), "tv", i18n( "Configure On-Screen-Display" ) );

    setInitialSize( minimumSizeHint() );
}

//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

/**
 * Update the settings from the dialog.
 * Example use: User clicks Ok or Apply button in a configure dialog.
 * REIMPLEMENTED
 */
void AmarokConfigDialog::updateSettings()
{
    kdDebug() << k_funcinfo << endl;
    
    AmarokConfig::setSoundSystem( m_pSoundSystem->currentText() );
    if ( !m_pSoundOutput->currentText().isEmpty() )
        AmarokConfig::setSoundOutput( m_pSoundOutput->currentText() );
    AmarokConfig::setSoundDevice( m_pSoundDevice->text() );

    emit settingsChanged();
    updateWidgets();
}


/**
 * Update the dialog based on the settings.
 * Example use: Initialisation of dialog.
 * Example use: User clicks Reset button in a configure dialog.
 * REIMPLEMENTED
 */
void AmarokConfigDialog::updateWidgets()
{
    kdDebug() << k_funcinfo << endl;
    
    m_pSoundSystem->setCurrentText( AmarokConfig::soundSystem() );
    m_pSoundDevice->setText( AmarokConfig::soundDevice() );
    
    soundSystemChanged();
}


/**
 * Update the dialog based on the default settings.
 * Example use: User clicks Defaults button in a configure dialog.
 * REIMPLEMENTED
 */
void AmarokConfigDialog::updateWidgetsDefault()
{
    kdDebug() << k_funcinfo << endl;
    
    m_pSoundSystem->setCurrentText( "aRts Engine" );
    soundSystemChanged();
}


void AmarokConfigDialog::soundSystemChanged()
{
    // Update Sound Output Combo
    m_pSoundOutput->clear();
    QStringList outputs = EngineController::engine()->getOutputsList();

    if ( outputs.isEmpty() ) {
        m_pSoundOutput->setEnabled( false );
        m_opt4->outputLabel->setEnabled( false );
        m_pCustomSoundDevice->setEnabled( false ); //will toggle the device lineEdit
        m_pSoundDevice->setEnabled( false );

    } else {

        m_pSoundOutput->setEnabled( true );
        m_opt4->outputLabel->setEnabled( true );
        m_pCustomSoundDevice->setEnabled( true );
        m_pSoundDevice->setEnabled( m_pCustomSoundDevice->isChecked() );

        m_pSoundOutput->insertStringList( outputs );

        /**
         * Find index of current item, but only if the selected system
         * is the current one (otherwise it doesn't make much sense).
         */
        if ( m_pSoundSystem->currentText() == AmarokConfig::soundSystem() )
            for ( uint i = 0; i < outputs.count(); i++ )
                if ( outputs[ i ] == AmarokConfig::soundOutput() ) {
                    m_pSoundOutput->setCurrentItem( i );
                    break;
                }
    }

    updateButtons();
}


//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE
//////////////////////////////////////////////////////////////////////////////////////////

/** REIMPLEMENTED */
bool AmarokConfigDialog::hasChanged()
{
    kdDebug() << k_funcinfo << endl;
    
    bool changed =
        m_pSoundSystem->currentText()            != AmarokConfig::soundSystem();
    
    if ( m_pSoundOutput->isEnabled() )  
        changed |= m_pSoundOutput->currentText() != AmarokConfig::soundOutput();
    
    if ( m_pSoundDevice->isEnabled() )  
        changed |= m_pSoundDevice->text()        != AmarokConfig::soundDevice();

    return changed;
}


/** REIMPLEMENTED */
bool AmarokConfigDialog::isDefault()
{
    kdDebug() << k_funcinfo << endl;
    
    return ( m_pSoundSystem->currentText() == "aRts Engine" );
}


#include "configdialog.moc"
