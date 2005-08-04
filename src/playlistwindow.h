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

#include "browserbar.h"

#include <qwidget.h>        //baseclass
#include <kxmlguiclient.h>  //baseclass (for XMLGUI)

class ClickLineEdit;
class CollectionBrowser;
class ContextBrowser;
class KMenuBar;
class KPopupMenu;
class KToolBar;
class QTimer;


class PlaylistWindow : public QWidget, public KXMLGUIClient
{
        Q_OBJECT

    public:
        PlaylistWindow();
       ~PlaylistWindow();

        void init();
        void applySettings();

        void createGUI(); //should be private but App::slowConfigToolbars requires it
        void recreateGUI();

        //allows us to switch browsers from within other browsers etc
        void showBrowser( const QString& name ) { m_browsers->showBrowser( name ); }

        virtual bool eventFilter( QObject*, QEvent* );

        //instance is declared in KXMLGUI
        static PlaylistWindow *self() { return s_instance; }

    public slots:
        void showHide();

    private slots:
        void savePlaylist() const;
        void slotPlayMedia();
        void slotAddLocation( bool directPlay = false );
        void playAudioCD();
        void showQueueManager();
        void showScriptSelector();
        void slotMenuActivated( int );
        void actionsMenuAboutToShow();
        void toolsMenuAboutToShow();
        void slotToggleMenu();

    protected:
        virtual void closeEvent( QCloseEvent* );
        virtual QSize sizeHint() const;

    private:
        enum MenuId { ID_SHOW_TOOLBAR = 2000, ID_SHOW_PLAYERWINDOW };

        KMenuBar      *m_menubar;
        KPopupMenu    *m_toolsMenu;
        KPopupMenu    *m_settingsMenu;
        BrowserBar    *m_browsers;
        KPopupMenu    *m_searchMenu;
        ClickLineEdit *m_lineEdit;
        KToolBar      *m_toolbar;
        QTimer        *m_timer;  //search filter timer

        int m_lastBrowser;
        int m_searchField;

        static PlaylistWindow *s_instance;
};


#endif //AMAROK_PLAYLISTWINDOW_H
