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

#ifndef BROWSERWIN_H
#define BROWSERWIN_H


#include <qwidget.h>        //baseclass
#include <kurl.h>           //KUL::List
#include <kxmlguiclient.h>  //baseclass (for XMLGUI)

class BrowserBar;
class KLineEdit;
class KActionCollection;
class KToolBar;
class PlaylistLoader;
class PlaylistWidget;
class QColor;
class QCloseEvent;
class QCustomEvent;
class QFont;
class QListViewItem;
class QPalette;
class QPoint;
class QPushButton;
class QSplitter;
class QString;

class BrowserWin : public QWidget, public KXMLGUIClient
{
        Q_OBJECT

    public:
        BrowserWin( QWidget* = 0, const char* = 0 );

        //convenience functions
        void insertMedia( const QString& );
        void insertMedia( const KURL& );
        void insertMedia( const KURL::List&, bool clearList = false, bool directPlay = false );
        void restoreSessionPlaylist();
        bool isAnotherTrack() const;

        void setFont( const QFont& );
        void setColors( const QPalette&, const QColor& );
        void saveConfig();

        KToolBar *createGUI(); //should be private but PlayerApp::slowConfigToolbars requires it

        PlaylistWidget *playlist() const { return m_playlist; }

        virtual bool eventFilter( QObject*, QEvent* );

    private slots:
        void savePlaylist() const;
        void slotAddLocation();

    private:
        BrowserBar     *m_browsers;
        PlaylistWidget *m_playlist;
        KLineEdit      *m_lineEdit;
};


inline
void BrowserWin::insertMedia( const QString &path )
{
    insertMedia( KURL::fromPathOrURL( path ) );
}

inline
void BrowserWin::insertMedia( const KURL &url )
{
    if( !url.isEmpty() )
    {
        insertMedia( KURL::List( url ) );
    }
}

#endif
