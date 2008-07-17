/***************************************************************************
 *   Copyright (c) 2008  Casey Link <unnamedrambler@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "Mp3tunesHarmonyHandler.h"
#include "mp3tunesharmonyhandleradaptor.h"
#include "App.h"
#include "Debug.h"

#include <QList>
#include <QVariant>

Mp3tunesHarmonyHandler::Mp3tunesHarmonyHandler( QString identifier,
                                                QString email = QString(),
                                                QString pin = QString() )
    : QObject( kapp )
    , m_daemon( 0 )
    , m_identifier( identifier )
    , m_email( email )
    , m_pin( pin )
{
    new Mp3tunesHarmonyHandlerAdaptor( this );
    QDBusConnection::sessionBus().registerObject("/Mp3tunesHarmonyHandler", this);
}

void Mp3tunesHarmonyHandler::startDaemon()
{
    m_daemon = new AmarokProcess( this );
    if( m_email.isEmpty() && m_pin.isEmpty() )
        *m_daemon << "amarokmp3tunesharmonydaemon" << m_identifier;
    else if( !m_email.isEmpty() && !m_pin.isEmpty() )
        *m_daemon << "amarokmp3tunesharmonydaemon" << m_identifier << m_email << m_pin;
    m_daemon->setOutputChannelMode( KProcess::OnlyStdoutChannel );
    connect( m_daemon, SIGNAL( finished( int ) ), SLOT( slotFinished(  ) ) );
    connect( m_daemon, SIGNAL( error( QProcess::ProcessError ) ), SLOT( slotError( QProcess::ProcessError ) ) );
    m_daemon->start();
}

void
Mp3tunesHarmonyHandler::slotFinished( )
{
    m_daemon->deleteLater();
    m_daemon = 0;
}

void
Mp3tunesHarmonyHandler::slotError( QProcess::ProcessError error )
{
    if( error == QProcess::Crashed )
    {
        //handleRestart();
    }
}

bool Mp3tunesHarmonyHandler::daemonRunning()
{
    if( !m_daemon )
        return false;
    debug() << "Daemon process is running";
}

bool Mp3tunesHarmonyHandler::daemonConnected()
{
    DEBUG_BLOCK
    if( !daemonRunning() )
        return false;
    QString name = "org.kde.amarok.Mp3tunesHarmonyDaemon-" + QString::number( m_daemon->pid() );
    debug() << "Making Dbus call about daemonConnected";
    QDBusMessage m = QDBusMessage::createMethodCall( name,
                                               "/Mp3tunesHarmonyDaemon",
                                               "",
                                               "daemonConnected" );
    QDBusMessage response = QDBusConnection::sessionBus().call( m );
    QList<QVariant> args = response.arguments();
    debug() << "Got response  #args: " << args.count();
    int i = 0;
    foreach( QVariant q, args )
    {
        debug() << QString::number(i) << ": " << q.toString();
        i++;
    }
}

void Mp3tunesHarmonyHandler::makeConnection()
{
}

void Mp3tunesHarmonyHandler::breakConnection()
{
}

void
Mp3tunesHarmonyHandler::emitError( const QString &error )
{
   emit( errorSignal( error ) );
}

void
Mp3tunesHarmonyHandler::emitWaitingForEmail()
{
    emit( waitingForEmail() );
}

void
Mp3tunesHarmonyHandler::emitWaitingForPin()
{
    emit( waitingForPin() );
}

void
Mp3tunesHarmonyHandler::emitConnected()
{
    emit( connected() );
}

void
Mp3tunesHarmonyHandler::emitDisconnected()
{
    emit( disconnected() );
}

void
Mp3tunesHarmonyHandler::emitDownloadReady( const Mp3tunesHarmonyDownload &download )
{
    emit( downloadReady( download ) );
}

void
Mp3tunesHarmonyHandler::emitDownloadPending( const Mp3tunesHarmonyDownload &download )
{
    emit( downloadPending( download ) );
}