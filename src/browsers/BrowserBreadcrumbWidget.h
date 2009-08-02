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
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef BREADCRUMBWIDGET_H
#define BREADCRUMBWIDGET_H

#include "BrowserBreadcrumbItem.h"
#include "BrowserCategoryList.h"

#include <KHBox>

#include <QList>
#include <QStringList>


/**
A widget for displaying th ecurrent state of, and navigating, the browser dig down interface.

	@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class BrowserBreadcrumbWidget : public KHBox
{
    Q_OBJECT
public:
    BrowserBreadcrumbWidget( QWidget * parent );

    ~BrowserBreadcrumbWidget();

    void setRootList( BrowserCategoryList * rootList );

signals:
    void toHome();
    
public slots:
    void updateBreadcrumbs();

private:
    void clearCrumbs();
    
    /**
     * Recursive function that traverses the tree of BrowserCategoryList's
     * and adds each one as a level in the breadcrumb.
     * @param level the root level BrowserCategoryList.
     */
    void addLevel( BrowserCategoryList * list );

    //QStringList m_currentPath;
    BrowserCategoryList * m_rootList;

    QList<BrowserBreadcrumbItem *> m_items;
    QWidget * m_spacer;

    KHBox * m_breadcrumbArea;

};

#endif
