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
class QPaintEvent;
class QPoint;
class QRect;
class QString;
class QTimer;

class MetaBundle;
class PlaylistBrowser;
class PlaylistLoader;
class ThreadWeaver;

/*
 * @author Mark Kretschmann && Max Howell
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

class PlaylistWidget : private KListView
{
    Q_OBJECT
    public:
        PlaylistWidget( QWidget* = 0, const char* = 0 );
        ~PlaylistWidget();

        void insertMedia( const KURL::List&, bool directPlay = false );
        void saveM3u( const QString& ) const;
        bool isEmpty() const { return childCount() == 0; }
        bool isAnotherTrack() const;
        QWidget *browser() const;

        //made public for convenience
        void setFont( const QFont &f ) { KListView::setFont( f ); }
        void setColors( const QPalette &p, const QColor &c ) { setPalette( p ); setAlternateBackground( c ); }

        static const int NO_SORT = 200;

        enum RequestType { Prev = -1, Current = 0, Next = 1 };
        enum ColumnType  { Trackname = 0,
                           Title = 1,
                           Artist = 2,
                           Album = 3,
                           Year = 4,
                           Comment = 5,
                           Genre = 6,
                           Track = 7,
                           Directory = 8,
                           Length = 9,
                           Bitrate = 10 };

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

    private slots:
        void slotGlowTimer();
        void slotTextChanged( const QString& );
        void slotEraseMarker();
        void slotMouseButtonPressed( int, QListViewItem*, const QPoint&, int );
        void showContextMenu( QListViewItem*, const QPoint&, int );
        void activate( QListViewItem* );
        void writeTag( QListViewItem*, const QString&, int );
        void handleStreamMeta( const MetaBundle& );

        void undo();
        void redo();
        void saveUndoState();

    private:
        PlaylistItem *restoreCurrentTrack();
        PlaylistItem *currentTrack() const { return m_currentTrack; }
        void setCurrentTrack( PlaylistItem* );
        void showTrackInfo( PlaylistItem* ) const;
        void insertMediaInternal( const KURL::List&, QListViewItem* );
        bool saveState( QStringList& );
        void switchState( QStringList&, QStringList& );
        void readAudioProperties( PlaylistItem* );

// REIMPLEMENTED ------
        void contentsDropEvent( QDropEvent* );
        void contentsDragEnterEvent( QDragEnterEvent* );
        void contentsDragMoveEvent( QDragMoveEvent* );
        void contentsDragLeaveEvent( QDragLeaveEvent* );
        void viewportPaintEvent( QPaintEvent* );
        void customEvent( QCustomEvent* );
        bool eventFilter( QObject*, QEvent* );
        void setSorting( int, bool=true );

// ATTRIBUTES ------
        PlaylistBrowser *m_browser;

        QTimer* const m_GlowTimer; //FIXME allocate on stack
        int m_GlowCount;
        int m_GlowAdd;
        QColor m_GlowColor;

        PlaylistItem  *m_currentTrack; //this track is playing
        //mutable //TODO not supported by gcc 2.9.5
        PlaylistItem  *m_cachedTrack;  //we expect this to be activated next
        PlaylistItem  *m_nextTrack;    //the track to be played after the current track
        QListViewItem *m_marker;

        QStringList searchTokens;
        QPtrList<QListViewItem> searchPtrs;
        QPtrList<QListViewItem> recentPtrs;

        ThreadWeaver* const m_weaver;

        QPushButton *m_undoButton;
        QPushButton *m_redoButton;
        QPushButton *m_clearButton;

        QDir         m_undoDir;
        QStringList  m_undoList;
        QStringList  m_redoList;
        uint         m_undoCounter;

        bool directPlay;
};

#endif
