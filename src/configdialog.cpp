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

#include "amarokconfig.h"
#include "amarokconfigdialog.h"
#include "Options1.h"
#include "Options2.h"
#include "Options3.h"
#include "Options4.h"
#include "Options5.h"
#include "playerapp.h"
#include "pluginmanager.h"

#include <qcombobox.h>
#include <qradiobutton.h>

#include <kconfigdialog.h>
#include <klocale.h>


AmarokConfigDialog::AmarokConfigDialog( QWidget *parent, const char* name, KConfigSkeleton *config )
        : KConfigDialog( parent, name, config )
{
    //we must handle the "Sound Setting" QComboBox manually, because KConfigDialogManager can't
    //manage dynamic itemLists (at least I don't know how to do it)
    Options4* pOpt4 = new Options4( 0,"Playback" );
    m_pSoundSystem = pOpt4->sound_system;

    KTrader::OfferList offers = PluginManager::query( "[X-KDE-amaroK-plugintype] == 'engine'" );
    QStringList list;

    for( KTrader::OfferList::ConstIterator it = offers.begin(); it != offers.end(); ++it )
        list << (*it)->name();

    m_pSoundSystem->insertStringList( list );
    m_pSoundSystem->setCurrentText  ( AmarokConfig::soundSystem() );
   
    // add screens
    connect( m_pSoundSystem, SIGNAL( activated( int ) ), this, SLOT( settingsChangedSlot() ) );

    addPage( new Options1( 0,"General" ),  i18n("General"),  "misc",   i18n("Configure General Options") );
    addPage( new Options2( 0,"Fonts" ),    i18n("Fonts"),    "fonts",  i18n("Configure Fonts") );
    addPage( new Options3( 0,"Colors" ),   i18n("Colors"),   "colors", i18n("Configure Colors") );
    addPage( pOpt4,                        i18n("Playback"), "kmix",   i18n("Configure Playback") );
    addPage( new Options5( 0,"OSD" ),      i18n("OSD" ),     "tv",     i18n("Configure On-Screen-Display") );

    setInitialSize( QSize( 460, 390 ) );
}


bool AmarokConfigDialog::hasChanged()
{
    return ( m_pSoundSystem->currentText() != AmarokConfig::soundSystem() );
}


bool AmarokConfigDialog::isDefault()
{
    return ( m_pSoundSystem->currentText() == "arts" );
}


void AmarokConfigDialog::updateSettings()
{
    AmarokConfig::setSoundSystem( m_pSoundSystem->currentText() );
    emit settingsChanged();
}

#include "amarokconfigdialog.moc"
