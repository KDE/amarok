/***************************************************************************
                        playlistwidget.h  -  description
                            -------------------
    begin                : Don Dez 5 2002
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

#ifndef PLAYLISTWIDGET_H
#define PLAYLISTWIDGET_H

#include "browserwin.h"  //friend
#include "engine/engineobserver.h"

#include <qstringlist.h> //stack allocated
#include <qptrlist.h>    //stack allocated
#include <klistview.h>   //baseclass
#include <kurl.h>        //KURL::List
#include <qdir.h>        //stack allocated

class QColor;
class QCustomEvent;
class QDragEnterEvent;
class QDragLeaveEvent;
class QDragMoveEvent;
class QDropEvent;
class QEvent;
class QFocusEvent;
class QKeyEvent;
class QListViewItem;
class QPalette;
class QPaintEvent;
class QPoint;
class QRect;
class QString;

class KAction;
class KActionCollection;

class MetaBundle;
class PlaylistBrowser;
class PlaylistLoader;
class ThreadWeaver;

/*
 * @authors Mark Kretschmann && Max Howell
 *
 * PlaylistWidget inherits KListView privately and thus is no longer a ListView
 * Instead it is a part of BrowserWin and they interact in harmony. The change
 * was necessary as it is too dangerous to allow public access to PlaylistItems
 * due to the multi-threading environment.
 *
 * Unfortunately, since QObject is now inaccessible you have to connect slots
 * via one of BrowserWin's friend members or in PlaylistWidget
 *
 * If you want to add new playlist type functionality you should implement it
 * inside this class or inside BrowserWin.
 *
 */

/*******
 * TODO
 *
 * The engine and playlist should be more linked, through a few signals or direct functions IMO.
 * Then the engine says, isThereMoreTracks(), gimmeNextTrack() and it has a series of signals:
 * play( const MetaBundle& ), stopped(), etc. that all UI bits can connect to. But the link between the
 * playlist and the engine needs to be concrete.
 * The playlist should offer out next/prev/play/undo/redo as KActions.
 * We'd have to derive our own KAction class so we can plug QPushButtons into layouts, etc.
 *
 ***/

class PlaylistWidget : private KListView, public EngineObserver
{
    Q_OBJECT
    public:
        PlaylistWidget( QWidget*, KActionCollection*, const char* = 0 );
        ~PlaylistWidget();

        void insertMedia( const KURL::List&, bool directPlay = false );
        bool isEmpty() const { return childCount() == 0; }
        bool isTrackBefore() const;
        bool isTrackAfter() const;

        void saveM3U( const QString& ) const;
        void saveXML( const QString& ) const;

        QWidget *browser() const;

        //made public for convenience
        void setFont( const QFont &f ) { KListView::setFont( f ); }

        static const int NO_SORT = 200;
        static QString defaultPlaylistPath();

        enum RequestType { Prev = -1, Current = 0, Next = 1 };

        friend class PlaylistItem;
        friend class PlaylistLoader;
        friend BrowserWin::BrowserWin( QWidget*, const char* );   //setting up connections etc.
        friend bool BrowserWin::eventFilter( QObject*, QEvent* ); //for convenience we handle some playlist events here
    signals:
        void playRequest( const MetaBundle& );
        void aboutToClear();

    public slots:
        void handleOrderPrev();
        void handleOrderCurrent();
        void handleOrder( PlaylistWidget::RequestType = Next );
        void clear();
        void shuffle();
        void removeSelectedItems();
        void copyToClipboard( const QListViewItem* = 0 ) const;
        void showCurrentTrack();
        void setCurrentTrack( const KURL& );
        void undo();
        void redo();

    protected:
        void engineNewMetaData( const MetaBundle &/*bundle*/, bool /*trackChanged*/ );

    private slots:
        void slotGlowTimer();
        void slotTextChanged( const QString& );
        void slotEraseMarker();
        void slotMouseButtonPressed( int, QListViewItem*, const QPoint&, int );
        void showContextMenu( QListViewItem*, const QPoint&, int );
        void activate( QListViewItem* );
        void writeTag( QListViewItem*, const QString&, int );

        void saveUndoState();

    private:
        PlaylistItem *restoreCurrentTrack();
        PlaylistItem *currentTrack() const { return m_currentTrack; }
        void setCurrentTrack( PlaylistItem* );
        void showTrackInfo( PlaylistItem* ) const;
        void insertMediaInternal( const KURL::List&, QListViewItem*, bool = false );
        bool saveState( QStringList& );
        void switchState( QStringList&, QStringList& );
        void readAudioProperties( PlaylistItem* );
        void removeItem( PlaylistItem* );
        void refreshNextTracks( int=-1 );

// REIMPLEMENTED ------
        void contentsDropEvent( QDropEvent* );
        void contentsDragEnterEvent( QDragEnterEvent* );
        void contentsDragMoveEvent( QDragMoveEvent* );
        void contentsDragLeaveEvent( QDragLeaveEvent* );
        //void contentsMouseMoveEvent( QMouseEvent *e ); { QListView::contentsMouseMoveEvent( e ); } //KListView imposes hand cursor so override it
        void viewportPaintEvent( QPaintEvent* );
        void customEvent( QCustomEvent* );
        bool eventFilter( QObject*, QEvent* );
        void setSorting( int, bool=true );
        void setColumnWidth( int, int );
        PlaylistItem *firstChild() const { return (PlaylistItem*)KListView::firstChild(); }

// ATTRIBUTES ------
        PlaylistBrowser *m_browser;

        int m_GlowCount;
        int m_GlowAdd;

        PlaylistItem  *m_currentTrack; //this track is playing
        PlaylistItem  *m_cachedTrack;  //we expect this to be activated next //FIXME mutable
        QPtrList<PlaylistItem> m_nextTracks;    //the tracks to be played after the current track
        QListViewItem *m_marker;

        QStringList searchTokens;
        QPtrList<QListViewItem> searchPtrs;
        QPtrList<QListViewItem> recentPtrs;

        ThreadWeaver* const m_weaver;

        KAction *m_undoButton;
        KAction *m_redoButton;
        KAction *m_clearButton;

        QDir         m_undoDir;
        QStringList  m_undoList;
        QStringList  m_redoList;
        uint         m_undoCounter;
};

#endif
