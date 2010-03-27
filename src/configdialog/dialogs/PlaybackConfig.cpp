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

#include "PlaybackConfig.h"

#include "amarokconfig.h"
#include "Amarok.h"
#include "ActionClasses.h"
#include "EngineController.h"
#include "core/support/Debug.h"

#include <KCMultiDialog>
#include <kmessagebox.h>


PlaybackConfig::PlaybackConfig( QWidget* parent )
    : ConfigDialogBase( parent )
{
    setupUi( this );
    kcfg_FadeoutOnExit->setHidden( true );

    connect( findChild<QPushButton*>( "pushButtonPhonon" ), SIGNAL( clicked() ), SLOT( configurePhonon() ) );
}

PlaybackConfig::~PlaybackConfig()
{}


///////////////////////////////////////////////////////////////
// REIMPLEMENTED METHODS from ConfigDialogBase
///////////////////////////////////////////////////////////////

bool
PlaybackConfig::hasChanged()
{
    return false;
}

bool
PlaybackConfig::isDefault()
{
    return false;
}

void
PlaybackConfig::updateSettings()
{}


///////////////////////////////////////////////////////////////
// PRIVATE METHODS 
///////////////////////////////////////////////////////////////

void
PlaybackConfig::configurePhonon() //SLOT
{
    DEBUG_BLOCK

    KCMultiDialog KCM;

    KCM.setWindowTitle( i18n( "Sound System - Amarok" ) );
    KCM.addModule( "kcm_phonon" );
    KCM.exec();
}

#include "PlaybackConfig.moc"
