/***************************************************************************
 * copyright            : (C) 2005 Seb Ruiz <me@sebruiz.net>               *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_QUEUEMANAGER_H
#define AMAROK_QUEUEMANAGER_H

#include "playlistitem.h"

#include <kdialogbase.h>    //baseclass
#include <klistview.h>      //baseclass

#include <qmap.h>

class KPushButton;

class QueueItem : public KListViewItem
{
    public:
        QueueItem( QListView *parent, QListViewItem *after, QString t )
            : KListViewItem( parent, after, t )
        { };

        void paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int align );

};

class QueueList : public KListView
{
        Q_OBJECT

    friend class QueueManager;

    public:
        QueueList( QWidget *parent, const char *name = 0 );
        ~QueueList() {};

        bool    hasSelection();
        bool    isEmpty() { return ( childCount() == 0 ); }
        QPtrList<QListViewItem>  selectedItems();

    public slots:
        void    moveSelectedUp();
        void    moveSelectedDown();
        void    removeSelected();
        virtual void    clear();

    private:
        void    contentsDragEnterEvent( QDragEnterEvent *e );
        void    contentsDragMoveEvent( QDragMoveEvent* e );
        void    contentsDropEvent( QDropEvent *e );
        void    keyPressEvent( QKeyEvent *e );
        void    viewportPaintEvent( QPaintEvent* );

     signals:
        void    changed();
};

class QueueManager : public KDialogBase
{
        Q_OBJECT

    public:
        QueueManager( QWidget *parent = 0, const char *name = 0 );
        ~QueueManager();

        QPtrList<PlaylistItem> newQueue();

        static QueueManager *instance() { return s_instance; }

    public slots:
        void    applyNow();
        void    addItems( QListViewItem *after = 0 ); /// For the add button (uses selected playlist tracks)
        void    changeQueuedItems( const PLItemList &in, const PLItemList &out );  /// For keeping queue/dequeue in sync
        void    updateButtons();

    private slots:
        void    removeSelected();
        void    changed();

    private:
        void    insertItems();
        void    addQueuedItem( PlaylistItem *item );
        void    removeQueuedItem( PlaylistItem *item );

        QMap<QListViewItem*, PlaylistItem*> m_map;
        QueueList   *m_listview;
        KPushButton *m_up;
        KPushButton *m_down;
        KPushButton *m_remove;
        KPushButton *m_add;
        KPushButton *m_clear;

        static QueueManager *s_instance;
};

#endif /* AMAROK_QUEUEMANAGER_H */
