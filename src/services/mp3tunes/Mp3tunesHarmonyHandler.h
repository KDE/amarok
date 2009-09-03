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

#ifndef MP3TUNESHARMONYHANDLER_H
#define MP3TUNESHARMONYHANDLER_H

#include "harmonydaemon/Mp3tunesHarmonyDownload.h"
#include "AmarokProcess.h"

#include <QObject>
#include <QString>
#include <QMap>


class Mp3tunesHarmonyHandler : public QObject {
    Q_OBJECT
    Q_CLASSINFO("Amarok Harmony D-Bus Interface", "org.kde.amarok.Mp3tunesHarmonyHandler")
    public:
        explicit Mp3tunesHarmonyHandler( QString identifier,
                                         QString email = QString(),
                                         QString pin  = QString() );
        ~Mp3tunesHarmonyHandler();

        bool startDaemon();
        void stopDaemon();
        bool daemonRunning();
        bool daemonConnected();
        void makeConnection();
        void breakConnection();
        QString pin();
        QString email();

    signals:
      void waitingForEmail( const QString &pin );
      void waitingForPin();
      void connected();
      void disconnected();
      void signalError( const QString &error );
      void downloadReady( const QVariantMap &download );
      void downloadPending( const QVariantMap &download );

    public slots:
        virtual void emitError( const QString &error );
        virtual void emitWaitingForEmail( const QString &pin );
        virtual void emitWaitingForPin();
        virtual void emitConnected();
        virtual void emitDisconnected();
        virtual void emitDownloadReady( const QVariantMap &download  );
        virtual void emitDownloadPending( const QVariantMap &download  );

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
