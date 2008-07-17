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

#ifndef MP3TUNESHARMONYHANDLER_H
#define MP3TUNESHARMONYHANDLER_H

#include "harmonydaemon/Mp3tunesHarmonyDownload.h"
#include "AmarokProcess.h"

#include <QObject>
#include <QString>

class Mp3tunesHarmonyHandler : public QObject {
    Q_OBJECT
    Q_CLASSINFO("Amarok Harmony D-Bus Interface", "org.kde.amarok.Mp3tunesHarmonyHandler")
    public:
        Mp3tunesHarmonyHandler( QString identifier, QString email, QString pin);

        void startDaemon();
        bool daemonRunning();
        bool daemonConnected();
        void makeConnection();
        void breakConnection();

    signals:
      void waitingForEmail();
      void waitingForPin();
      void connected();
      void disconnected();
      void errorSignal( const QString &error );
      void downloadReady( const Mp3tunesHarmonyDownload &download );
      void downloadPending( const Mp3tunesHarmonyDownload &download );

    public slots:
        virtual void emitError( const QString &error );
        virtual void emitWaitingForEmail();
        virtual void emitWaitingForPin();
        virtual void emitConnected();
        virtual void emitDisconnected();
        virtual void emitDownloadReady( const Mp3tunesHarmonyDownload &download );
        virtual void emitDownloadPending( const Mp3tunesHarmonyDownload &download );

    private slots:
        void slotFinished();
        void slotError( QProcess::ProcessError error );

    private:
        AmarokProcess *m_daemon;

        QString m_identifier;
        QString m_email;
        QString m_pin;
};
#endif
