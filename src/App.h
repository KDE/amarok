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

#include "MainWindow.h"
#include "amarok_export.h"
#include <config.h>
#include "aboutdialog/OcsData.h"

#include <KAboutData>
#include <KUniqueApplication>   //baseclass
#include <KUrl>

#include <QHash>
#include <QWeakPointer>
#include <QString>

namespace Amarok {
    class TrayIcon;
}

namespace ScriptConsoleNS{
    class ScriptConsole;
}

class OcsData;

namespace KIO { class Job; }

class KJob;
class MediaDeviceManager;

class AMAROK_EXPORT App : public KUniqueApplication
{
    Q_OBJECT

    public:
        App();
       ~App();

        static App *instance() { return static_cast<App*>( kapp ); }

        void setUniqueInstance( bool isUnique ) { m_isUniqueInstance = isUnique; }
        bool isNonUniqueInstance() const { return m_isUniqueInstance; }

        Amarok::TrayIcon* trayIcon() const { return m_tray; }
        static void handleCliArgs();
        static void initCliArgs();

        virtual int newInstance();

        inline MainWindow *mainWindow() const { return m_mainWindow.data(); }

        // FRIENDS
        friend class MainWindow; //requires access to applySettings()

    signals:
        void prepareToQuit();
        void settingsChanged();

    private slots:
        void continueInit();

    public slots:
        void applySettings( bool firstTime = false );
        void slotConfigAmarok( const QString& page = QString() );
        void slotConfigShortcuts();
        KIO::Job *trashFiles( const KUrl::List &files );
        void quit();

    protected:
        virtual bool event( QEvent *event );

    private slots:
        void slotTrashResult( KJob *job );

    private:
        void handleFirstRun();

        // ATTRIBUTES
        bool                        m_isUniqueInstance;
        QWeakPointer<MainWindow>    m_mainWindow;
        Amarok::TrayIcon            *m_tray;
        MediaDeviceManager          *m_mediaDeviceManager;
        QWeakPointer<ScriptConsoleNS::ScriptConsole> m_scriptConsole;

        static QStringList       s_delayedAmarokUrls;
};

#define pApp static_cast<App*>(kapp)


#endif  // AMAROK_APP_H
