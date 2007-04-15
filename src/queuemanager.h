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

#include <kdialog.h>    //baseclass
#include <QListWidget>      //baseclass
#include <QListWidgetItem> //baseclass

#include <QMap>

class KPushButton;
class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class QKeyEvent;
class QPaintEvent;

class QueueItem : public QListWidgetItem
{

     public:
        QueueItem( const QString& s ) : QListWidgetItem( s ) { }
#if 0
        void paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int align );
#endif
};

class QueueList : public QListWidget
{
        Q_OBJECT

    friend class QueueManager;

    public:
        explicit QueueList( QWidget *parent, const char *name = 0 );
        ~QueueList() {};

        bool    hasSelection();
        bool    isEmpty() { return ( count() == 0 ); }

    public slots:
        void    moveSelectedUp() { moveSelected( -1 ); }
        void    moveSelectedDown() { moveSelected( 1 ); }
        void    removeSelected();
        virtual void    clear();

    private:
        void    moveSelected( int direction );
        void    dragEnterEvent( QDragEnterEvent *e );
        void    dragMoveEvent( QDragMoveEvent* e );
        void    dropEvent( QDropEvent *e );
        void    keyPressEvent( QKeyEvent *e );
        void    paintEvent( QPaintEvent* );

     signals:
        void    changed();
};

class QueueManager : public KDialog
{
        Q_OBJECT

    public:
        explicit QueueManager( QWidget *parent = 0, const char *name = 0 );
        ~QueueManager();

        QList<PlaylistItem* > newQueue();

        static QueueManager *instance() { return s_instance; }

    public slots:
        void    applyNow();
        void    addItems( QListWidgetItem *after = 0 ); /// For the add button (uses selected playlist tracks)
        void    changeQueuedItems( const QList<PlaylistItem*> &in, const QList<PlaylistItem*> &out );  /// For keeping queue/dequeue in sync
        void    updateButtons();

    private slots:
        void    removeSelected();
        void    changed();

    private:
        void    insertItems();
        void    addQueuedItem( PlaylistItem *item );
        void    removeQueuedItem( PlaylistItem *item );

        QMap<QListWidgetItem*, PlaylistItem*> m_map;
        QueueList   *m_listview;
        KPushButton *m_up;
        KPushButton *m_down;
        KPushButton *m_remove;
        KPushButton *m_add;
        KPushButton *m_clear;

        static QueueManager *s_instance;
};

#endif /* AMAROK_QUEUEMANAGER_H */
