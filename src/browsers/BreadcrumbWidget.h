/***************************************************************************
 *   Copyright (c) 2009  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "BrowserCategoryList.h"
 
#ifndef BREADCRUMBWIDGET_H
#define BREADCRUMBWIDGET_H

#include <KHBox>

#include <QPushButton>
#include <QList>
#include <QStringList>


class BreadcrumbItem : public KHBox
{
    Q_OBJECT
public:
    BreadcrumbItem( const QString &name, BrowserCategory * category, QWidget * parent );
    ~BreadcrumbItem();

    void setBold( bool bold );

private:
    BrowserCategory * m_category;
    QPushButton * m_menuButton;
    QPushButton * m_mainButton;
};


/**
A widget for displaying th ecurrent state of, and navigating, the browser dig down interface.

	@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class BreadcrumbWidget : public KHBox
{
    Q_OBJECT
public:
    BreadcrumbWidget( QWidget * parent );

    ~BreadcrumbWidget();

    void setRootList( BrowserCategoryList * rootList );

signals:
    void toHome();
    
public slots:
    void updateBreadcrumbs();

private:

    
    /**
     * Recursive function that traverses the tree of BrowserCategoryList's
     * and adds each one as a level in the breadcrumb.
     * @param level the root level BrowserCategoryList.
     */
    void addLevel( BrowserCategoryList * list );

    //QStringList m_currentPath;
    BrowserCategoryList * m_rootList;

    QList<BreadcrumbItem *> m_items;
    QWidget * m_spacer;

};

#endif
