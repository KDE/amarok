/****************************************************************************************
 * Copyright (c) 2007-2010 Bart Cerneels <bart.cerneels@kde.org>                        *
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

#include "PlaylistBrowserCategory.h"

#include <KDialog>

#include <QModelIndex>
#include <QPoint>
#include <QSortFilterProxyModel>
#include <QItemDelegate>
#include <QWebPage>

namespace PlaylistBrowserNS {

/**
    @author Bart Cerneels <bart.cerneels@kde.org>
*/
class PodcastCategory : public PlaylistBrowserCategory
{
    Q_OBJECT
    public:
        static PodcastCategory *instance();
        static void destroy();

    private:
        static PodcastCategory* s_instance;
        static QString s_configGroup;
        static QString s_mergedViewKey;

        PodcastCategory( QWidget *parent );
        ~PodcastCategory();

    private slots:
        void showInfo( const QModelIndex & index );
        void slotImportOpml();
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

        void paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const;
        QSize sizeHint( const QStyleOptionViewItem &option, const QModelIndex &index ) const;

    private:
        QTreeView *m_view;
        mutable int m_lastHeight;
        QWebPage *m_webPage;
};

}

namespace The {
    PlaylistBrowserNS::PodcastCategory *podcastCategory();
}

#endif
