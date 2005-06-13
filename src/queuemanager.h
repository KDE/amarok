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
#include <klistview.h>

#include <qmap.h>
#include <qptrlist.h>
#include <qvbox.h>

//////////////////////////////////////////////////////////////////////////////////////////
/// CLASS QueueManager
//////////////////////////////////////////////////////////////////////////////////////////

class QueueManager : public KDialogBase
{
    public:
        QueueManager( QWidget *parent = 0, const char *name = 0 );
        ~QueueManager() {};

        QPtrList<PlaylistItem> newQueue();

    private:
        void  insertItems();

        QMap<QListViewItem*, PlaylistItem*> m_map;
        KListView *m_listview;
};




QueueManager::QueueManager( QWidget *parent, const char *name )
                    : KDialogBase( parent, name, false, i18n("Queue Manager"), Ok|Cancel )
{
    makeVBoxMainWidget();

    QHBox *box = new QHBox( mainWidget() );
    box->setSpacing( 5 );
    m_listview = new KListView( box );
    m_listview->addColumn( i18n("Name") );
    m_listview->setResizeMode( QListView::LastColumn );
    m_listview->setSelectionMode( QListView::Extended );
    m_listview->setSorting( -1 );

    insertItems();
}

QPtrList<PlaylistItem>
QueueManager::newQueue()
{
    QPtrList<PlaylistItem> queue;
    for( QListViewItem *key = m_listview->firstChild(); key; key = key->nextSibling() )
    {
        queue.append( m_map[ key ] );
    }
    return queue;
}

void
QueueManager::insertItems()
{
    QPtrList<PlaylistItem> list = Playlist::instance()->m_nextTracks;
    QListViewItem *last = 0;

    for( PlaylistItem *item = list.first(); item; item = list.next() )
    {
        QString title = item->title();
        title.append( i18n(" - " ) );
        title.append( item->artist() );

        last = new QListViewItem( m_listview, last, title );

        m_map[ last ] = item;
    }
}

#endif
