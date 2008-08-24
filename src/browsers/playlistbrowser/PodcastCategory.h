/* This file is part of the KDE project
   Copyright (C) 2007 Bart Cerneels <bart.cerneels@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef PODCASTCATEGORY_H
#define PODCASTCATEGORY_H

#include "playlist/PlaylistModel.h"
#include "PodcastModel.h"

#include <QContextMenuEvent>
#include <QItemDelegate>
#include <QListView>
#include <QToolButton>
#include <QTreeView>
#include <QWebPage>

class PopupDropper;
class PopupDropperAction;

// class Meta::PodcastMetaCommon;

namespace PlaylistBrowserNS {

class PodcastModel;
class PodcastView;
class PodcastCategoryDelegate;
class ViewKicker;

/**
    @author Bart Cerneels <bart.cerneels@kde.org>
*/
class PodcastCategory : public QWidget
{
    Q_OBJECT
    public:
    PodcastCategory( PlaylistBrowserNS::PodcastModel *podcastModel );

    ~PodcastCategory();

    private:
        QToolButton *m_addPodcastButton;
        QToolButton *m_refreshPodcastsButton;
        QToolButton *m_configurePodcastsButton;
        QToolButton *m_podcastsIntervalButton;

        PodcastModel *m_podcastModel;
        PodcastView *m_podcastTreeView;
        ViewKicker * m_viewKicker;

    private slots:
        void showInfo( const QModelIndex & index );
};


class ViewKicker : public QObject
{
Q_OBJECT
    public:
        ViewKicker( QTreeView * treeView );

    private:
        QTreeView * m_treeView;

    public slots:
        void kickView();

};

class PodcastView : public QTreeView
{
    Q_OBJECT
    public:
        explicit PodcastView( PodcastModel *model, QWidget *parent = 0 );
        ~PodcastView();


    protected:
        void mousePressEvent( QMouseEvent *event );
        void mouseReleaseEvent( QMouseEvent *event );
        void mouseDoubleClickEvent( QMouseEvent *event );
        void startDrag( Qt::DropActions supportedActions );

        void contextMenuEvent( QContextMenuEvent* event );

    private:
        PodcastModel *m_model;

        QList<PopupDropperAction *> createCommonActions( QModelIndexList indices );

        PopupDropper* m_pd;

        PopupDropperAction * m_appendAction;
        PopupDropperAction * m_loadAction;

        PopupDropperAction * m_deleteAction;
        PopupDropperAction * m_renameAction;

        QSet<Meta::PodcastMetaCommon *> m_currentItems;
        
        QPoint m_dragStartPosition;

    private slots:

        void slotLoad();
        void slotAppend();
//         void slotDelete();
//         void slotRename();
};

/**
    A delegate for displaying the Podcast category

    @author Bart Cerneels
 */
class PodcastCategoryDelegate : public QItemDelegate
{
    public:
        PodcastCategoryDelegate( QTreeView *view );
        ~PodcastCategoryDelegate();

        void paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;
        QSize sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const;

    private:
        QTreeView *m_view;
        mutable int m_lastHeight;
        QWebPage * m_webPage;
};

}

#endif
