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
#include <qstringlist.h>
#include <qptrlist.h>

#include <klistview.h>
#include <krootpixmap.h>
#include <kurl.h>

class PlaylistItem;

class QColor;
class QDragLeaveEvent;
class QDragMoveEvent;
class QDropEvent;
class QFocusEvent;
class QListViewItem;
class QPaintEvent;
class PlaylistItem;
class QPoint;
class QRect;
class QString;
class QTextStream;
class QTimer;

class KDirLister;
class KURL;

class PlayerApp;
extern PlayerApp *pApp;

// CLASS PlaylistWidget --------------------------------------------------------

class PlaylistWidget : public KListView
{
    Q_OBJECT
    public:
        PlaylistWidget(QWidget *parent=0, const char *name=0);
        ~PlaylistWidget();

        QListViewItem* currentTrack();
        void setCurrentTrack( QListViewItem *item );
        void unglowItems();
        void triggerSignalPlay();
        PlaylistItem* addItem( PlaylistItem *after, KURL url );
        void contentsDropEvent( QDropEvent* e);
        bool loadPlaylist( KURL url, QListViewItem *destination );
        void saveM3u( QString fileName );

        void initUndo();
        void writeUndo();
        bool canUndo();
        bool canRedo();

// ATTRIBUTES ------
        KRootPixmap m_rootPixmap;

    public slots:
        void clear();
        void slotGlowTimer();
        void slotSetRecursive();
        void slotReturnPressed();
        void slotTextChanged( const QString &str );
        void slotHeaderClicked( int section );
        void slotEraseMarker();
        void doUndo();
        void doRedo();

    signals:
        void cleared();
        void signalJump();
        void signalPlay();
        void sigUndoState( bool );
        void sigRedoState( bool );

    private:
        void contentsDragMoveEvent( QDragMoveEvent* e );
        void contentsDragLeaveEvent( QDragLeaveEvent* );
        void focusInEvent( QFocusEvent *e );
        void viewportPaintEvent( QPaintEvent *e );

        void playlistDrop( KURL::List urlList );
        PlaylistItem* playlistInsertItem( KURL srcUrl, PlaylistItem* dstItem );
        PlaylistItem* playlistInsertDir( KURL srcUrl, PlaylistItem* dstItem );
        
        void loadM3u( QTextStream &stream, PlaylistItem *destItem, QString dir );
        void loadPls( QTextStream &stream, PlaylistItem *destItem, QString dir );
        bool loadPlaylist_( KURL url, QListViewItem *destination );

// ATTRIBUTES ------
        KDirLister *m_pDirLister;
        PlaylistItem *m_pDropCurrentItem;
        int m_dropRecursionCounter;
        bool m_dropRecursively;

        QTimer* m_GlowTimer;
        QTimer* m_pTagTimer;
        int m_GlowCount;
        int m_GlowAdd;
        QColor m_GlowColor;
        QListViewItem *m_pCurrentTrack;
        QRect m_marker;

        QDir m_undoDir;
        QStringList m_undoList;
        QStringList m_redoList;
        unsigned int m_undoCounter;

        QStringList searchTokens;
        QPtrList<QListViewItem> searchPtrs;
        QString lastSearch;
};
#endif
