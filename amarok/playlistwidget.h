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
#include <klistview.h>
//#include <krootpixmap.h>
#include <kurl.h>

#include "playlistitem.h" //friend

class QDragLeaveEvent;
class QDragMoveEvent;
class QKeyEvent;
class QDropEvent;
class QFocusEvent;
class QPaintEvent;
class QCustomEvent;
class QListViewItem;
class QPoint;
class QRect;
class QString;
class QStringList;
class QColor;
class QTimer;

struct Tags;

class TagReader;
class PlayerApp;
extern PlayerApp *pApp;



class PlaylistWidget : public KListView
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
        void insertMedia( const KURL::List &, QListViewItem * );

        void saveM3u( QString fileName );

    //TEMPORARY re-public
        void setFont( const QFont &font ) { KListView::setFont( font ); }
        bool hasFocus() const { return KListView::hasFocus(); }
        void setFocus() { KListView::setFocus(); }
        void setPaletteBackgroundColor( const QColor &c ) { KListView::setPaletteBackgroundColor( c ); }
        void setPaletteForegroundColor( const QColor &c ) { KListView::setPaletteForegroundColor( c ); }
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

    private slots:
        void slotGlowTimer();
        void slotEraseMarker();
        void slotTextChanged( const QString & );
        void showContextMenu( QListViewItem *, const QPoint & );
        void activate( QListViewItem * );

    signals:
        void activated( const KURL&, const Tags * );
        void cleared();
        void sigUndoState( bool );
        void sigRedoState( bool );

    private:
        PlaylistItem *restoreCurrentTrack();
        PlaylistItem *currentTrack() const { return m_pCurrentTrack; }        
        void setCurrentTrack( QListViewItem * );
        void showTrackInfo( const PlaylistItem * );

        void contentsDropEvent( QDropEvent* );
        void contentsDragMoveEvent( QDragMoveEvent* );
        void contentsDragLeaveEvent( QDragLeaveEvent* );
        void keyPressEvent( QKeyEvent* );
        void viewportPaintEvent( QPaintEvent* );
        void customEvent( QCustomEvent * );

        bool saveState( QStringList& );
        void writeUndo();
        void initUndo();
        bool canUndo();
        bool canRedo();

        void startLoader( const KURL::List &, QListViewItem * );
        
        void setSorting( int, bool=true );

// ATTRIBUTES ------

        QTimer* m_GlowTimer;
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
