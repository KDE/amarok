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
 
#ifndef BROWSERCATEGORY_H
#define BROWSERCATEGORY_H

#include "amarok_export.h"


#include <KVBox>

#include <QIcon>

class BreadcrumbItem;
class BrowserCategoryList;

/**
The base class of browsers, services, categories or any other widget that can be inserted into a CategoryList

	@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class AMAROK_EXPORT BrowserCategory : public KVBox
{
    Q_OBJECT
public:
    BrowserCategory( const QString &name );

    ~BrowserCategory();

    QString name() const;

    /**
     * Set the user visible name of this category
     * @param prettyName The user visible name.
     */
    void setPrettyName( const QString &prettyName );
    
    /**
     * Get the user visible name of this category.
     * @return The name of the service.
     */
    QString prettyName() const;

    /**
     * Set a short description string for this category. This string is used to describe the category in the category browser.
     * @param shortDescription The description.
     */
    void setShortDescription( const QString &shortDescription );

    /**
     * Get the short description of this category.
     * @return The short description.
     */
    QString shortDescription() const;

    /**
     * Set a long description of the category. This is for allowing users to get more detailed info a about a category.
     * @param longDescription The long description.
     */
    void setLongDescription( const QString &longDescription );

    /**
     * Get the long description of this category.
     * @return The long description.
     */
    QString longDescription() const;

    /**
     * Set the icon that will be used to identify this service.
     * @param icon The icon to use.
     */
    void setIcon( const QIcon &icon );

    /**
     * Get the icon of this service.
     * @return The icon
     */
    QIcon icon() const;

    BrowserCategoryList * parentList();
    void setParentList( BrowserCategoryList * parent );

    BreadcrumbItem * breadcrumb();

    virtual void polish() {};

public slots:
    void activate();

private:
    QString m_name;
    QString m_prettyName;
    QString m_shortDescription;
    QString m_longDescription;
    QIcon   m_icon;
    BrowserCategoryList * m_parentList;

    BreadcrumbItem * m_breadcrumb;
    

};

#endif
