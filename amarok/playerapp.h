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

#define APP_VERSION "0.9.1-CVS"

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

class KActionCollection;
class KGlobalAccel;


class PlayerApp : public KApplication
{
        Q_OBJECT

    public:
        PlayerApp();
        ~PlayerApp();

        bool playObjectConfigurable();
        void setupColors();
        void insertMedia( const KURL::List& );
        static void initCliArgs( int argc, char *argv[] );

        KActionCollection *actionCollection() { return m_pActionCollection; }

        // STATICS
        static const int     SCOPE_SIZE  = 7;

        // ATTRIBUTES
        KGlobalAccel *m_pGlobalAccel;

        PlayerWidget *m_pPlayerWidget;
        BrowserWin   *m_pBrowserWin;

        QColor m_optBrowserBgAltColor;
        QColor m_optBrowserSelColor;

        bool m_artsNeedsRestart;

    public slots:
        void slotPlaylistShowHide();
        void slotShowOptions();
        void slotShowOSD();
        void slotShowVolumeOSD();
        void slotIncreaseVolume();
        void slotDecreaseVolume();
        void setOsdEnabled(bool enable);
        void slotConfigShortcuts();
        void slotConfigGlobalShortcuts();

    private slots:
        void handleLoaderArgs( QCString args );
        void applySettings();
        void showEffectWidget();
        void slotEffectWidgetDestroyed();
        void slotShowOSD( const MetaBundle& );


    private:
        void handleCliArgs();
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
        long      m_length;
        OSDWidget *m_pOSD;
        int       m_sockfd;
        QString   m_textForOSD;
        bool      m_showBrowserWin;
        KActionCollection *m_pActionCollection;
};


class LoaderServer : public QServerSocket
{
    Q_OBJECT

    public:
        LoaderServer( QObject* parent );

    signals:
        void loaderArgs( QCString );

    private :
        void newConnection( int socket );
};


#endif                                            // AMAROK_PLAYERAPP_H

extern PlayerApp* pApp;
