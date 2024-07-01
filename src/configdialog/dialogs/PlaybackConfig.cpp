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
#include "core/support/Amarok.h"
#include "ActionClasses.h"
#include "EngineController.h"
#include "core/support/Debug.h"


PlaybackConfig::PlaybackConfig( Amarok2ConfigDialog* parent )
    : ConfigDialogBase( parent )
{
    setupUi( this );

    EngineController *engine = EngineController::instance();
    Q_ASSERT( engine );
    if( !engine->supportsFadeout() )
    {
        const QString toolTip = i18n( "Current Phonon backend does not support volume fading" );
        kcfg_FadeoutOnStop->setEnabled( false );
        kcfg_FadeoutOnStop->setToolTip( toolTip );
        kcfg_FadeoutOnPause->setEnabled( false );
        kcfg_FadeoutOnPause->setToolTip( toolTip );
        fadeoutLengthLabel->setEnabled( false );
        fadeoutLengthLabel->setToolTip( toolTip );
        kcfg_FadeoutLength->setEnabled( false );
        kcfg_FadeoutLength->setToolTip( toolTip );
    }

    connect( kcfg_FadeoutOnStop, &QCheckBox::toggled, this, &PlaybackConfig::setFadeoutState );
    connect( kcfg_FadeoutOnPause, &QCheckBox::toggled, this, &PlaybackConfig::setFadeoutState );
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
PlaybackConfig::setFadeoutState() //SLOT
{
    if( !EngineController::instance()->supportsFadeout() )
        return;

    const bool enabled = kcfg_FadeoutOnPause->isChecked() || kcfg_FadeoutOnStop->isChecked();

    fadeoutLengthLabel->setEnabled( enabled );
    kcfg_FadeoutLength->setEnabled( enabled );
}


