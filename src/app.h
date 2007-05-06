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

#include "config-amarok.h"

#include "amarok_export.h"
#include "engineobserver.h" //baseclass
#include <kapplication.h>   //baseclass
#include <kurl.h>

#include <QByteArray>

namespace Amarok {
    class TrayIcon;
}


namespace KIO { class Job; }

class KJob;
class KActionCollection;
class KConfig;
class MetaBundle;
class Playlist;
class PlaylistWindow;
class MediaDeviceManager;

class AMAROK_EXPORT App : public KApplication, public EngineObserver
{
    Q_OBJECT
    public:
        App();
       ~App();

        static App *instance() { return static_cast<App*>( kapp ); }

        static void handleCliArgs();
        static void initCliArgs( int argc, char *argv[] );

        static int mainThreadId;

        PlaylistWindow *playlistWindow() const { return m_pPlaylistWindow; }

        // FRIENDS ------
        friend class PlaylistWindow; //requires access to applySettings()

    signals:
        void useScores( bool use );
        void useRatings( bool use );
        void moodbarPrefs( bool show, bool moodier, int alter, bool withMusic );
        void prepareToQuit();
    protected: /* for OSD, tray, and dcop */
        void engineStateChanged( Engine::State state, Engine::State oldState = Engine::Empty );
        void engineNewMetaData( const MetaBundle &bundle, bool trackChanged );
        void engineTrackPositionChanged( long position, bool /*userSeek*/ );
        void engineVolumeChanged( int );

    private slots:
        void setRating1() { setRating( 1 ); }
        void setRating2() { setRating( 2 ); }
        void setRating3() { setRating( 3 ); }
        void setRating4() { setRating( 4 ); }
        void setRating5() { setRating( 5 ); }
        void continueInit();


    public slots:
        void applySettings( bool firstTime = false );
        void slotConfigAmarok( const QByteArray& page = QByteArray() );
        void slotConfigShortcuts();
        void slotConfigToolBars();
        void slotConfigEqualizer();
        void setUseScores( bool use );
        void setUseRatings( bool use );
        void setMoodbarPrefs( bool show, bool moodier, int alter, bool withMusic );
        KIO::Job *trashFiles( const KUrl::List &files );
        void quit();

    private slots:
        void slotTrashResult( KJob *job );

    private:
        void initGlobalShortcuts();
        void firstRunWizard();

        /** returns the playlistWindow */
        QWidget *mainWindow() const;

        void setRating( int n );

        // ATTRIBUTES ------
        PlaylistWindow      *m_pPlaylistWindow;
#ifndef Q_WS_MAC
        Amarok::TrayIcon    *m_pTray;
#endif
        MediaDeviceManager  *m_pMediaDeviceManager;
};

#define pApp static_cast<App*>(kapp)


#endif  // AMAROK_APP_H
