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

#include "engineobserver.h"  //baseclass
#include "playlistwindow.h"  //friend

#include <klistview.h>       //baseclass
#include <kurl.h>            //KURL::List
#include <qdir.h>            //stack allocated
#include <qstringlist.h>     //stack allocated
#include <qptrlist.h>        //stack allocated
#include <vector>            //stack allocated

class KAction;
class KActionCollection;
class MetaBundle;
class PlaylistItem;
class PlaylistLoader;
class QBoxLayout;
class QLabel;
class QTimer;


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
        ~Playlist();

        static const int Append     = 1;     /// inserts media after the last item in the playlist
        static const int Queue      = 2;     /// inserts media after the currentTrack
        static const int Clear      = 4;     /// clears the playlist first
        static const int Replace    = Clear;
        static const int DirectPlay = 8;     /// start playback of the first item in the list
        static const int Unique     = 16;    /// don't insert anything already in the playlist

        /** Add media to the playlist
         *  @param options you can OR these together, see the enum,
         */
        void insertMedia( KURL::List, int options = Append );

        bool isEmpty() const { return childCount() == 0; }
        bool isTrackBefore() const;
        bool isTrackAfter() const;

        /// called during initialisation
        void restoreSession();

        void saveM3U( const QString& ) const;
        void saveXML( const QString& );

        void burnPlaylist( int projectType=-1 );
        void burnSelectedTracks( int projectType=-1 );

        class QDragObject *dragObject();

        //made public for convenience
        void setFont( const QFont &f ) { KListView::setFont( f ); }
        void unsetFont() { KListView::unsetFont(); }

        /** Converts physical PlaylistItem column position to logical */
        int mapToLogicalColumn( int physical );

        //static
        static const int NO_SORT = 200;
        static QString defaultPlaylistPath();
        static Playlist *instance() { return s_instance; }

        //enums, typedefs and friends
        enum RequestType { Prev = -1, Current = 0, Next = 1 };

        friend class PlaylistItem;
        friend class PlaylistLoader;
        friend void PlaylistWindow::init(); //setting up connections etc.
        friend bool PlaylistWindow::eventFilter( QObject*, QEvent* ); //for convenience we handle some playlist events here

    signals:
        void aboutToClear();
        void itemCountChanged( int newCount, int newLength, int selCount, int selLength );

    public slots:
        void appendMedia( const QString &path );
        void appendMedia( const KURL &url );
        void clear();
        void shuffle();
        void removeSelectedItems();
        void deleteSelectedFiles();
        void removeDuplicates();
        void copyToClipboard( const QListViewItem* = 0 ) const;
        void showCurrentTrack() { ensureItemVisible( reinterpret_cast<QListViewItem*>(m_currentTrack) ); }
        void undo();
        void redo();
        void selectAll() { QListView::selectAll( true ); }
        void playPrevTrack();
        void playCurrentTrack();
        void playNextTrack();
        void updateMetaData( const MetaBundle& );
        void setFilter( const QString &filter );
        void scoreChanged( const QString &path, int score );
        void addCustomColumn();

    private slots:
        void slotGlowTimer();
        void slotEraseMarker();
        void slotMouseButtonPressed( int, QListViewItem*, const QPoint&, int );
        void slotSelectionChanged();
        void showContextMenu( QListViewItem*, const QPoint&, int );
        void writeTag( QListViewItem*, const QString&, int );
        void saveUndoState();
        void columnOrderChanged();
        void updateNextPrev();
        void queue( QListViewItem* );
        void activate( QListViewItem* );
        void columnResizeEvent( int, int, int );

    private:
        Playlist( QWidget*, KActionCollection*, const char* = 0 );
        Playlist( const Playlist& ); //not defined

        static Playlist *s_instance;

        PlaylistItem *restoreCurrentTrack();
        PlaylistItem *currentTrack() const { return m_currentTrack; }
        void setCurrentTrack( PlaylistItem* );
        void insertMediaInternal( const KURL::List&, PlaylistItem*, bool directPlay = false );
        bool saveState( QStringList& );
        void switchState( QStringList&, QStringList& );
        void removeItem( PlaylistItem* );
        void refreshNextTracks( int=-1 );
        void showTagDialog( QPtrList<QListViewItem> items );

        //engine observer functions
        void engineNewMetaData( const MetaBundle&, bool );
        void engineStateChanged( Engine::State );

// REIMPLEMENTED ------
        void contentsDropEvent( QDropEvent* );
        void contentsDragEnterEvent( QDragEnterEvent* );
        void contentsDragMoveEvent( QDragMoveEvent* );
        void contentsDragLeaveEvent( QDragLeaveEvent* );
        #ifdef PURIST
        //KListView imposes hand cursor so override it
        void contentsMouseMoveEvent( QMouseEvent *e ) { QListView::contentsMouseMoveEvent( e ); }
        #endif
        void paletteChange( const QPalette& );
        void viewportPaintEvent( QPaintEvent* );
        void viewportResizeEvent( QResizeEvent* );
        void customEvent( QCustomEvent* );
        bool eventFilter( QObject*, QEvent* );
        void setSorting( int, bool=true );
        void setColumnWidth( int, int );
        void rename( QListViewItem*, int );
        PlaylistItem *firstChild() const { return (PlaylistItem*)KListView::firstChild(); }
        PlaylistItem *lastItem() const { return (PlaylistItem*)KListView::lastItem(); }

// ATTRIBUTES ------
        PlaylistItem  *m_currentTrack; //the track that is playing
        QListViewItem *m_marker;       //track that has the drag/drop marker under it

        //NOTE these container types were carefully chosen
        QString                m_lastSearch; //the last search token
        QPtrList<PlaylistItem> m_prevTracks; //the previous history
        QPtrList<PlaylistItem> m_nextTracks; //the tracks to be played after the current track

        int m_firstColumn;
        int m_totalLength;
        int m_selectCounter;
        int m_selectLength;

        KAction *m_undoButton;
        KAction *m_redoButton;
        KAction *m_clearButton;

        QDir         m_undoDir;
        QStringList  m_undoList;
        QStringList  m_redoList;
        uint         m_undoCounter;

        KURL::List   m_queueList;
        bool         m_stopAfterCurrent;
        bool         m_showHelp;

        //text before inline editing ( the new tag is written only if it's changed )
        QString m_editOldTag;

        KActionCollection* const m_ac;

        std::vector<double> m_columnFraction;
};

#endif //AMAROK_PLAYLIST_H
