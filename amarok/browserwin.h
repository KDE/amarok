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


// CLASS PlaylistSideBar ==============================================================

//In order to remember the size of the tabs when using a QSplitter it is
//necessary to override sizeHint(). Hence this class.
//Later it seemed convenient to move management of the widgets (pages)
//here too, so I did that too.

#include <qhbox.h>       //baseclass
#include <qptrlist.h>    //stack allocated
#include <qpushbutton.h> //baseclass

class KMultiTabBar;
class QSignalMapper;
class QResizeEvent;
class QPushButton;

static const char* const not_close_xpm[]={
"5 5 2 1",
"# c black",
". c None",
"#####",
"#...#",
"#...#",
"#...#",
"#####"};


class PlaylistSideBar : public QHBox
{
Q_OBJECT

public:
    PlaylistSideBar( QWidget *parent );
    ~PlaylistSideBar();

    void setFont( const QFont& );
    void addPage( QWidget*, const QString&, const QString& );
    QWidget *page( const QString& );
    virtual QSize sizeHint() const;

public slots:
    void showHidePage( int );
    void close();
    void autoClosePages();

private:
    static const int DefaultHeight = 50;

    KMultiTabBar     *m_multiTabBar;
    QWidget          *m_pageHolder;
    QPushButton      *m_stayButton;
    QSignalMapper    *m_mapper;
    QPtrList<QWidget> m_pages;

    virtual void  resizeEvent( QResizeEvent * );


    // CLASS DockButton =================

    class TinyButton : public QPushButton
    {
    public:
        TinyButton( QWidget * = 0 );

    protected:
        virtual void drawButton( QPainter * );
        virtual void enterEvent( QEvent * );
        virtual void leaveEvent( QEvent * );

    private:
        bool m_mouseOver;
    };
};



// CLASS BrowserWin =====================================================================

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
        void insertMedia( const QString&, bool = false );
        void insertMedia( const KURL&, bool = false );
        void insertMedia( const KURL::List&, bool = false );
        bool isAnotherTrack() const;

        void setFont( const QFont& );
        void setColors( const QPalette&, const QColor& );
        void saveConfig();

        KActionCollection *m_pActionCollection;

    private slots:
        void savePlaylist() const;
        void slotAddLocation();

    private:
        bool    eventFilter( QObject*, QEvent* );
        QString defaultPlaylistPath() const; //inline convenience function

        QSplitter       *m_splitter;
        PlaylistSideBar *m_sideBar;
        PlaylistWidget  *m_playlist;
        KLineEdit       *m_lineEdit;
};


inline
void BrowserWin::insertMedia( const QString &path, bool b )
{
    KURL url;
    url.setPath( path );
    insertMedia( KURL::List( url ), b );
}

inline
void BrowserWin::insertMedia( const KURL &url, bool b )
{
    if( !url.isEmpty() )
    {
        insertMedia( KURL::List( url ), b );
    }
}

#endif
