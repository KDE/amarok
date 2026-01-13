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
#include "Debug.h"
#include "EngineController.h"

#include <QAction>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusPendingReply>
#include <QDBusUnixFileDescriptor>

#include <KLocalizedString>

const static QString s_login1Service = QStringLiteral("org.freedesktop.login1");
const static QString s_login1Path = QStringLiteral("/org/freedesktop/login1");
const static QString s_login1ManagerInterface = QStringLiteral("org.freedesktop.login1.Manager");

PowerManager::PowerManager( EngineController *engine )
    : QObject( engine )
    , m_inhibitionFD()
{
    connect( engine, &EngineController::stopped, this, &PowerManager::slotNotPlaying );
    connect( engine, &EngineController::paused, this, &PowerManager::slotNotPlaying );
    connect( engine, &EngineController::trackPlaying, this, &PowerManager::slotPlaying );
    connect( pApp, &App::settingsChanged, this, &PowerManager::slotSettingsChanged );

    QDBusConnection::systemBus().connect( s_login1Service,
                                          s_login1Path,
                                          s_login1ManagerInterface,
                                          QStringLiteral("PrepareForSleep"),
                                          this, SLOT( slotHandleSuspend() ) );
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
PowerManager::slotHandleSuspend()
{
    DEBUG_BLOCK

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
    if( m_inhibitionFD.isValid())
        return; // Already inhibiting

    QDBusMessage message = QDBusMessage::createMethodCall(s_login1Service, s_login1Path, s_login1ManagerInterface, QStringLiteral("Inhibit"));
    message.setArguments(QVariantList(
        {QStringLiteral("sleep"), QStringLiteral("Amarok"),
            i18nc("Reason for sleep block, shown in power and battery applet", "Playing music"), QStringLiteral("block")}));
    QDBusPendingReply<QDBusUnixFileDescriptor> reply = QDBusConnection::systemBus().asyncCall(message, 1000);
    QDBusPendingCallWatcher *callWatcher  = new QDBusPendingCallWatcher(reply, this);
    connect(callWatcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *callWatcher) {
        callWatcher->deleteLater();
        QDBusPendingReply<QDBusUnixFileDescriptor> reply = *callWatcher;
        if (reply.isValid()) {
            reply.value().swap(m_inhibitionFD);
        }
    });
}

void
PowerManager::stopInhibitingSuspend()
{
    if (!m_inhibitionFD.isValid())
        return;

    m_inhibitionFD = QDBusUnixFileDescriptor{};
}
