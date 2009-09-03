/****************************************************************************************
 * Copyright (c) 2008 Casey Link <unnamedrambler@gmail.com>                             *
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

#include "Mp3tunesHarmonyClient.h"
#include <QtDebug>
Mp3tunesHarmonyClient::Mp3tunesHarmonyClient()
{}

/* SLOTS */
void
Mp3tunesHarmonyClient::harmonyError( const QString &error )
{
    qDebug() << "Received Error: " << error;
}

void
Mp3tunesHarmonyClient::harmonyWaitingForEmail( const QString &pin )
{
    qDebug() << "Received HARMONY_WAITING_FOR_EMAIL " << pin;
}

void
Mp3tunesHarmonyClient::harmonyWaitingForPin()
{
    qDebug() << "Received HARMONY_WAITING_FOR_PIN";
}

void
Mp3tunesHarmonyClient::harmonyConnected()
{
    qDebug() << "Apparently Harmony is connected";
}

void
Mp3tunesHarmonyClient::harmonyDisconnected()
{
    qDebug() << "Harmony said see ya later.";
}

void
Mp3tunesHarmonyClient::harmonyDownloadReady( const Mp3tunesHarmonyDownload &download )
{
    qDebug() << "Got message about ready: " << download.trackTitle() << " by " << download.artistName() << " on " << download. albumTitle();
}

void
Mp3tunesHarmonyClient::harmonyDownloadPending( const Mp3tunesHarmonyDownload &download )
{
    qDebug() << "Got message about pending: " << download.trackTitle() << " by " << download.artistName() << " on " << download. albumTitle();
}
