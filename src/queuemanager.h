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

#include <qmap.h>

class QueueList : public KListView
{
        Q_OBJECT

    public:
        QueueList( QWidget *parent, const char *name = 0 );
        ~QueueList() {};

    private:
        void viewportPaintEvent( QPaintEvent* );
        void eraseMarker();

        QListViewItem *m_marker;
};

class QueueManager : public KDialogBase
{
    public:
        QueueManager( QWidget *parent = 0, const char *name = 0 );
        ~QueueManager() {};

        QPtrList<PlaylistItem> newQueue();

    private:
        void  insertItems();

        QMap<QListViewItem*, PlaylistItem*> m_map;
        QueueList *m_listview;
};

#endif /* QUEUEMANAGER_H */
