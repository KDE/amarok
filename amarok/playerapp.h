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

#define ANIM_TIMER 30
#define APP_VERSION "0.8.4"
#define MAIN_TIMER 150
#define SCOPE_SIZE 7
#define VOLUME_MAX 100

#include <kglobalaccel.h>
#include <kuniqueapplication.h>
#include <kurl.h>
    
#include <vector>

class QColor;
class QListView;
class QListViewItem;
class QString;
class QTimer;

class KConfig;

class BrowserWin;
class EffectWidget;
class EngineBase;
class FHT;
class MetaBundle;
class OSDWidget;
class PlayerWidget;
class PlaylistItem;

class PlayerApp;
extern PlayerApp *pApp;

class AmarokConfig;

class PlayerApp : public KUniqueApplication
{
        Q_OBJECT

    public:
        PlayerApp();
        virtual ~PlayerApp();

        virtual int newInstance();

        bool queryClose();
        bool playObjectConfigurable();
        bool isPlaying() { return m_bIsPlaying; }
        int  trackLength() { return m_length; }
        void setupColors();
        bool restorePlaylistSelection(const KURL& url);
        void insertMedia( const KURL::List& );

        AmarokConfig *config();

        // ATTRIBUTES ------
        EngineBase *m_pEngine;
        
        KGlobalAccel *m_pGlobalAccel;

        PlayerWidget *m_pPlayerWidget;
        BrowserWin *m_pBrowserWin;
        OSDWidget *m_pOSD;

        QColor m_bgColor;
        QColor m_fgColor;

        QColor m_optBrowserBgAltColor;
        QColor m_optBrowserSelColor;

        int m_DelayTime;
        int m_Volume;
        bool m_bSliderIsPressed;
        bool m_artsNeedsRestart;

        KURL m_playingURL; ///< The URL of the currently playing item

    public slots:
        void slotPrev() const;
        void slotNext() const;
        void slotPlay() const;
        void play( const KURL&, const MetaBundle * = 0 );
        void slotPause();
        void slotStop();
        void slotSliderPressed();
        void slotSliderReleased();
        void slotSliderChanged( int );
        void slotVolumeChanged( int value );
        void slotMainTimer();
        void slotAnimTimer();
        void slotVisTimer();
        void slotPlaylistToggle( bool b );
        void slotPlaylistIsHidden();
        void slotPlaylistShowHide();
        void slotEq( bool b );
        void slotShowOptions();
        void slotConfigEffects();
        void slotHide();
        void slotShow();
        
    private slots:
        void receiveStreamMeta( QString title, QString url, QString kbps );

    signals:
        void sigScope( std::vector<float> *s );
        void sigPlay();
        /*         void sigUpdateFonts(); */
    
    private:
        void initOSD();
        void initPlayerWidget();
        void initMixer();
        bool initMixerHW();
        void initBrowserWin();
        void initColors();
        void restoreSession();

        void saveConfig();
        void readConfig();

        void setupScrolltext();

        // ATTRIBUTES ------
        bool m_usingMixerHW;
        KConfig *m_pConfig;
        QTimer *m_pMainTimer;
        QTimer *m_pAnimTimer;
        long m_length;
        int m_Mixer;
        int m_playRetryCounter;
        EffectWidget *m_pEffectWidget;
        bool m_bIsPlaying;
        bool m_bChangingSlider;
        const int m_scopeSize;
        FHT *m_pFht;
                
        bool m_XFadeRunning;
        float m_XFadeValue;
        QString m_XFadeCurrent;
    
//         int m_beatCounter;
//         float m_lastPeak;
//         float m_beatEnergy[63];
    };
#endif                                            // AMAROK_PLAYERAPP_H
