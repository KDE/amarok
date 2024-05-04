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

#include "browsers/BrowserCategory.h"

class QAction;
class CollectionBrowserTreeView;
class SearchWidget;

class CollectionWidget : public BrowserCategory
{
    Q_OBJECT

    public:
        enum ViewMode
        {
            UnifiedCollection,
            NormalCollections
        };
        Q_ENUM( ViewMode )

        explicit CollectionWidget( const QString &name , QWidget *parent );
        ~CollectionWidget() override;
        static CollectionWidget *instance() { return s_instance; }


        QString filter() const override;

        /**
         * Apply a filter to the tree view.
         * @param filter The filter to apply.
         */
        void setFilter( const QString &filter ) override;

        /** Return the current views selected levels */
        QList<CategoryId::CatMenuId> levels() const override;

        /** Set the current views selected levels */
        void setLevels( const QList<CategoryId::CatMenuId> &levels ) override;

        void focusInputLine();
        CollectionBrowserTreeView *currentView();
        SearchWidget *searchWidget();
        ViewMode viewMode() const;

    public Q_SLOTS:
        void sortLevelSelected( QAction * );
        void sortByActionPayload( QAction * );
        void slotShowYears( bool checked );
        void slotShowTrackNumbers( bool checked );
        void slotShowArtistForVarious( bool checked );
        void slotShowCovers( bool checked );

        void toggleView( bool merged );

    private:
        QList<CategoryId::CatMenuId> readLevelsFromConfig() const;

        class Private;
        Private *const d;
        static CollectionWidget *s_instance;

        CollectionWidget( const CollectionWidget& );
        CollectionWidget& operator=( const CollectionWidget& );
};

#endif
