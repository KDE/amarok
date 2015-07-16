/****************************************************************************************
 * Copyright (c) 2013 Anmol Ahuja <darthcodus@gmail.com>                                *
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

#define DEBUG_PREFIX "PowerManager"

#include "PowerManager.h"

#include "amarokconfig.h"
#include "App.h"
#include "EngineController.h"

#include <Solid/PowerManagement>
#include <KLocalizedString>
#include <QAction>

PowerManager::PowerManager( EngineController *engine )
    : QObject( engine )
    , m_inhibitionCookie( -1 )
{
    connect( engine, SIGNAL(stopped(qint64,qint64)), this, SLOT(slotNotPlaying()) );
    connect( engine, SIGNAL(paused()), this, SLOT(slotNotPlaying()) );
    connect( engine, SIGNAL(trackPlaying(Meta::TrackPtr)), this, SLOT(slotPlaying()) );
    connect( App::instance(), SIGNAL(settingsChanged()), SLOT(slotSettingsChanged()) );
    connect( Solid::PowerManagement::notifier(), SIGNAL(resumingFromSuspend()),
            this, SLOT(slotResumingFromSuspend()) );
}

PowerManager::~PowerManager()
{
    stopInhibitingSuspend();
}

void
PowerManager::slotNotPlaying()
{
    stopInhibitingSuspend();
}

void
PowerManager::slotPlaying()
{
    if( AmarokConfig::inhibitSuspend() )
        startInhibitingSuspend();
}

void
PowerManager::slotResumingFromSuspend()
{
    if( AmarokConfig::pauseOnSuspend() && The::engineController()->isPlaying() )
        The::engineController()->playPause();
}

void
PowerManager::slotSettingsChanged()
{
    if( AmarokConfig::inhibitSuspend() && The::engineController()->isPlaying() )
        startInhibitingSuspend();
    else
        stopInhibitingSuspend();
}

void
PowerManager::startInhibitingSuspend()
{
    if( m_inhibitionCookie == -1 )
        m_inhibitionCookie = Solid::PowerManagement::beginSuppressingSleep( i18n( "Amarok is currently playing a track" ) );
}

void
PowerManager::stopInhibitingSuspend()
{
    if( m_inhibitionCookie != -1 )
    {
        Solid::PowerManagement::stopSuppressingSleep( m_inhibitionCookie );
        m_inhibitionCookie = -1;
    }
}
