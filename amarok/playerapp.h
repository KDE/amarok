/***************************************************************************
                         playerapp.h  -  description
                            -------------------
   begin                : Mit Okt 23 14:35:18 CEST 2002
   copyright            : (C) 2002 by Mark Kretschmann
   email                :
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

#define APP_VERSION "0.7.0"

#include "amarokarts/amarokarts.h"

#include <kglobalaccel.h>
#include <kuniqueapplication.h>

#include <vector>

#include <arts/artsmodules.h>
#include <arts/kartsdispatcher.h>
#include <arts/kplayobjectfactory.h>

class QColor;
class QListView;
class QListViewItem;
class QString;
class QTimer;

class KConfig;

class BrowserWin;
class EffectWidget;
class PlaylistItem;
class PlayerWidget;

class PlayerApp;
extern PlayerApp *pApp;

class PlayerApp : public KUniqueApplication
{
        Q_OBJECT

    public:
        PlayerApp();
        virtual ~PlayerApp();

        virtual int newInstance();

        bool isFileValid( const KURL &url );
        bool queryClose();
        bool playObjectConfigurable();
        bool isPlaying() { return m_bIsPlaying; }
        void setupColors();

        // ATTRIBUTES ------
        KGlobalAccel *m_pGlobalAccel;

        PlayerWidget *m_pPlayerWidget;
        BrowserWin *m_pBrowserWin;

        QColor m_bgColor;
        QColor m_fgColor;

        // <option attributes>
        bool m_optSavePlaylist;
        bool m_optConfirmClear;
        bool m_optConfirmExit;
        bool m_optFollowSymlinks;
        bool m_optTimeDisplayRemaining;
        bool m_optReadMetaInfo;
        bool m_optRepeatTrack;
        bool m_optRepeatPlaylist;
        bool m_optRandomMode;
        bool m_optShowTrayIcon;
        bool m_optHidePlaylistWindow;
        bool m_optSoftwareMixerOnly;
        bool m_optResumePlayback;
        bool m_optUseCustomFonts;
        QString m_optDropMode;
        long m_optXFadeLength;
        long m_optTrackDelay;
        QFont m_optBrowserWindowFont;
        QFont m_optPlayerWidgetFont;
        QFont m_optPlayerWidgetScrollFont;
        QColor m_optBrowserFgColor;
        QColor m_optBrowserBgColor;
        QColor m_optBrowserBgAltColor;
        QColor m_optBrowserSelColor;
        bool   m_optBrowserUseCustomColors;
        uint m_optUndoLevels;
        int m_optVisCurrent;
        int m_optBrowserSortSpec;
        bool m_optTitleStream;
        // </option attributes>

        int m_DelayTime;
        int m_Volume;
        bool m_bSliderIsPressed;

        // <aRts>
        KDE::PlayObject *m_pPlayObject;
        KDE::PlayObject *m_pPlayObjectXFade;
        Arts::SoundServerV2 m_Server;
        Arts::StereoFFTScope m_Scope;
        Arts::StereoEffectStack m_globalEffectStack;
        Arts::StereoEffectStack m_effectStack;
        Arts::StereoVolumeControl m_volumeControl;
        Arts::Synth_AMAN_PLAY m_amanPlay;
        Amarok::Synth_STEREO_XFADE m_XFade;
        // </aRts>
        
    public slots:
        void slotPrev();
        void slotPlay();
        void slotConnectPlayObj();
        void slotPause();
        void slotStop();
        void slotStopCurrent();
        void slotNext();
        void slotSavePlaylist();
        void slotClearPlaylistAsk();
        void slotAddLocation();
        void slotSliderPressed();
        void slotSliderReleased();
        void slotSliderChanged( int );
        void slotVolumeChanged( int value );
        void slotMainTimer();
        void slotAnimTimer();
        void slotVisTimer();
        void slotItemDoubleClicked( QListViewItem *item );
        void slotShowAbout();
        void slotPlaylistToggle( bool b );
        void slotPlaylistIsHidden();
        void slotEq( bool b );
        void slotShowOptions();
        void slotConfigEffects();
        void slotShowTip();
        void slotSetRepeatTrack();
        void slotSetRepeatPlaylist();
        void slotSetRandomMode();
        void slotShowHelp();
        void slotHide();
        void slotShow();

    private slots:
        void receiveStreamMeta( QString title, QString url );

    signals:
        void sigScope( std::vector<float> *s );
        void sigPlay();
        /*         void sigUpdateFonts(); */

    private:
        void initArts();
        void initPlayerWidget();
        void initMixer();
        bool initMixerHW();
        bool initScope();
        void initBrowserWin();
        void initColors();

        void saveConfig();
        void readConfig();

        void getTrackLength();
        void setupScrolltext();
        void startXFade();
        void stopXFade();

        QString convertDigit( const long &digit );

        // ATTRIBUTES ------
        KArtsDispatcher *m_pArtsDispatcher;
        bool m_usingMixerHW;
        KConfig *m_pConfig;
        QTimer *m_pMainTimer;
        QTimer *m_pAnimTimer;
        long m_scopeId;
        bool m_scopeActive;
        long m_length;
        int m_Mixer;
        int m_playRetryCounter;
        EffectWidget *m_pEffectWidget;
        bool m_bIsPlaying;
        bool m_bChangingSlider;

        bool m_XFadeRunning;
        float m_XFadeValue;
        QString m_XFadeCurrent;
};
#endif                                            // AMAROK_PLAYERAPP_H
