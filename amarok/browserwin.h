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


#include <qwidget.h>     //baseclass
#include <kurl.h>        //KUL::List

class PlaylistLoader;
class PlaylistSideBar;
class PlaylistWidget;
class KLineEdit;
class KActionCollection;
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

class BrowserWin : public QWidget
{
        Q_OBJECT

    public:
        BrowserWin( QWidget* = 0, const char* = 0 );
        ~BrowserWin();

        //convenience functions
        void insertMedia( const QString& );
        void insertMedia( const KURL& );
        void insertMedia( const KURL::List&, bool clearList = false, bool directPlay = false );
        bool isAnotherTrack() const;
        QString defaultPlaylistPath() const;

        void setFont( const QFont& );
        void setColors( const QPalette&, const QColor& );
        void saveConfig();

        KActionCollection *m_pActionCollection;

    private slots:
        void savePlaylist() const;
        void slotAddLocation();

    private:
        enum ButtonIds { id_addItem, id_playlistActions, id_undo, id_redo, id_play };
        
        bool    eventFilter( QObject*, QEvent* );

        QSplitter       *m_splitter;
        PlaylistSideBar *m_sideBar;
        PlaylistWidget  *m_playlist;
        KLineEdit       *m_lineEdit;
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
