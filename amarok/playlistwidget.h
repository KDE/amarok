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
#include <vector>

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

class PlayerApp;
extern PlayerApp *pApp;

// CLASS PlaylistWidget --------------------------------------------------------

class PlaylistWidget : public KListView
{
    Q_OBJECT
    public:
        PlaylistWidget( QWidget *parent=0, const char *name=0 );
        ~PlaylistWidget();

        bool setCurrentTrack( const KURL & );
        bool restoreCurrentTrack();
        //TODO make these private, then move all playlist management into playlist (playerapp.cpp will suffer)
        //TODO consider making the inheritance protected
        void setCurrentTrack( QListViewItem *item );
        QListViewItem* currentTrack() const;

        void setSorting( int, bool=true );

        void insertMedia( const QString & );
        void insertMedia( const KURL & );
        void insertMedia( const KURL::List &, bool=false );
        void insertMedia( const KURL::List &, QListViewItem * );

        void saveM3u( QString fileName );

// ATTRIBUTES ------
        //KRootPixmap m_rootPixmap;

    public slots:
        void clear();
        void shuffle();
        void removeSelectedItems();
        void doUndo();
        void doRedo();

    private slots:
        void slotGlowTimer();
        void slotEraseMarker();
        void slotTextChanged( const QString & );

    signals:
        void cleared();
        void sigUndoState( bool );
        void sigRedoState( bool );

    private:
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

        void unglowItems();

        void startLoader( const KURL::List &, QListViewItem * );

// ATTRIBUTES ------

        QTimer* m_GlowTimer;
        int m_GlowCount;
        int m_GlowAdd;
        QColor m_GlowColor;
        QListViewItem *m_pCurrentTrack;
        QRect m_marker;

        QDir m_undoDir;
        QStringList m_undoList;
        QStringList m_redoList;
        uint m_undoCounter;

        QStringList searchTokens;
        QPtrList<QListViewItem> searchPtrs;
        QString lastSearch;
};
#endif
