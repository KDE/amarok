/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2011 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#ifndef AMAROKBROWSERCATEGORYLIST_H
#define AMAROKBROWSERCATEGORYLIST_H

#include "BrowserCategory.h"
#include "BrowserCategoryListModel.h"
#include "BrowserCategoryListSortFilterProxyModel.h"

#include <QMap>

namespace Amarok {
    class PrettyTreeView;
}
class SearchWidget;

class QStackedWidget;

/**
 * This is a browser category that can contain other sub-categories
 * The main (home/root) category is such a BrowserCategoryList
 *
 * @author Nikolaj Hald Nielsen <nhn@kde.org>
 */
class BrowserCategoryList : public BrowserCategory
{
    Q_OBJECT

    public:
       /**
        * Constructor
        * @param parent The parent widget.
        * @param name The name of this widget.
        * @param sort Whether to sort the list.
        */
        explicit BrowserCategoryList( const QString& name, QWidget* parent = nullptr, bool sort = false );

        /**
         * Destructor.
         */
        ~BrowserCategoryList() override;

        /**
         * Get a map of the categories.
         * @return the map of categories.
         */
        QMap<QString,BrowserCategory *> categories();

        BrowserCategory *activeCategory() const;

        /**
         * Show a category. Hide any other active category if needed.
         */
        void setActiveCategory( BrowserCategory *category );

        /**
         * Recursively navigate to a specific category.
         * @param target This is a / delimited string of category names.
         * This list will take the first category name, and if a child category with
         * this name exists, it will switch to it. If there are are more category names
         * in the target string, and the category activated is itself a category list,
         * it will strip the first category name and / from the targe string and pass
         * the rest to the navigate() method of the active category list.
         *
         * @return this method will navigate as far as the target makes sense. Any parts
         * of the target that does not match up with child categories will be returned
         * as this might be additional arguments that are usable elsewhere.
         */
        QString navigate( const QString &target );


        QString path();

        BrowserCategory *activeCategoryRecursive();

    Q_SIGNALS:
        void viewChanged();

    public Q_SLOTS:

        /**
         * Add a category.
         * This category will take ownership of the new sub-category.
         * @param category The category to add.
         */
        void addCategory( BrowserCategory *category );

        /**
         * Remove a named category from the list and delete it.
         * @param category The category to remove.
         */
        void removeCategory( BrowserCategory *category );


        /**
         * Slot called when the active category should be hidden the category selection list shown again.
         */
        void home();

        /**
         * Slot called when the we need to move up one level. Forwarded to child lists as needed
         */
        void back();

        void childViewChanged();

    private Q_SLOTS:
        /** Sets the current filter value and updates the content */
        void setFilter( const QString &filter ) override;

    private:

        SearchWidget *m_searchWidget;
        QStackedWidget *m_widgetStack;
        Amarok::PrettyTreeView *m_categoryListView;

        QMap<QString, BrowserCategory *> m_categories;

        BrowserCategoryListModel *m_categoryListModel;
        BrowserCategoryListSortFilterProxyModel* m_proxyModel;

        QString m_infoHtmlTemplate;

        bool m_sorting;

    private Q_SLOTS:
        /**
         * Slot called when an item in the list has been activated and the
         * corresponding category should be shown.
         * @param index The index that was activated
         */
        void categoryActivated( const QModelIndex &index );

        void categoryEntered( const QModelIndex &index );

        QString css();
};


#endif
