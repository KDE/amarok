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

#ifndef AMAROK_PLAYLISTWINDOW_H
#define AMAROK_PLAYLISTWINDOW_H

#include <enginecontroller.h>  //baseclass

#include <qwidget.h>           //baseclass
#include <ktoolbar.h>          //baseclass
#include <kxmlguiclient.h>     //baseclass (for XMLGUI)


namespace amaroK {
    class ToolBar;
}

using amaroK::ToolBar;

class BrowserBar;
class ClickLineEdit;
class CollectionBrowser;
class ContextBrowser;
class Playlist;

class QTimer;

class KMenuBar;
class KPopupMenu;
class KStatusBar;


class PlaylistWindow : public QWidget, public KXMLGUIClient, public EngineObserver
{
        Q_OBJECT

    public:
        PlaylistWindow();
        ~PlaylistWindow();

        void init();

        void setFont( const QFont&, const QFont& );
        void setColors( const QPalette&, const QColor& );

        void createGUI(); //should be private but App::slowConfigToolbars requires it
        void recreateGUI();
        BrowserBar* browserBar() { return m_browsers; }

        virtual bool eventFilter( QObject*, QEvent* );

        //instance is declared in KXMLGUI
        static PlaylistWindow *self() { return s_instance; }

    public slots:
        void showHide();
        void showMenuBar( bool enabled = true );

    private slots:
        void savePlaylist() const;
        void slotAddLocation();
        void playAudioCD();
        void slotMenuActivated( int );
        void toolsMenuAboutToShow();

    protected:
        virtual void closeEvent( QCloseEvent* );
        void engineStateChanged( Engine::State );

    private:
        template <class B> void addBrowser( const char*, const QString&, const QString& );

        enum MenuId { ID_SHOW_MENUBAR = 2000, ID_SHOW_TOOLBAR, ID_SHOW_PLAYERWINDOW };

        KMenuBar      *m_menubar;
        KPopupMenu    *m_toolsMenu;
        KPopupMenu    *m_settingsMenu;
        BrowserBar    *m_browsers;
        Playlist      *m_playlist;
        KPopupMenu    *m_searchMenu;
        ClickLineEdit *m_lineEdit;
        KStatusBar    *m_statusbar;
        ToolBar       *m_toolbar;
        QTimer        *m_timer;  //search filter timer

        int m_lastBrowser;
        int m_searchField;

        static PlaylistWindow *s_instance;
};


#endif //AMAROK_PLAYLISTWINDOW_H
