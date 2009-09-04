/****************************************************************************************
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_COLLECTION_WIDGET
#define AMAROK_COLLECTION_WIDGET

#include "BrowserCategory.h"

#include <KVBox>

class QAction;
class QMenu;
class QStackedWidget;

class SearchWidget;
class CollectionBrowserTreeView;
class CollectionTreeItemModelBase;

class CollectionWidget : public BrowserCategory
{
    Q_OBJECT
    Q_ENUMS( ViewMode )
    public:
    enum ViewMode {
        UnifiedCollection,
        NormalCollections
    };

        CollectionWidget( const QString &name , QWidget *parent );
        static CollectionWidget *instance() { return s_instance; }
        CollectionBrowserTreeView *view() const { return m_treeView; }

        /**
         * Apply a filter to the tree view.
         * @param filter The filter to apply.
         */
        void setFilter( const QString &filter );
        virtual QString filter() const;
        virtual QList<int> levels() const;

    public slots:

        void customFilter( QAction * );
        void sortByAlbum();
        void sortByArtist();
        void sortByArtistAlbum();
        void sortByGenreArtist();
        void sortByGenreArtistAlbum();
        void slotShowYears( bool checked );
        void slotShowCovers( bool checked );

        void setLevels( const QList<int> &levels );

        void toggleView();

    private:
        SearchWidget        *m_searchWidget;
        QStackedWidget *m_stack;
        CollectionBrowserTreeView  *m_treeView;
        CollectionBrowserTreeView  *m_singleTreeView;
        CollectionTreeItemModelBase *m_singleModel;
        CollectionTreeItemModelBase *m_multiModel;
        ViewMode m_viewMode;

        QAction             *m_firstLevelSelectedAction;
        QAction             *m_secondLevelSelectedAction;
        QAction             *m_thirdLevelSelectedAction;

        QMenu               *m_firstLevel;
        QMenu               *m_secondLevel;
        QMenu               *m_thirdLevel;

        QList<int>          m_levels;

        static CollectionWidget *s_instance;
};

#endif
