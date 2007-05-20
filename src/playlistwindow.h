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

#include <qhbox.h>          //baseclass for DynamicBox
#include <qwidget.h>        //baseclass
#include <kxmlguiclient.h>  //baseclass (for XMLGUI)

class ClickLineEdit;
class CollectionBrowser;
class ContextBrowser;
class MediaBrowser;
class QMenuBar;
class KPopupMenu;
class KToolBar;
class QLabel;
class QTimer;

/**
  * @class PlaylistWindow
  * @short The PlaylistWindow widget class.
  *
  * This is the main window widget (the Playlist not Player).
  */
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
        void addBrowser( const QString &name, QWidget *widget, const QString &text, const QString &icon );

        //takes into account minimized, multiple desktops, etc.
        bool isReallyShown() const;

        virtual bool eventFilter( QObject*, QEvent* );

        //instance is declared in KXMLGUI
        static PlaylistWindow *self() { return s_instance; }

        void activate();

    public slots:
        void showHide();
        void mbAvailabilityChanged( bool isAvailable );

    private slots:
        void savePlaylist() const;
        void slotBurnPlaylist() const;
        void slotPlayMedia();
        void slotAddLocation( bool directPlay = false );
        void slotAddStream();
        void playLastfmPersonal();
        void addLastfmPersonal();
        void playLastfmNeighbor();
        void addLastfmNeighbor();
        void playLastfmCustom();
        void addLastfmCustom();
        void playLastfmGlobaltag( int );
        void addLastfmGlobaltag( int );
        void playAudioCD();
        void showQueueManager();
        void showScriptSelector();
        void showStatistics();
        void slotMenuActivated( int );
        void actionsMenuAboutToShow();
        void toolsMenuAboutToShow();
        void slotToggleFocus();
        void slotEditFilter();
        void slotSetFilter( const QString &filter );

    protected:
        virtual void closeEvent( QCloseEvent* );
        virtual void showEvent( QShowEvent* );
        virtual QSize sizeHint() const;

    private:
        enum MenuId { ID_SHOW_TOOLBAR = 2000, ID_SHOW_PLAYERWINDOW };

        QMenuBar      *m_menubar;
        KPopupMenu    *m_toolsMenu;
        KPopupMenu    *m_settingsMenu;
        BrowserBar    *m_browsers;
        KPopupMenu    *m_searchMenu;
        ClickLineEdit *m_lineEdit;
        KToolBar      *m_toolbar;
        QTimer        *m_timer;  //search filter timer
        QStringList    m_lastfmTags;
        MediaBrowser  *m_currMediaBrowser;

        int m_lastBrowser;
        int m_searchField;

        static PlaylistWindow *s_instance;
};

class DynamicTitle : public QWidget
{
    Q_OBJECT
    public:
        DynamicTitle( QWidget* parent );
        void setTitle( const QString& newTitle );

    protected:
        virtual void paintEvent( QPaintEvent* e );

    private:
        static const int s_curveWidth = 5;
        static const int s_imageSize = 16;
        QString m_title;
        QFont m_font;
};

class DynamicBar : public QHBox
{
    Q_OBJECT
    public:
        DynamicBar( QWidget* parent );
        void init();

    public slots:
        void slotNewDynamicMode( const DynamicMode* mode );
        void changeTitle( const QString& title );

    private:
        DynamicTitle* m_titleWidget;
};


#endif //AMAROK_PLAYLISTWINDOW_H
