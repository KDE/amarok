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

#include <kuniqueapplication.h>
#include <kurl.h>
#include <vector>

#define APP_VERSION "0.9-CVS"

class QColor;
class QListView;
class QListViewItem;
class QString;
class QTimer;
class QEvent;

class KGlobalAccel;

class BrowserWin;
class EngineBase;
class MetaBundle;
class OSDWidget;
class PlayerWidget;
class PlaylistItem;

class PlayerApp : public KUniqueApplication
{
        Q_OBJECT

    public:
        PlayerApp();
        ~PlayerApp();

        int newInstance();

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
        void slotSliderPressed();
        void slotSliderReleased();
        void slotSliderChanged( int );
        void slotVolumeChanged( int value );
        void slotMainTimer();
        void slotShowOptions();

    private slots:
        void applySettings();
        void proxyError();

    signals:
        //void sigScope( std::vector<float> *s );
        //void sigPlay();
        void metaData( const MetaBundle& );
        void orderPreviousTrack();
        void orderCurrentTrack();
        void orderNextTrack();
        void currentTrack( const KURL& );

    private:
        void initBrowserWin();
        void initColors();
        void initConfigDialog();
        void initMixer();
        bool initMixerHW();
        void initPlayerWidget();
        void readConfig();
        void restoreSession();
        void saveConfig();
        bool eventFilter( QObject*, QEvent* );

        void setupScrolltext();

        // ATTRIBUTES ------
        QTimer *m_pMainTimer;
        QTimer *m_pAnimTimer;
        long m_length;
        int m_playRetryCounter;
        int m_delayTime;


        OSDWidget *m_pOSD;
        bool m_proxyError;
};


#endif                                            // AMAROK_PLAYERAPP_H

extern PlayerApp* pApp;
