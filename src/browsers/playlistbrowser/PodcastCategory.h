/****************************************************************************************
 * Copyright (c) 2007 Bart Cerneels <bart.cerneels@kde.org>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef PODCASTCATEGORY_H
#define PODCASTCATEGORY_H

#include "browsers/BrowserCategory.h"
#include "playlist/PlaylistModel.h"
#include "PodcastModel.h"
#include "widgets/PrettyTreeView.h"

#include <QContextMenuEvent>
#include <QItemDelegate>
#include <QMutex>
#include <QListView>
#include <QToolButton>
#include <QWebPage>

class PopupDropper;
class QAction;

class PlaylistsByProviderProxy;

namespace PlaylistBrowserNS {

class PodcastModel;
class PodcastView;
class PodcastCategoryDelegate;
class ViewKicker;

/**
    @author Bart Cerneels <bart.cerneels@kde.org>
*/
class PodcastCategory : public BrowserCategory
{
    Q_OBJECT
    public:
        static PodcastCategory *instance();
        static void destroy();

    private:
        static PodcastCategory* s_instance;
        static QString s_configGroup;
        static QString s_byProviderKey;

        PodcastCategory( PlaylistBrowserNS::PodcastModel *podcastModel );
        ~PodcastCategory();

        PodcastModel *m_podcastModel;
        PlaylistsByProviderProxy *m_byProviderProxy;
        PodcastView *m_podcastTreeView;
        ViewKicker * m_viewKicker;

    private slots:
        void showInfo( const QModelIndex & index );
        void slotImportOpml();
        void toggleView( bool );
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

class PodcastView : public Amarok::PrettyTreeView
{
    Q_OBJECT
    public:
        explicit PodcastView( PodcastModel *model, QWidget *parent = 0 );
        ~PodcastView();

    protected:
        void mouseReleaseEvent( QMouseEvent *event );
        void mouseDoubleClickEvent( QMouseEvent *event );
        void startDrag( Qt::DropActions supportedActions );

        void contextMenuEvent( QContextMenuEvent* event );

    private:
        PodcastModel *m_podcastModel;

        PopupDropper* m_pd;

        bool m_ongoingDrag;
        QMutex m_dragMutex;
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

namespace The {
    PlaylistBrowserNS::PodcastCategory *podcastCategory();
}

#endif
