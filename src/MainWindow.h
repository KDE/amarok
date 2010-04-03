/****************************************************************************************
 * Copyright (c) 2002-2009 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2002 Max Howell <max.howell@methylblue.com>                            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "amarok_export.h"
#include "core/meta/Meta.h"
#include "core/engine/EngineObserver.h"
#include "browsers/BrowserWidget.h"

#include <KMainWindow>
#include <KVBox>

#include <QPointer>

class CollectionWidget;
class ContextWidget;
class SlimToolbar;
class MainToolbar;
class MainWindow;
class PlaylistFileProvider;
class SearchWidget;

namespace PlaylistBrowserNS { class PlaylistBrowser; }

namespace Plasma { class Containment; }
namespace Playlist { class Widget; }

namespace Context {
    class ContextScene;
    class ContextView;
    class ToolbarView;
}

class KMenu;
class QMenuBar;
class QSplitter;
class QTimer;

namespace The {
    AMAROK_EXPORT MainWindow* mainWindow();
}

//This should only change if docks or toolbars are added or removed
#define LAYOUT_VERSION 3

/**
  * @class MainWindow
  * @short The MainWindow widget class.
  *
  * This is the main window widget.
  */
class AMAROK_EXPORT MainWindow : public KMainWindow, public Engine::EngineObserver, public Meta::Observer
{
    friend MainWindow* The::mainWindow();

    Q_OBJECT

    public:
        MainWindow();
        ~MainWindow();

        //allows us to switch browsers from within other browsers etc
        void showBrowser( const QString& name );
        void addBrowser( const QString &name, QWidget *widget, const QString &text, const QString &icon );

        //takes into account minimized, multiple desktops, etc.
        bool isReallyShown() const;

        void activate();

        BrowserWidget *browserWidget() const { return m_browsers; }
        QPointer<KMenu> ToolsMenu() const { return m_toolsMenu; }
        QPointer<KMenu> SettingsMenu() const { return m_settingsMenu; }
        QPointer<Playlist::Widget> playlistWidget() { return m_playlistWidget; }
        void deleteBrowsers();

        /* Reimplemented from QMainWindow to allow only one active toolbar at any time */
        virtual QMenu* createPopupMenu();

        QString activeBrowserName();

        CollectionWidget * collectionBrowser();
        PlaylistBrowserNS::PlaylistBrowser * playlistBrowser();

        //will return the size of the rect defined top, right and left by the main toolbar and bottom by the context view.
        QSize backgroundSize();

        int contextXOffset();
        QRect contextRectGlobal();
        QPoint globalBackgroundOffset();

        bool isLayoutLocked();

        /**
        *    If an audiocd collection is present. Stop current playback, clear playlist,
        *    add cd to playlist and start playback
        */
        bool playAudioCd();

        bool isWaitingForCd();

    signals:
        void loveTrack( Meta::TrackPtr track );
        void banTrack();
        void skipTrack();
        void switchQueueStateShortcut();

    public slots:
        void showHide();
        void slotFullScreen();
        void slotLoveTrack() { emit loveTrack( The::engineController()->currentTrack() ); }
        void showNotificationPopup();
        void hideContextView( bool hide );

        void setLayoutLocked( bool locked );

        void showAbout();
        void showReportBug();

    protected:
        //Reimplemented from EngineObserver
        virtual void engineStateChanged( Phonon::State state, Phonon::State oldState = Phonon::StoppedState );
        virtual void engineNewTrackPlaying();

        //Reimplemented from Meta::Observer
        using Observer::metadataChanged;
        virtual void metadataChanged( Meta::TrackPtr track );

    private slots:
        void exportPlaylist() const;
        void slotShowActiveTrack() const;
        void slotShowBookmarkManager() const;
        void slotShowEqualizer() const;
        void slotShowCoverManager() const;
        void slotPlayMedia();
        void slotAddLocation( bool directPlay = false );
        void slotAddStream();
        void slotJumpTo();
        void showScriptSelector();

        /**
         * Save state and position of dock widgets.
         */
        void saveLayout();

        /**
         * Try to restore saved layout, if this fails, try to use the default layout.
         */
        void restoreLayout();

    protected:
        bool eventFilter(QObject *, QEvent *);
        virtual void closeEvent( QCloseEvent* );
        virtual void keyPressEvent( QKeyEvent* );
        virtual void resizeEvent ( QResizeEvent * event );
        virtual void paletteChange( const QPalette & oldPalette );

    private slots:
        void setRating1() { setRating( 1 ); }
        void setRating2() { setRating( 2 ); }
        void setRating3() { setRating( 3 ); }
        void setRating4() { setRating( 4 ); }
        void setRating5() { setRating( 5 ); }

    private:
        void init();
        void setRating( int n );
        void showBrowser( const int index );

        CollectionWidget * m_collectionBrowser;
        PlaylistBrowserNS::PlaylistBrowser * m_playlistBrowser;

        QPointer<QMenuBar>  m_menubar;
        QPointer<KMenu>     m_toolsMenu;
        QPointer<KMenu>     m_settingsMenu;
        QPointer<BrowserWidget>   m_browsers;
        QStringList         m_browserNames;
        QPointer<KMenu>     m_searchMenu;
        //QPointer<KVBox>     m_statusbarArea;

        QPointer<SearchWidget>     m_searchWidget;
        QPointer<Playlist::Widget> m_playlistWidget;
        QPointer<QTimer>           m_timer;  //search filter timer
        QPointer<QSplitter>        m_splitter;

        QByteArray                 m_splitterState;

        QPointer<ContextWidget>         m_contextWidget;
        QPointer<Context::ContextScene> m_corona;
        QPointer<Context::ContextView>  m_contextView;
        QPointer<Context::ToolbarView>  m_contextToolbarView;

        Meta::TrackPtr m_currentTrack;

        QDockWidget * m_browsersDock;
        QDockWidget * m_contextDock;
        QDockWidget * m_playlistDock;

        QPointer<SlimToolbar> m_slimToolbar;
        QPointer<MainToolbar> m_mainToolbar;

        QWidget *     m_browserDummyTitleBarWidget;
        QWidget *     m_contextDummyTitleBarWidget;
        QWidget *     m_playlistDummyTitleBarWidget;


        void    createActions();
        void    createMenus();
        int     m_lastBrowser;
        int     m_searchField;

        static QPointer<MainWindow> s_instance;

        bool m_layoutLocked;

        bool m_waitingForCd;

        // Layout hack -----------------
        typedef struct Ratio {
            float x,y;
        } Ratio;
        Ratio m_browsersRatio;
        Ratio m_contextRatio;
        Ratio m_playlistRatio;
        QRect m_dockingRect;
        bool m_mouseDown;
        bool m_LH_initialized;
        QTimer * m_saveLayoutChangesTimer;

        bool LH_isIrrelevant( const QDockWidget *dock );
        QTabBar *LH_dockingTabbar();
        void LH_extend( QRect &target, const QDockWidget *dock );
        QSize LH_desiredSize( QDockWidget *dock, const QRect &area, float rx, float ry, int splitter );
        bool LH_fuzzyMatch( const QSize &sz1, const QSize &sz2 );
        bool LH_isConstrained( const QDockWidget *dock );

    private slots:
        void updateDockRatio();
        void updateDockRatio(QDockWidget*);
        void initLayoutHack();
        // ------------------------------
        void createContextView( Plasma::Containment *c );
};


#endif //AMAROK_PLAYLISTWINDOW_H

