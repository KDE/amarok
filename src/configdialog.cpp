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
#include "configdialog.h"
#include "enginecontroller.h"
#include "osd.h"
#include "plugin/pluginconfig.h"
#include "pluginmanager.h"

#include <qcombobox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qvbox.h>

#include <kdebug.h>
#include <klocale.h>
#include <kiconloader.h>

//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC
//////////////////////////////////////////////////////////////////////////////////////////

AmarokConfigDialog::AmarokConfigDialog( QWidget *parent, const char* name, KConfigSkeleton *config )
        : KConfigDialog( parent, name, config )
        , m_engineConfig( 0 )
        , m_enginePage( 0 )
{
    Options4 *opt4 = new Options4( 0, "Playback" );
    Options5 *opt5 = new Options5( 0, "OSD" );

    //TODO find out when KConfig XT can handle QComboBoxes
    m_pSoundSystem = opt4->sound_system;

    // Sound System
    QStringList systems;
    KTrader::OfferList offers = PluginManager::query( "[X-KDE-amaroK-plugintype] == 'engine'" );

    for ( KTrader::OfferList::ConstIterator it = offers.begin(); it != offers.end(); ++it )
        systems << ( *it )->property( "X-KDE-amaroK-name" ).toString();

    m_pSoundSystem->insertStringList( systems );

    // add pages
    addPage( new Options1( 0, "General" ), i18n( "General" ), "misc", i18n( "Configure General Options" ) );
    addPage( new Options2( 0, "Fonts" ), i18n( "Fonts" ), "fonts", i18n( "Configure Fonts" ) );
    addPage( new Options3( 0, "Colors" ), i18n( "Colors" ), "colors", i18n( "Configure Colors" ) );
    addPage( opt4, i18n( "Playback" ), "kmix", i18n( "Configure Playback" ) );
    addPage( opt5, i18n( "OSD" ), "tv", i18n( "Configure On-Screen-Display" ) );
    
    connect( m_pSoundSystem, SIGNAL( activated( int ) ), SLOT( settingsChangedSlot() ) );
    connect( opt4->pushButton_aboutEngine, SIGNAL( clicked() ), this, SLOT( aboutEngine() ) );
    connect( opt5, SIGNAL( settingsChanged() ), SLOT( settingsChangedSlot() ) ); //see options5.ui.h
}


//////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

/**
 * Update the settings from the dialog.
 * Example use: User clicks Ok or Apply button in a configure dialog.
 * REIMPLEMENTED
 */
void AmarokConfigDialog::updateSettings()
{
    AmarokConfig::setSoundSystem( m_pSoundSystem->currentText() );
    
    OSDWidget *osd = (OSDWidget*)child( "osdpreview" );
    AmarokConfig::setOsdAlignment( osd->alignment() );
    AmarokConfig::setOsdYOffset( osd->y() );
    if ( m_engineConfig ) m_engineConfig->save();        
    
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
    m_pSoundSystem->setCurrentText( AmarokConfig::soundSystem() );
    
    soundSystemChanged();
}


/**
 * Update the dialog based on the default settings.
 * Example use: User clicks Defaults button in a configure dialog.
 * REIMPLEMENTED
 */
void AmarokConfigDialog::updateWidgetsDefault()
{
    m_pSoundSystem->setCurrentItem( 0 );
    
    soundSystemChanged();
}


//////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED
//////////////////////////////////////////////////////////////////////////////////////////

/** REIMPLEMENTED */
bool AmarokConfigDialog::hasChanged()
{
    kdDebug() << k_funcinfo << endl;
    
    OSDWidget *osd = (OSDWidget*) child( "osdpreview" );
 
    bool engineChanged = false;
    if ( m_engineConfig ) engineChanged = m_engineConfig->hasChanged();
               
    return  m_pSoundSystem->currentText() != AmarokConfig::soundSystem() ||
            osd->alignment()              != AmarokConfig::osdAlignment() ||
            osd->y()                      != AmarokConfig::osdYOffset() ||
            engineChanged;
}


/** REIMPLEMENTED */
bool AmarokConfigDialog::isDefault()
{
    return false;
}


//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void AmarokConfigDialog::aboutEngine() //SLOT
{
    PluginManager::showAbout( QString( "[X-KDE-amaroK-name] == '%1'" ).arg( m_pSoundSystem->currentText() ) );
}


//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE
//////////////////////////////////////////////////////////////////////////////////////////

void AmarokConfigDialog::soundSystemChanged()
{
    // Remove old engine config page
    delete m_enginePage;
    m_enginePage = 0;
    delete m_engineConfig;
    m_engineConfig = 0;
    
    if( EngineController::engine()->hasConfigure() )
    {
        m_enginePage = addVBoxPage( i18n( "Engine" ), 
                                    i18n( "Configure " ) + PluginManager::getService( EngineController::engine() )->name(),
                                    DesktopIcon( "amarok" ) );
        
        m_engineConfig = EngineController::engine()->configure();
        m_engineConfig->view()->reparent( m_enginePage, QPoint() );                       
        
        connect( m_engineConfig, SIGNAL( settingsChanged() ), this, SLOT( settingsChangedSlot() ) );
    }
}

#include "configdialog.moc"
