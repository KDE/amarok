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

#include <qpixmap.h>
#include <qwidget.h>

class BrowserWidget;
class ExpandButton;
class PlaylistItem;
class PlaylistWidget;
class StreamBrowser;

class QHideEvent;
class QCloseEvent;
class QColor;
class QListViewItem;
class QPaintEvent;
class QPoint;
class QMoveEvent;
class QSplitter;

class KActionCollection;
class KJanusWidget;
class KLineEdit;
class KHistoryCombo;
class KListView;
class KURL;

class PlayerApp;
extern PlayerApp *pApp;

/**
 *@author mark
 */

// CLASS BrowserWin =====================================================================

class BrowserWin : public QWidget
{
        Q_OBJECT

    public:
        BrowserWin( QWidget *parent = 0, const char *name = 0 );
        ~BrowserWin();

        void setPalettes( const QColor &, const QColor &, const QColor & );

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

        BrowserWidget *m_pBrowserWidget;
        PlaylistWidget *m_pPlaylistWidget;
        StreamBrowser *m_pStreamBrowser;
        
        QSplitter *m_pSplitter;
        KHistoryCombo *m_pBrowserLineEdit;
        KLineEdit *m_pPlaylistLineEdit;
        KJanusWidget *m_pJanusWidget;

    public slots:
        void slotBrowserDoubleClicked( QListViewItem *pItem );
        void slotUpdateFonts();
        void savePlaylist();

    private slots:
        void setBrowserURL( const KURL& ); //sets browser line edit to KURL
        void slotAddLocation();

    signals:
        void signalHide();

    private:
        void initChildren();
        void closeEvent( QCloseEvent * );
        void moveEvent( QMoveEvent * );
        void paintEvent( QPaintEvent * );
        void keyPressEvent( QKeyEvent * );

        // ATTRIBUTES ------
        //QColor m_TextColor;
        //QPixmap m_bgPixmap;
};

#endif
