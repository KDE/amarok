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

#include <qcombobox.h>
#include <qlabel.h>

#include <kconfigdialog.h>
#include <kdebug.h>
#include <klocale.h>


AmarokConfigDialog::AmarokConfigDialog( QWidget *parent, const char* name, KConfigSkeleton *config )
        : KConfigDialog( parent, name, config )
{
    //we must handle the "Sound Setting" QComboBox manually, because KConfigDialogManager can't
    //manage dynamic itemLists (at least I don't know how to do it)
    m_opt4 = new Options4( 0,"Playback" );
    m_pSoundSystem = m_opt4->sound_system;
    m_pSoundOutput = m_opt4->sound_output;
    
    // Sound System
    QStringList systems;
    KTrader::OfferList offers = PluginManager::query( "[X-KDE-amaroK-plugintype] == 'engine'" );

    for( KTrader::OfferList::ConstIterator it = offers.begin(); it != offers.end(); ++it )
        systems << (*it)->name();

    m_pSoundSystem->insertStringList( systems );
    m_pSoundSystem->setCurrentText  ( AmarokConfig::soundSystem() );
               
    connect( m_pSoundSystem, SIGNAL( activated( int ) ), this, SLOT( settingsChangedSlot() ) );
    connect( m_pSoundOutput, SIGNAL( activated( int ) ), this, SLOT( settingsChangedSlot() ) );

    // add pages
    addPage( new Options1( 0,"General" ),  i18n("General"),  "misc",   i18n("Configure General Options") );
    addPage( new Options2( 0,"Fonts" ),    i18n("Fonts"),    "fonts",  i18n("Configure Fonts") );
    addPage( new Options3( 0,"Colors" ),   i18n("Colors"),   "colors", i18n("Configure Colors") );
    addPage( m_opt4,                       i18n("Playback"), "kmix",   i18n("Configure Playback") );
    addPage( new Options5( 0,"OSD" ),      i18n("OSD" ),     "tv",     i18n("Configure On-Screen-Display") );

    setInitialSize( QSize( 440, 390 ) );
}


void AmarokConfigDialog::show()
{
    // Sound Output    
    m_pSoundOutput->clear();
    QStringList outputs = EngineController::engine()->getOutputsList();
    
    if ( outputs.isEmpty() ) {
        m_pSoundOutput->setEnabled( false );
        m_opt4->outputLabel->setEnabled( false );
    } else {
        m_pSoundOutput->setEnabled( true );
        m_opt4->outputLabel->setEnabled( true );
        m_pSoundOutput->insertStringList( outputs );
        
        //find index of current item
        for ( int i = 0; i < outputs.count(); i++ )
            if ( outputs[i] == AmarokConfig::soundOutput() ) {
                m_pSoundOutput->setCurrentItem( i );
                break;
            }
    }
    
    KConfigDialog::show();
}


bool AmarokConfigDialog::hasChanged()
{
    return ( m_pSoundSystem->currentText() != AmarokConfig::soundSystem() ) ||
           ( m_pSoundOutput->currentText() != AmarokConfig::soundOutput() );
}


bool AmarokConfigDialog::isDefault()
{
    return ( m_pSoundSystem->currentText() == "aRts Engine" ) &&
           ( m_pSoundOutput->currentText() == "Alsa" );   
}


void AmarokConfigDialog::updateSettings()
{
    AmarokConfig::setSoundSystem( m_pSoundSystem->currentText() );
    AmarokConfig::setSoundOutput( m_pSoundOutput->currentText() );
    emit settingsChanged();
}

#include "configdialog.moc"
