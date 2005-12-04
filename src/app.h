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

namespace amaroK {
    class TrayIcon;
}

class KActionCollection;
class KConfig;
class KGlobalAccel;
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

        PlaylistWindow *playlistWindow() const { return m_pPlaylistWindow; }

        // FRIENDS ------
        friend class PlaylistWindow; //requires access to applySettings()
    protected: /* for OSD, tray, and dcop */
        void engineStateChanged( Engine::State state, Engine::State oldState = Engine::Empty );
        void engineNewMetaData( const MetaBundle &bundle, bool trackChanged );
        void engineTrackPositionChanged( long position, bool /*userSeek*/ );
        void engineVolumeChanged( int );

    private slots:
        void showHyperThreadingWarning();

    public slots:
        void applySettings( bool firstTime = false );
        void slotConfigAmarok( const QCString& page = QCString() );
        void slotConfigShortcuts();
        void slotConfigGlobalShortcuts();
        void slotConfigToolBars();
        void slotConfigEqualizer();
        void firstRunWizard();

    private:
        /** Workaround for HyperThreading CPU's, @see BUG 99199 */
        void fixHyperThreading();

        void initGlobalShortcuts();
        void applyColorScheme();

        /** returns the leading window, either playerWindow or playlistWindow */
        QWidget *mainWindow() const;

        // ATTRIBUTES ------
        KGlobalAccel        *m_pGlobalAccel;
        PlayerWidget        *m_pPlayerWindow;
        PlaylistWindow      *m_pPlaylistWindow;
        amaroK::TrayIcon    *m_pTray;
};

#define pApp static_cast<App*>(kapp)


#endif  // AMAROK_APP_H
