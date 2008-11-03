/***************************************************************************
                         app.h  -  description
                            -------------------
   begin                : Mit Okt 23 14:35:18 CEST 2002
   copyright            : (C) 2002 by Mark Kretschmann
   email                : markey@web.de
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_APP_H
#define AMAROK_APP_H

#include <config-amarok.h>  

#include "amarok_export.h"
#include <KUniqueApplication>   //baseclass
#include <KUrl>

#include <QByteArray>
#include <QHash>
#include <QString>

namespace Amarok {
    class TrayIcon;
}


namespace KIO { class Job; }

class KJob;
class MainWindow;
class MediaDeviceManager;
class KSplashScreen;

class AMAROK_EXPORT App : public KUniqueApplication
{
    Q_OBJECT

    public:
        App();
       ~App();

        static App *instance() { return static_cast<App*>( kapp ); }

        static void handleCliArgs();
        static void initCliArgs( int argc, char *argv[] );
        static void initCliArgs();

        static int mainThreadId;

        virtual int newInstance();

        inline MainWindow *mainWindow() const { return m_mainWindow; }

        // FRIENDS ------
        friend class MainWindow; //requires access to applySettings()

    signals:
        void prepareToQuit();

    private slots:
        void continueInit();

    public slots:
        void applySettings( bool firstTime = false );
        void slotConfigAmarok( const QString& page = QString() );
        void slotConfigShortcuts();
        void slotConfigToolBars();
        void slotConfigEqualizer();
        KIO::Job *trashFiles( const KUrl::List &files );
        void quit();

    private slots:
        void slotTrashResult( KJob *job );

    private:
        // ATTRIBUTES ------
        MainWindow          *m_mainWindow;
        Amarok::TrayIcon    *m_tray;
        MediaDeviceManager  *m_mediaDeviceManager;
        KSplashScreen       *m_splash;
};

#define pApp static_cast<App*>(kapp)


#endif  // AMAROK_APP_H
