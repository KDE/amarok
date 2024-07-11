/****************************************************************************************
 * Copyright (c) 2002 Mark Kretschmann <kretschmann@kde.org>                            *
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

#ifndef AMAROK_APP_H
#define AMAROK_APP_H

#include <config.h> // for HAVE_LIBLASTFM

#include "amarok_export.h"

#include <KJob>

#include <QApplication>
#include <QPointer>

#ifdef HAVE_LIBLASTFM
#include <ws.h>
#endif

namespace Amarok {
    class TrayIcon;
}

namespace ScriptConsoleNS{
    class ScriptConsole;
}

namespace KIO {
    class Job;
}

class MainWindow;
class QCommandLineParser;
class QUrl;

class AMAROK_EXPORT App : public QApplication
{
    Q_OBJECT

    public:
        App(int &argc, char **argv);
        ~App() override;

        static App *instance() { return static_cast<App*>( qApp ); }

        void continueInit();
        Amarok::TrayIcon* trayIcon() const { return m_tray; }
        void handleCliArgs(const QString &cwd);
        void initCliArgs(QCommandLineParser *parsers);

        virtual int newInstance();

        inline QPointer<MainWindow> mainWindow() const { return m_mainWindow; }

    Q_SIGNALS:
        void prepareToQuit();
        void settingsChanged();

    public Q_SLOTS:
        void activateRequested(const QStringList &  arguments, const QString & cwd);
        void applySettings();
        void applySettingsFirstTime();
        void slotConfigAmarok( const QString& page = QString() );
        void slotConfigAmarokWithEmptyPage();
        void slotConfigShortcuts();
        KIO::Job *trashFiles( const QList<QUrl> &files );
        void quit();

    protected:
        bool event( QEvent *event ) override;

    private Q_SLOTS:
        void slotTrashResult( KJob *job );
#ifdef HAVE_LIBLASTFM
        void onWsError( lastfm::ws::Error e );
#endif

    private:
        void handleFirstRun();

        // ATTRIBUTES
        QPointer<MainWindow>        m_mainWindow;
        Amarok::TrayIcon            *m_tray;
        QPointer<ScriptConsoleNS::ScriptConsole> m_scriptConsole;
        QCommandLineParser          *m_args;
        QString                     m_cwd;
        QStringList                 s_delayedAmarokUrls;
};

#define pApp App::instance()


#endif  // AMAROK_APP_H
