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

#include <klistview.h>
#include <kurl.h>

class PlaylistItem;

class QColor;
class QDragMoveEvent;
class QDropEvent;
class QFocusEvent;
class QPoint;
class QString;
class QStringList;
class QTimer;

class KDirLister;

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

// ATTRIBUTES ------

    public slots:
        void slotGlowTimer();
        void slotSetRecursive();
        void slotTextChanged( const QString &str );
        void slotHeaderClicked( int section );

    signals:
        void signalJump();
        void signalPlay();

    private:
        void contentsDragMoveEvent( QDragMoveEvent* e);
        void focusInEvent( QFocusEvent *e );

        void playlistDrop( KURL::List urlList );
        PlaylistItem* playlistInsertItem( KURL srcUrl, PlaylistItem* dstItem );
        PlaylistItem* playlistInsertDir( KURL srcUrl, PlaylistItem* dstItem );

// ATTRIBUTES ------
        KDirLister *m_pDirLister;
        PlaylistItem *m_pDropCurrentItem;
        int m_dropRecursionCounter;
        bool m_dropRecursively;

        QTimer* m_GlowTimer;
        QTimer* m_pTagTimer;
        int m_GlowCount, m_GlowAdd;
        QColor m_GlowColor;
        QListViewItem* m_pCurrentTrack;
};
#endif
