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

#include "engineobserver.h" //baseclass
#include <kapplication.h>   //baseclass
#include <kurl.h>           //needed for KURL::List (nested)

class KActionCollection;
class KConfig;
class KGlobalAccel;
class QColor;
class QCString;
class QEvent;

namespace amaroK {
    class TrayIcon;
    class DcopHandler;
}

class MetaBundle;
class PlayerWidget;
class Playlist;
class PlaylistWindow;


class App : public KApplication, public EngineObserver
{
    Q_OBJECT

    public:
        App();
        ~App();

        static void handleCliArgs();
        static void initCliArgs( int argc, char *argv[] );

        amaroK::DcopHandler *dcopHandler() const { return m_pDcopHandler; }
        QWidget             *mainWindow() const;
        PlaylistWindow      *playlistWindow() const { return m_pPlaylistWindow; }

        // FRIENDS ------
        friend class PlaylistWindow; //requires access to applySettings()

    protected: /* for OSD, tray, and dcop */
        void engineStateChanged( Engine::State state );
        void engineNewMetaData( const MetaBundle &bundle, bool trackChanged );
        void engineVolumeChanged( int );

    public slots:
        void slotConfigAmarok();
        void slotConfigShortcuts();
        void slotConfigGlobalShortcuts();
        void slotConfigToolBars();
        void slotConfigEffects( bool = true );

    private slots:
        void applySettings( bool firstTime = false );

    private:
        void initGlobalShortcuts();
        void restoreSession();
        void setupColors();

        // ATTRIBUTES ------
        KGlobalAccel        *m_pGlobalAccel;
        PlayerWidget        *m_pPlayerWindow;
        PlaylistWindow      *m_pPlaylistWindow;
        amaroK::DcopHandler *m_pDcopHandler;
        amaroK::TrayIcon    *m_pTray;
};


#define pApp static_cast<App*>(kapp)


#endif  // AMAROK_APP_H
