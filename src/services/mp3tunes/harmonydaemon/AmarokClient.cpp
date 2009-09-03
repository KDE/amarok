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

#include "AmarokClient.h"

#include <QDBusMessage>
#include <QDBusConnection>
#include <QtDebug>

Mp3tunesAmarokClient::Mp3tunesAmarokClient() : Mp3tunesHarmonyClient()
{}

void Mp3tunesAmarokClient::dbusEmitMessage( const QString &message, const QString &param = QString() )
{
    QString name = "org.kde.amarok";
    QDBusMessage m = QDBusMessage::createMethodCall( name,
                                               "/Mp3tunesHarmonyHandler",
                                               "",
                                               message );
    if( !param.isEmpty() )
    {
      QList<QVariant> args;
      args.append( param );
      m.setArguments(args);
    }
    QDBusMessage response = QDBusConnection::sessionBus().call( m );
    if( response.type() == QDBusMessage::ErrorMessage )
    {
            qDebug() << "Got ERROR response " << message;
            qDebug() << response.errorName() << ":  " << response.errorMessage();
    }
}


/* SLOTS */
void
Mp3tunesAmarokClient::harmonyError( const QString &error )
{
    qDebug() << "Received Error: " << error;
}

void
Mp3tunesAmarokClient::harmonyWaitingForEmail( const QString &pin )
{
    qDebug() << "Received HARMONY_WAITING_FOR_EMAIL " << pin;
    dbusEmitMessage( "emitWaitingForEmail", pin );
}

void
Mp3tunesAmarokClient::harmonyWaitingForPin()
{
    qDebug() << "Received HARMONY_WAITING_FOR_PIN";
    dbusEmitMessage( "emitWaitingForPin" );
}

void
Mp3tunesAmarokClient::harmonyConnected()
{
    qDebug() << "Apparently Harmony is connected";
    dbusEmitMessage( "emitConnected" );
}

void
Mp3tunesAmarokClient::harmonyDisconnected()
{
    qDebug() << "Harmony said see ya later.";
    dbusEmitMessage( "emitDisconnected" );
}

void
Mp3tunesAmarokClient::harmonyDownloadReady( const Mp3tunesHarmonyDownload &download )
{
    qDebug() << "Got message about ready: " << download.trackTitle() << " by " << download.artistName() << " on " << download. albumTitle();
    QString name = "org.kde.amarok";
    QDBusMessage m = QDBusMessage::createMethodCall( name,
                                               "/Mp3tunesHarmonyHandler",
                                               "",
                                               "emitDownloadReady" );
    QList<QVariant> args;
    args.append( download.serialize() );
    m.setArguments(args);
    QDBusMessage response = QDBusConnection::sessionBus().call( m );
    if( response.type() == QDBusMessage::ErrorMessage )
    {
            qDebug() << "Got ERROR response harmonyDownloadReady";
            qDebug() << response.errorName() << ":  " << response.errorMessage();
    }
}

void
Mp3tunesAmarokClient::harmonyDownloadPending( const Mp3tunesHarmonyDownload &download )
{
    qDebug() << "Got message about pending: " << download.trackTitle() << " by " << download.artistName() << " on " << download. albumTitle();
    qDebug() << "Got message about ready: " << download.trackTitle() << " by " << download.artistName() << " on " << download. albumTitle();
    QString name = "org.kde.amarok";
    QDBusMessage m = QDBusMessage::createMethodCall( name,
                                               "/Mp3tunesHarmonyHandler",
                                               "",
                                               "emitDownloadPending" );
    QList<QVariant> args;
    args.append( download.serialize() );
    m.setArguments(args);
    QDBusMessage response = QDBusConnection::sessionBus().call( m );
    if( response.type() == QDBusMessage::ErrorMessage )
    {
            qDebug() << "Got ERROR response harmonyDownloadPending";
            qDebug() << response.errorName() << ":  " << response.errorMessage();
    }
}
