/***************************************************************************
                         playerapp.h  -  description
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

#ifndef AMAROK_PLAYERAPP_H
#define AMAROK_PLAYERAPP_H

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include <qserversocket.h>         //baseclass

#include <kapplication.h>          //baseclass
#include <kurl.h>                  //needed for KURL::List (nested)

#define APP_VERSION "0.9-CVS"

class BrowserWin;
class EngineBase;
class MetaBundle;
class OSDWidget;
class PlayerWidget;
class PlaylistItem;

class QColor;
class QCString;
class QEvent;
class QListView;
class QListViewItem;
class QString;
class QTimer;

class KCmdLineArgs;
class KGlobalAccel;


class PlayerApp : public KApplication
{
        Q_OBJECT

    public:
        PlayerApp();
        ~PlayerApp();

        bool playObjectConfigurable();
        bool isPlaying() const;
        int  trackLength() const { return m_length; }
        void setupColors();
        void insertMedia( const KURL::List& );

        // STATICS
        static const int     ANIM_TIMER  = 30;
        static const int     MAIN_TIMER  = 150;
        static const int     SCOPE_SIZE  = 7;

        // ATTRIBUTES
        static EngineBase *m_pEngine;

        KGlobalAccel *m_pGlobalAccel;

        PlayerWidget *m_pPlayerWidget;
        BrowserWin   *m_pBrowserWin;

        QColor m_optBrowserBgAltColor;
        QColor m_optBrowserSelColor;

        bool m_sliderIsPressed;
        bool m_artsNeedsRestart;

        KURL m_playingURL; ///< The URL of the currently playing item

    public slots:
        void slotPrev();
        void slotNext();
        void slotPlay();
        void play( const MetaBundle& );
        void slotPause();
        void slotStop();
        void slotPlaylistShowHide();
        void slotSliderPressed();
        void slotSliderReleased();
        void slotSliderChanged( int );
        void slotVolumeChanged( int value );
        void slotMainTimer();
        void slotShowOptions();
        void slotShowOSD();

    private slots:
        void handleLoaderArgs( const QCString& args );
        void applySettings();
        void loaderMessage();
        void proxyError();
        void showEffectWidget();
        void prepareTextForOSD( const MetaBundle& );

    signals:
        void metaData( const MetaBundle& );
        void orderPreviousTrack();
        void orderCurrentTrack();
        void orderNextTrack();
        void orderRecentTrack();
        void currentTrack( const KURL& );

    private:
        void handleCliArgs( KCmdLineArgs* args );
        void initBrowserWin();
        void initColors();
        void initConfigDialog();
        void initEngine();
        void initIpc();
        void initMixer();
        bool initMixerHW();
        void initPlayerWidget();
        void readConfig();
        void restoreSession();
        void saveConfig();
        bool eventFilter( QObject*, QEvent* );

        void setupScrolltext();

        // ATTRIBUTES ------
        QTimer    *m_pMainTimer;
        QTimer    *m_pAnimTimer;
        long      m_length;
        int       m_playRetryCounter;
        int       m_delayTime;
        OSDWidget *m_pOSD;
        bool      m_proxyError;
        int       m_sockfd;
        QString   m_textForOSD;
};


class LoaderServer : public QServerSocket
{
    Q_OBJECT

    public:
        LoaderServer( QObject* parent );

    signals:
        void loaderArgs( const QCString& );

    private :
        void newConnection( int socket );
};


#endif                                            // AMAROK_PLAYERAPP_H

extern PlayerApp* pApp;
