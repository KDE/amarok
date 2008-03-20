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
#include <QSvgRenderer>
#include <QToolButton>
#include <QTreeView>

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
    public:
        explicit PodcastView( PodcastModel *model, QWidget *parent = 0 );
        ~PodcastView();

        void contextMenuEvent( QContextMenuEvent* event );

    private:
        PodcastModel *m_model;
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
};

}

#endif
