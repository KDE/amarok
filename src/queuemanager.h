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

#ifndef QUEUEMANAGER_H
#define QUEUEMANAGER_H


#include <playlist.h>
#include <playlistitem.h>

#include <kdialogbase.h>    //baseclass
#include <klistview.h>      //baseclass
#include <kpushbutton.h>

#include <qmap.h>

class QueueList : public KListView
{
        Q_OBJECT

    public:
        QueueList( QWidget *parent, const char *name = 0 );
        ~QueueList() {};

        bool    hasSelection();

    public slots:
        void    moveSelectedUp();
        void    moveSelectedDown();
        void    removeSelected();

    private:
        void    contentsDragEnterEvent( QDragEnterEvent *e );
        void    contentsDropEvent( QDropEvent *e );
        void    keyPressEvent( QKeyEvent *e );
        void    viewportPaintEvent( QPaintEvent* );
};

class QueueManager : public KDialogBase
{
        Q_OBJECT

    public:
        QueueManager( QWidget *parent = 0, const char *name = 0 );
        ~QueueManager() {};

        QPtrList<PlaylistItem> newQueue();

    private slots:
        void    updateButtons();

    private:
        void    insertItems();

        QMap<QListViewItem*, PlaylistItem*> m_map;
        QueueList *m_listview;
        KPushButton *m_up;
        KPushButton *m_down;
        KPushButton *m_remove;
};

#endif /* QUEUEMANAGER_H */
