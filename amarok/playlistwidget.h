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

#include <qdir.h>
#include <qstringlist.h>    // <markey> forward declaration of QStringList does not work with some compilers
#include <klistview.h>
#include <kurl.h>

#include "playlistitem.h" //friend

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

#define COL_TRACKNAME 0
#define COL_TITLE   1
#define COL_ARTIST  2
#define COL_ALBUM   3
#define COL_YEAR    4
#define COL_COMMENT 5
#define COL_GENRE   6
#define COL_TRACK   7
#define COL_DIR     8
#define COL_LENGTH  9
#define COL_BITRATE 10


//FIXME <mxcl> I would very much like to make this inherit KListView privately as it scares me having multiple threads and public access!
//amarok/playerapp.cpp: In member function `void PlayerApp::initBrowserWin()':
//amarok/playerapp.cpp:479: `QObject' is an inaccessible base of `PlaylistWidget'
//make: *** [../amarok/amarok/playerapp.o] Fehler 1
//FIXME 479: connect( m_pBrowserWin->m_pPlaylistWidget, SIGNAL( activated( const KURL&, const MetaBundle * ) ), this, SLOT( play( const KURL&, const MetaBundle * ) ) );
//FIXME the protected/private inheritance is necessary as NO other classes can have access to QListViewItems!!

class PlaylistWidget : public KListView //: private KListView
{
    Q_OBJECT
    public:
        PlaylistWidget( QWidget *parent=0, const char *name=0 );
        ~PlaylistWidget();

        enum RequestType { Prev, Current, Next };

        bool request( RequestType = Next, bool = true );
        KURL currentTrackURL() const { return currentTrack() ? m_pCurrentTrack->url() : KURL(); }
        QString currentTrackName() const { return currentTrack() ? currentTrack()->text( 0 ) : QString(); }

        void insertMedia( const QString & );
        void insertMedia( const KURL & );
        void insertMedia( const KURL::List &, bool=false );

        void saveM3u( QString fileName );

    //TEMPORARY re-public
        void setFont( const QFont &font ) { KListView::setFont( font ); }
        bool hasFocus() const { return KListView::hasFocus(); }
        void setFocus() { KListView::setFocus(); }
        void triggerUpdate() { KListView::triggerUpdate(); }

        friend PlaylistItem::~PlaylistItem();

// ATTRIBUTES ------
        //KRootPixmap m_rootPixmap;

    public slots:
        void clear( bool = true );
        void shuffle();
        void removeSelectedItems();
        void doUndo();
        void doRedo();
        void copyAction( QListViewItem* = 0 );

    private slots:
        void slotGlowTimer();
        void slotEraseMarker();
        void slotTextChanged( const QString & );
        void slotReturnPressed();
        void showContextMenu( QListViewItem *, const QPoint &, int );
        void activate( QListViewItem * );
        void writeTag( QListViewItem *, const QString &, int );

    signals:
        void activated( const KURL&, const MetaBundle& );
        void cleared();
        void sigUndoState( bool );
        void sigRedoState( bool );

    private:
        void insertMedia( const KURL::List &, PlaylistItem * );

        PlaylistItem *restoreCurrentTrack();
        PlaylistItem *currentTrack() const { return m_pCurrentTrack; }
        void setCurrentTrack( PlaylistItem * );
        void showTrackInfo( const PlaylistItem * );

        void contentsDropEvent( QDropEvent* );
        void contentsDragEnterEvent( QDragEnterEvent* );
        void contentsDragMoveEvent( QDragMoveEvent* );
        void contentsDragLeaveEvent( QDragLeaveEvent* );
        void keyPressEvent( QKeyEvent* );
        void viewportPaintEvent( QPaintEvent* );
        void customEvent( QCustomEvent * );
        bool eventFilter( QObject *, QEvent * );

        bool saveState( QStringList& );
        void writeUndo();
        void initUndo();
        bool canUndo();
        bool canRedo();

        void startLoader( const KURL::List &, PlaylistItem * );

        void setSorting( int, bool=true );

// ATTRIBUTES ------

        QTimer* m_GlowTimer; //FIXME allocate on stack
        int m_GlowCount;
        int m_GlowAdd;
        QColor m_GlowColor;
        PlaylistItem *m_pCurrentTrack;
        QRect m_marker;

        QDir m_undoDir;
        QStringList m_undoList;
        QStringList m_redoList;
        uint m_undoCounter;

        QStringList searchTokens;
        QPtrList<QListViewItem> searchPtrs;
        QString lastSearch;

        TagReader *m_tagReader;
};
#endif
