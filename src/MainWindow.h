/***************************************************************************
  begin                : Fre Nov 15 2002
  copyright            : (C) Mark Kretschmann <markey@web.de>
                       : (C) Max Howell <max.howell@methylblue.com>
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "amarok_export.h"
#include "meta/Meta.h"

#include <KXmlGuiWindow>

class ContextWidget;
class MainToolbar;
class MainWindow;
class PlaylistFileProvider;
class SearchWidget;
class SideBar;

namespace Plasma
{
    class Containment;
}

namespace Context {
    class ContextScene;
    class ContextView;
}

class KMenu;
class QMenuBar;
class QSplitter;
class QTimer;

namespace The {
        AMAROK_EXPORT MainWindow* mainWindow();
}

/**
  * @class MainWindow
  * @short The MainWindow widget class.
  *
  * This is the main window widget.
  */
class AMAROK_EXPORT MainWindow : public KXmlGuiWindow
{
    friend MainWindow* The::mainWindow();

    Q_OBJECT

    public:
        MainWindow();
       ~MainWindow();

        void init();

        //allows us to switch browsers from within other browsers etc
        void showBrowser( const QString& name );
        void addBrowser( const QString &name, QWidget *widget, const QString &text, const QString &icon );

        //takes into account minimized, multiple desktops, etc.
        bool isReallyShown() const;

        void activate();

        SideBar *sideBar() const { return m_browsers; }
        KMenu   *ToolsMenu() const { return m_toolsMenu; }
        KMenu   *SettingsMenu() const { return m_settingsMenu; }
        void deleteBrowsers();

        //will return the size of the rect defined top, right and left by the main toolbar and bottom by the context view.
        QSize backgroundSize();

        int contextXOffset();
        QRect contextRectGlobal();
        QPoint globalBackgroundOffset();

    signals:
        void loveTrack( Meta::TrackPtr );

    public slots:
        void showHide();
        void loveTrack();
        void playAudioCD();

    private slots:
        void slotShrinkBrowsers( int index );
        void savePlaylist() const;
        void exportPlaylist() const;
        void slotBurnPlaylist() const;
        void slotShowCoverManager() const;
        void slotPlayMedia();
        void slotAddLocation( bool directPlay = false );
        void slotAddStream();
        void showQueueManager();
        void showScriptSelector();
        void showStatistics();
        void toolsMenuAboutToShow();
        void slotToggleFocus();
        void slotSetFilter( const QString &filter );

    protected:
        virtual void closeEvent( QCloseEvent* );
        virtual void keyPressEvent( QKeyEvent* );
        virtual void showEvent( QShowEvent* );
        virtual QSize sizeHint() const;
        virtual void resizeEvent ( QResizeEvent * event );
        //virtual void paintEvent( QPaintEvent * );

        virtual void paletteChange( const QPalette & oldPalette );

    private slots:
        void setRating1() { setRating( 1 ); }
        void setRating2() { setRating( 2 ); }
        void setRating3() { setRating( 3 ); }
        void setRating4() { setRating( 4 ); }
        void setRating5() { setRating( 5 ); }

    private:
        void setRating( int n );
        void showBrowser( const int index );

        QMenuBar      *m_menubar;
        KMenu         *m_toolsMenu;
        KMenu         *m_settingsMenu;
        SideBar       *m_browsers;
        QStringList    m_browserNames;
        KMenu         *m_searchMenu;

        SearchWidget  *m_searchWidget;
        MainToolbar   *m_controlBar;
        QTimer        *m_timer;  //search filter timer
        QStringList    m_lastfmTags;
        //MediaBrowser  *m_currMediaBrowser;
        QSplitter     *m_splitter;
        QByteArray     m_splitterState;

        ContextWidget *m_contextWidget;
        Context::ContextScene *m_corona;
        Context::ContextView *m_contextView;

        PlaylistFileProvider *m_playlistFiles;

        void    createActions();
        void    createMenus();
        int     m_lastBrowser;
        int     m_searchField;

        static MainWindow *s_instance;

    private slots:
        void createContextView( Plasma::Containment *c );
};


#endif //AMAROK_PLAYLISTWINDOW_H

