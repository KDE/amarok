/***************************************************************************
                          browserwin.h  -  description
                             -------------------
    begin                : Fre Nov 15 2002
    copyright            : (C) 2002 by Mark Kretschmann
    email                :
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

#include <qwidget.h>
#include <qpixmap.h>

class BrowserWidget;
class PlaylistItem;
class PlaylistWidget;
class ExpandButton;

class QCloseEvent;
class QColor;
class QListViewItem;
class QPoint;
class QSplitter;

class KActionCollection;
class KLineEdit;
class KListView;
class KURL;

class PlayerApp;
extern PlayerApp *pApp;

/**
 *@author mark
 */

class BrowserWin : public QWidget
{
    Q_OBJECT
        public:
        BrowserWin( QWidget *parent=0, const char *name=0);
        ~BrowserWin();

        bool isFileValid( const KURL &url );
// ATTRIBUTES ------
        KActionCollection *m_pActionCollection;

        ExpandButton *m_pButtonAdd;

        ExpandButton *m_pButtonClear;
        ExpandButton *m_pButtonShuffle;
        ExpandButton *m_pButtonSave;

        ExpandButton *m_pButtonUndo;
        ExpandButton *m_pButtonRedo;

        ExpandButton *m_pButtonPlay;
        ExpandButton *m_pButtonPause;
        ExpandButton *m_pButtonStop;
        ExpandButton *m_pButtonNext;
        ExpandButton *m_pButtonPrev;

        BrowserWidget* m_pBrowserWidget;
        PlaylistWidget* m_pPlaylistWidget;
        QSplitter *m_pSplitter;
        KLineEdit *m_pBrowserLineEdit;
        KLineEdit *m_pPlaylistLineEdit;

    public slots:
        void slotBrowserDoubleClicked( QListViewItem *pItem );
        void slotShufflePlaylist();
        void slotBrowserDrop();
        void slotPlaylistRightButton( QListViewItem *pItem, const QPoint &rPoint );
        void slotShowInfo();
        void slotMenuPlay();
        void slotKeyUp();
        void slotKeyDown();
        void slotKeyPageUp();
        void slotKeyPageDown();
        void slotKeyEnter();
        void slotKeyDelete();

        signals:
        void signalHide();

    private:
        void initChildren();
        void closeEvent( QCloseEvent *e );

// ATTRIBUTES ------
        QColor m_TextColor;
        QPixmap m_bgPixmap;
};
#endif
