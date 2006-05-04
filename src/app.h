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

#include "amarok_export.h"
#include "engineobserver.h" //baseclass
#include <kapplication.h>   //baseclass
#include <kurl.h>

namespace amaroK {
    class TrayIcon;
}

namespace KIO { class Job; }

class KActionCollection;
class KConfig;
class KGlobalAccel;
class MetaBundle;
class PlayerWidget;
class Playlist;
class PlaylistWindow;
class DeviceManager;

class LIBAMAROK_EXPORT App : public KApplication, public EngineObserver
{
    Q_OBJECT
    public:
        App();
       ~App();

        static App *instance() { return static_cast<App*>( qApp ); }

        static void handleCliArgs();
        static void initCliArgs( int argc, char *argv[] );

        static int mainThreadId;

        PlaylistWindow *playlistWindow() const { return m_pPlaylistWindow; }

        // FRIENDS ------
        friend class PlaylistWindow; //requires access to applySettings()

    signals:
        void useScores( bool use );
        void useRatings( bool use );
        void prepareToQuit();
    protected: /* for OSD, tray, and dcop */
        void engineStateChanged( Engine::State state, Engine::State oldState = Engine::Empty );
        void engineNewMetaData( const MetaBundle &bundle, bool trackChanged );
        void engineTrackPositionChanged( long position, bool /*userSeek*/ );
        void engineVolumeChanged( int );

    private slots:
        void showHyperThreadingWarning();
        void setRating1() { setRating( 1 ); }
        void setRating2() { setRating( 2 ); }
        void setRating3() { setRating( 3 ); }
        void setRating4() { setRating( 4 ); }
        void setRating5() { setRating( 5 ); }

    public slots:
        void applySettings( bool firstTime = false );
        void slotConfigAmarok( const QCString& page = QCString() );
        void slotConfigShortcuts();
        void slotConfigGlobalShortcuts();
        void slotConfigToolBars();
        void slotConfigEqualizer();
        void firstRunWizard();
        void setUseScores( bool use );
        void setUseRatings( bool use );
        KIO::Job *trashFiles( const KURL::List &files );
        void quit();

    private slots:
        void slotTrashResult( KIO::Job *job );

    private:
        /** Workaround for HyperThreading CPU's, @see BUG 99199 */
        void fixHyperThreading();

        void initGlobalShortcuts();
        void applyColorScheme();

        /** returns the leading window, either playerWindow or playlistWindow */
        QWidget *mainWindow() const;

        void setRating( int n );

        // ATTRIBUTES ------
        KGlobalAccel        *m_pGlobalAccel;
        PlayerWidget        *m_pPlayerWindow;
        PlaylistWindow      *m_pPlaylistWindow;
        amaroK::TrayIcon    *m_pTray;
        DeviceManager       *m_pDeviceManager;
};

#define pApp static_cast<App*>(kapp)


#endif  // AMAROK_APP_H
