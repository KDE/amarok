/***************************************************************************
                        Playlist.h  -  description
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

#ifndef AMAROK_PLAYLIST_H
#define AMAROK_PLAYLIST_H

#include "playlistwindow.h"  //friend
#include "engineobserver.h" //baseclass

#include <qstringlist.h> //stack allocated
#include <qptrlist.h>    //stack allocated
#include <qmap.h>        //stack allocated
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
class QTimer;

class KAction;
class KActionCollection;

class MetaBundle;
//class PlaylistBrowser;
class PlaylistItem;
class PlaylistLoader;
class ThreadWeaver;


/*
 * @authors Mark Kretschmann && Max Howell
 *
 * Playlist inherits KListView privately and thus is no longer a ListView
 * Instead it is a part of PlaylistWindow and they interact in harmony. The change
 * was necessary as it is too dangerous to allow public access to PlaylistItems
 * due to the multi-threading environment.
 *
 * Unfortunately, since QObject is now inaccessible you have to connect slots
 * via one of PlaylistWindow's friend members or in Playlist
 *
 * If you want to add new playlist type functionality you should implement it
 * inside this class or inside PlaylistWindow.
 *
 */


class Playlist : private KListView, public EngineObserver
{
    Q_OBJECT
    public:
        Playlist( QWidget*, KActionCollection*, const char* = 0 );
        ~Playlist();

        void insertMedia( const KURL::List&, bool directPlay = false );
        bool isEmpty() const { return childCount() == 0; }
        bool isTrackBefore() const;
        bool isTrackAfter() const;

        void saveM3U( const QString& ) const;
        void saveXML( const QString& ) const;

        #ifdef PLAYLIST_BROWSER
        QWidget *browser() const;
        #endif

        class QDragObject *dragObject();

        //made public for convenience
        void setFont( const QFont &f ) { KListView::setFont( f ); }

        /** Converts physical PlaylistItem column position to logical */
        int mapToLogicalColumn( int physical );
        
        //static
        static const int NO_SORT = 200;
        static QString defaultPlaylistPath();

        //enums, typedefs and friends
        enum RequestType { Prev = -1, Current = 0, Next = 1 };

        friend class PlaylistItem;
        friend class PlaylistLoader;
        friend PlaylistWindow::PlaylistWindow( QWidget*, const char* );   //setting up connections etc.
        friend bool PlaylistWindow::eventFilter( QObject*, QEvent* ); //for convenience we handle some playlist events here

    signals:
        void aboutToClear();
        void itemCountChanged(int newCount);

    public slots:
        void insertMedia( const KURL &u ) { insertMedia( KURL::List(u), false ); }
        void handleOrderPrev(); //DEPRECATE
        void handleOrderCurrent(); //DEPRECATE
        void handleOrder( Playlist::RequestType = Next ); //DEPRECATE
        void clear();
        void shuffle();
        void removeSelectedItems();
        void copyToClipboard( const QListViewItem* = 0 ) const;
        void showCurrentTrack();
        void undo();
        void redo();

    private slots:
        void slotGlowTimer();
        void slotTextChanged( const QString& );
        void slotEraseMarker();
        void slotMouseButtonPressed( int, QListViewItem*, const QPoint&, int );
        void showContextMenu( QListViewItem*, const QPoint&, int );
        void activate( QListViewItem*, bool rememberTrack = true );
        void writeTag( QListViewItem*, const QString&, int );
        void slotHeaderResized( int, int, int );
        void saveUndoState();

    private:
        PlaylistItem *restoreCurrentTrack();
        PlaylistItem *currentTrack() const { return m_currentTrack; }
        void setCurrentTrack( PlaylistItem* );
        void showTrackInfo( PlaylistItem* ) const;
        void insertMediaInternal( const KURL::List&, PlaylistItem*, bool = false );
        bool saveState( QStringList& );
        void switchState( QStringList&, QStringList& );
        void readAudioProperties( PlaylistItem* );
        void removeItem( PlaylistItem* );
        void refreshNextTracks( int=-1 );

        //engine observer functions
        void engineNewMetaData( const MetaBundle&, bool );
        void engineStateChanged( EngineBase::EngineState );

// REIMPLEMENTED ------
        void contentsDropEvent( QDropEvent* );
        void contentsDragEnterEvent( QDragEnterEvent* );
        void contentsDragMoveEvent( QDragMoveEvent* );
        void contentsDragLeaveEvent( QDragLeaveEvent* );
        #ifdef PURIST
        //KListView imposes hand cursor so override it
        void contentsMouseMoveEvent( QMouseEvent *e ); { QListView::contentsMouseMoveEvent( e ); }
        #endif
        void paletteChange( const QPalette& );
        void viewportPaintEvent( QPaintEvent* );
        void customEvent( QCustomEvent* );
        bool eventFilter( QObject*, QEvent* );
        void setSorting( int, bool=true );
        void setColumnWidth( int, int );
        PlaylistItem *firstChild() const { return (PlaylistItem*)KListView::firstChild(); }
        PlaylistItem *lastItem() const { return (PlaylistItem*)KListView::lastItem(); }

// ATTRIBUTES ------
        #ifdef PLAYLIST_BROWSER
        PlaylistBrowser *m_browser;
        #endif

        PlaylistItem  *m_currentTrack; //the track that is playing
        PlaylistItem  *m_cachedTrack;  //we expect this to be activated next //FIXME mutable
        QListViewItem *m_marker;       //track that has the drag/drop marker under it

        //NOTE these container types were carefully chosen
        QString                    m_lastSearch; //the last search token
        QPtrList<PlaylistItem>     m_prevTracks; //the previous history
        QPtrList<PlaylistItem>     m_nextTracks; //the tracks to be played after the current track

        QTimer*       const m_glowTimer;
        ThreadWeaver* const m_weaver;

        KAction *m_undoButton;
        KAction *m_redoButton;
        KAction *m_clearButton;

        QDir         m_undoDir;
        QStringList  m_undoList;
        QStringList  m_redoList;
        uint         m_undoCounter;

        KActionCollection* const m_ac;
};


#endif //AMAROK_PLAYLIST_H



