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

#include <qpixmap.h>
#include <qwidget.h>

#include <kaction.h>

class ExpandButton;
class KDevFileSelector;
class PlaylistItem;
class PlaylistWidget;
class StreamBrowser;
class PlaylistSideBar;

class QHideEvent;
class QCloseEvent;
class QColor;
class QListViewItem;
class QPoint;
class QSplitter;
class QVBox;

class KLineEdit;
class KListView;
class KURL;

class PlayerApp;
extern PlayerApp *pApp;


/**
 *@author Max
 */
 
//In order to remember the size of the tabs when using a QSplitter it is
//necessary to override sizeHint(). Hence this class.
//Later it seemed convenient to move management of the widgets (pages)
//here too, so I did that too.
 
#include <qhbox.h>
#include <vector>

class KMultiTabBar;
class QSignalMapper;

class PlaylistSideBar : public QHBox
{
Q_OBJECT

public:
    PlaylistSideBar( QWidget *parent );

    void setPageFont( const QFont& );
    void addPage( QWidget*, const QString&, const QString&, bool = false );
    QWidget *page( const QString& );
    virtual QSize sizeHint() const;

public slots:
    void showHide( int );
    void close();
    
private:
    int  m_savedSize;
    int  m_current;
    
    KMultiTabBar  *m_MTB;
    QSignalMapper *m_mapper;
    
    std::vector<QWidget *> m_widgets;
};



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

        PlaylistWidget *m_pPlaylistWidget;
        
        QSplitter *m_pSplitter;
        KLineEdit *m_pPlaylistLineEdit;
    
    public slots:
        void slotUpdateFonts();
        void savePlaylist();

    signals:
        void signalHide();

    private:
        void initChildren();
        void closeEvent( QCloseEvent * );
        void keyPressEvent( QKeyEvent * );
        
        PlaylistSideBar *m_pSideBar;

    private slots:
        void slotAddLocation();
                
};

#endif
