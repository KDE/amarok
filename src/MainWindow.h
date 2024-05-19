/****************************************************************************************
 * Copyright (c) 2002-2013 Mark Kretschmann <kretschmann@kde.org>                       *
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

#include <config.h>

#include "amarok_export.h"
#include "core/meta/forward_declarations.h"

#include <QPointer>

#include <KMainWindow>
#include <KToggleAction>

#include <phonon/Global>

class CollectionWidget;
class SlimToolbar;
class MainToolbar;
class MainWindow;
#ifdef DEBUG_BUILD_TYPE
class NetworkAccessViewer;
#endif // DEBUG_BUILD_TYPE

namespace PlaylistBrowserNS { class PlaylistBrowser; }
namespace Playlist { class Dock; }
class BrowserDock;
class ContextDock;


class QMenu;
class QMenuBar;

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
class AMAROK_EXPORT MainWindow : public KMainWindow
{
    Q_OBJECT

    public:
        enum AmarokDockId {
            AmarokDockNavigation,
            AmarokDockContext,
            AmarokDockPlaylist
        };
        
        MainWindow();
        ~MainWindow() override;

        void activate();

        //allows us to switch browsers from within other browsers etc
        void showBrowser( const QString& name );

        //ensures the dock widget is visible in case it is tabbed
        void showDock( AmarokDockId dockId );

        QPointer<BrowserDock> browserDock() const { return m_browserDock; }
        QPointer<QMenu> ToolsMenu() const { return m_toolsMenu; }
        QPointer<QMenu> SettingsMenu() const { return m_settingsMenu; }
        QPointer<Playlist::Dock> playlistDock() const { return m_playlistDock; }
        void deleteBrowsers();

        /* Reimplemented from QMainWindow to allow only one active toolbar at any time */
        QMenu* createPopupMenu() override;

        void addViewMenuItems(QMenu* menu);

        QString activeBrowserName();

        CollectionWidget * collectionBrowser();

        bool isLayoutLocked() const;

        /**
         *    If an audiocd collection is present. Stop current playback, clear playlist,
         *    add cd to playlist and start playback
         */
        bool playAudioCd();

        bool isWaitingForCd() const;

        /**
         * @return Whether the application is on the currently active virtual desktop and visible.
           Behaviour might depend on windowing system.
         */
        void checkIfExpensivesShouldBeDrawn();

    Q_SIGNALS:
        void loveTrack( Meta::TrackPtr track );
        void banTrack( Meta::TrackPtr track );
        void skipTrack();
        void switchQueueStateShortcut();

        /**
        * Called when something happens with window that might affect the need to draw expensive elements
        * The parameter tells if drawing should continue
        */
        void drawNeedChanged( bool ) const;

    public Q_SLOTS:
        void showHide();
        void slotFullScreen();
        void showNotificationPopup();
        void setLayoutLocked( bool locked );
        void resetLayout();
        void showAbout();
        void showReportBug();

    private Q_SLOTS:
        void setDefaultDockSizes();

        void slotLoveTrack();
        void slotBanTrack();

        void slotStopped();
        void slotPaused();
        void slotNewTrackPlaying();
        void slotMetadataChanged( Meta::TrackPtr track );

        void exportPlaylist();
        void slotShowActiveTrack() const;
        void slotEditTrackInfo() const;
        void slotShowBookmarkManager();
        void slotShowEqualizer();
        void slotShowCoverManager();
        void slotShowDiagnosticsDialog();
        void slotShowMenuBar();
        void slotPlayMedia();
        void slotAddLocation( bool directPlay = false );
        void slotAddStream();
        void slotFocusPlaylistSearch();
        void slotFocusCollectionSearch();
        void slotShufflePlaylist();
        void slotSeekForwardShort();
        void slotSeekForwardMedium();
        void slotSeekForwardLong();
        void slotSeekBackwardShort();
        void slotSeekBackwardMedium();
        void slotSeekBackwardLong();
        void slotPutCurrentTrackToClipboard();

#ifdef DEBUG_BUILD_TYPE
        void showNetworkRequestViewer();
#endif // DEBUG_BUILD_TYPE

    protected:
        void closeEvent( QCloseEvent* ) override;
        void changeEvent( QEvent *event ) override;

    private Q_SLOTS:
        void setRating1() { setRating( 1 ); }
        void setRating2() { setRating( 2 ); }
        void setRating3() { setRating( 3 ); }
        void setRating4() { setRating( 4 ); }
        void setRating5() { setRating( 5 ); }

    private:
        void init();
        void setRating( int n );

        CollectionWidget * m_collectionBrowser;
        PlaylistBrowserNS::PlaylistBrowser * m_playlistBrowser;

        QPointer<QMenuBar>  m_menubar;
        QPointer<QMenu>     m_toolsMenu;
        QPointer<QMenu>     m_settingsMenu;
#ifdef DEBUG_BUILD_TYPE
        QPointer<NetworkAccessViewer> m_networkViewer;
#endif // DEBUG_BUILD_TYPE

        QPointer<BrowserDock> m_browserDock;
        QPointer<ContextDock> m_contextDock;
        QPointer<Playlist::Dock> m_playlistDock;

        QPointer<SlimToolbar> m_slimToolbar;
        QPointer<MainToolbar> m_mainToolbar;

        void createActions();
        void createMenus();

        KToggleAction* m_showMenuBar;

        int m_lastBrowser;
        int m_searchField;

        bool m_waitingForCd;
        bool m_expensiveDrawingPaused;
};


#endif //AMAROK_PLAYLISTWINDOW_H

