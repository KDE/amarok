/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#include <KVBox>

#include <QTimer>
#include <QTreeView>
#include <QMap>

class BrowserCategoryListDelegate;
class SearchWidget;

/**
 * A list for selecting and displaying a category. When a category is selected, this list
 * is replaces by the category widget. A back button allows the user to go back to this list.
 *
 * @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
 */
class BrowserCategoryList : public BrowserCategory
{
    Q_OBJECT

    public:
       /**
        * Constructor
        * @param parent The parent widget.
        * @param name The name of this widget.
        */
        BrowserCategoryList( QWidget *parent, const QString& name );

        /**
         * Destructor.
         */
        ~BrowserCategoryList();

        /**
         * Get a map of the categories.
         * @return the map of categories.
         */
        QMap<QString,BrowserCategory *> categories();

        /**
         * Remove a named category from the list.
         * @param name The name of the service to remove.
         */
        void removeCategory( const QString &name );
    
        /**
         * Make a category show. Hide any other active category if needed.
         * @param name the category to show.
         */
        void showCategory( const QString &name );

        QString activeCategoryName();
        BrowserCategory *activeCategory();

        /**
         * Recursively navigate to a specific category.
         * @param target This is a / delimited string of category names.
         * This list will take the first category name, and if a child category with
         * this name exists, it will switch to it. If there are are more category names
         * in the target string, and the category activcated is itself a category list,
         * it will strip the first category name and / from the targe string and pass
         * the rest to the navigate() method of the active category list.
         * 
         * @return this method will navigate as far as the target makes sense. Any parts
         * of the target that does not match up with child categories will be returned
         * as this might be additional arguments that are usable elsewhere.
         */
        QString navigate( const QString &target );

        void activate( BrowserCategory * category );

        QString path();

        virtual QString filter() const;
        virtual void setFilter( const QString &filter );

        BrowserCategory *activeCategoryRecursive();

    signals:
        void viewChanged();

    public slots:
        /**
         * Add a category.
         * @param category The category to add.
         */
        void addCategory( BrowserCategory *category );

        /**
         * Slot called when the active category should be hidden the category selection list shown again.
         */
        void home();

        /**
         * Slot called when the we need to move up one level. Forwarded to child lists as needed
         */
        void back();

        void childViewChanged();

    private:
        SearchWidget *m_searchWidget;
        QTreeView    *m_categoryListView;

        QMap<QString, BrowserCategory *> m_categories;
        BrowserCategory *m_currentCategory;

        BrowserCategoryListModel *m_categoryListModel;
        BrowserCategoryListSortFilterProxyModel* m_proxyModel;
        BrowserCategoryListDelegate *m_delegate;

        QTimer m_filterTimer;

        QString m_currentFilter;
        
        QString m_infoHtmlTemplate;

    private slots:
        /**
         * Slot called when an item in the list has been activated and the
         * corrosponding category should be shown.
         * @param index The index that was activated
         */
        void categoryActivated( const QModelIndex &index );

        void categoryEntered ( const QModelIndex & index );

        void slotSetFilterTimeout();
        void slotFilterNow();

        QString css();

};


#endif
