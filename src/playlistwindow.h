/***************************************************************************
                         browserwin.h  -  description
                            -------------------
   begin                : Fre Nov 15 2002
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

#ifndef AMAROK_PLAYLISTWINDOW_H
#define AMAROK_PLAYLISTWINDOW_H

#include <qwidget.h>        //baseclass
#include <kurl.h>           //KURL::List
#include <kxmlguiclient.h>  //baseclass (for XMLGUI)

class BrowserBar;
class ContextBrowser;
class KLineEdit;
class KActionCollection;
class KToolBar;
class PlaylistLoader;
class Playlist;
class QColor;
class QCloseEvent;
class QCustomEvent;
class QFocusEvent;
class QFont;
class QListViewItem;
class QPalette;
class QPoint;
class QPushButton;
class QSplitter;
class QString;

class PlaylistWindow : public QWidget, public KXMLGUIClient
{
        Q_OBJECT

    public:
        PlaylistWindow( QWidget* = 0, const char* = 0 );
        ~PlaylistWindow();

        //convenience functions
        void insertMedia( const QString& );
        void insertMedia( const KURL& );
        void insertMedia( const KURL::List&, bool clearList = false, bool directPlay = false );
        void restoreSessionPlaylist();
        bool isAnotherTrack() const;

        void setFont( const QFont& );
        void setColors( const QPalette&, const QColor& );

        void createGUI(); //should be private but App::slowConfigToolbars requires it

        Playlist *playlist() const { return m_playlist; }

        ContextBrowser *m_contextBrowser;

        virtual bool eventFilter( QObject*, QEvent* );

    private slots:
        void savePlaylist() const;
        void slotAddLocation();

    private:
        BrowserBar *m_browsers;
        Playlist   *m_playlist;
        KLineEdit  *m_lineEdit;
        KToolBar   *m_toolbar;
};


inline
void PlaylistWindow::insertMedia( const QString &path )
{
    insertMedia( KURL::fromPathOrURL( path ) );
}

inline
void PlaylistWindow::insertMedia( const KURL &url )
{
    if( !url.isEmpty() )
    {
        insertMedia( KURL::List( url ) );
    }
}


#endif //AMAROK_PLAYLISTWINDOW_H

