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
class QFocusEvent;
class QKeyEvent;
class QListViewItem;
class QPaintEvent;
class QPoint;
class QRect;
class QString;
class QTimer;
class QEvent;

class MetaBundle;
class TagReader;


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


class PlaylistWidget : private KListView
{
    Q_OBJECT
    public:
        PlaylistWidget( QWidget* = 0, const char* = 0 );
        ~PlaylistWidget();

        void insertMedia( const KURL::List& );
        void saveM3u( const QString& ) const;
        bool isEmpty() const { return childCount() == 0; }
        bool isAnotherTrack() const;

        //made public for convenience
        void setFont( const QFont &f ) { KListView::setFont( f ); }
        void setColors( const QPalette &p, const QColor &c ) { setPalette( p ); setAlternateBackground( c ); }

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

        friend class PlaylistItem; //NOTE only needed in dtor, but saves compilation dependency on header
        friend BrowserWin::BrowserWin( QWidget*, const char* );
        friend bool BrowserWin::eventFilter( QObject*, QEvent* );

    signals:
        void playRequest( const KURL&, const MetaBundle& );
        void aboutToClear();

    public slots:
        void handleOrderPrev();
        void handleOrderCurrent();
        void handleOrder( PlaylistWidget::RequestType = Next );
        void clear();
        void shuffle();
        void removeSelectedItems();
        void copyToClipboard( const QListViewItem* = 0 ) const;

    private slots:
        void slotGlowTimer();
        void slotTextChanged( const QString& );
        void slotEraseMarker();

        void showContextMenu( QListViewItem*, const QPoint&, int );
        void activate( QListViewItem* );
        void setCurrentTrack( const KURL& );
        void writeTag( QListViewItem*, const QString&, int );

        void undo();
        void redo();
        void saveUndoState();

    private:
        PlaylistItem *restoreCurrentTrack();
        PlaylistItem *currentTrack() const { return m_currentTrack; }
        void setCurrentTrack( PlaylistItem* );
        void showTrackInfo( const PlaylistItem* ) const;
        void insertMediaInternal( const KURL::List&, QListViewItem* );
        bool saveState( QStringList& );
        void switchState( QStringList&, QStringList& );

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

        TagReader* const m_tagReader;

        QPushButton *m_undoButton;
        QPushButton *m_redoButton;

        QDir         m_undoDir;
        QStringList  m_undoList;
        QStringList  m_redoList;
        uint         m_undoCounter;
};
#endif
