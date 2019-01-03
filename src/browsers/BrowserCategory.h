/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
 
#ifndef BROWSERCATEGORY_H
#define BROWSERCATEGORY_H

#include "amarok_export.h"
#include "BrowserDefines.h"
#include "widgets/BoxWidget.h"

#include <QIcon>

class BrowserBreadcrumbItem;
class BrowserCategoryList;

/**
 * The base class of browsers, services, categories or any other widget that can be inserted into a CategoryList
 *
 * @author Nikolaj Hald Nielsen <nhn@kde.org>
*/
class AMAROK_EXPORT BrowserCategory : public BoxWidget
{
    Q_OBJECT

public:

    /**
     * Constructor.
     *
     * @param name The internal name of the category, used for generating Amarok urls. This should never be translated.
     * @param parent The parent widget.
     */
    explicit BrowserCategory( const QString &name, QWidget *parent = nullptr );

    /**
     * Destructor.
     */
    ~BrowserCategory();

    /**
     * Get the internal name of this category.
     *
     * @return The name.
     */
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
    virtual QString prettyName() const;

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
     * Set the icon that will be used to identify this category.
     * @param icon The icon to use.
     */
    void setIcon( const QIcon &icon );

    /**
     * Get the icon of this category.
     * @return The icon
     */
    QIcon icon() const;

    /**
     * Set the background image of this browser widget
     *
     * \param path Fully qualified path (e.g. looked up with KStandardDirs::locate)
     */
    void setBackgroundImage( const QString &path );

    /**
     * Set the path of the imaged used in the presentation of this category.
     * @param path The path of the image to use.
     */
    void setImagePath( const QString &path );

    /**
     * Get the path of the image used in the presentation of this category.
     * @return The path of the image.
     */
    QString imagePath() const;

    BrowserCategoryList * parentList() const;
    void setParentList( BrowserCategoryList * parent );

    /**
     * Returns an item that will be added to the breadcrumb widget
     * if this category is selected.
     *
     * The caller will have to free the returned item.
     */
    BrowserBreadcrumbItem* breadcrumb();

    virtual void polish() {}
    virtual void setupAddItems() {}

    //These 2 functions are forwarded to simplify the creation of urls
    //even though they might not be needed in many cases.
    virtual QString filter() const { return QString(); }
    virtual QList<CategoryId::CatMenuId> levels() const { return QList<CategoryId::CatMenuId>(); }

    virtual void setFilter( const QString &filter ) { Q_UNUSED( filter ) };
    virtual void setLevels( const QList<CategoryId::CatMenuId> &levels ) { Q_UNUSED( levels ) };

    /**
     * Add an additional category-specific item to breadcrumbs. This BrowserCategory
     * takes ownership of the item.
     */
    void addAdditionalItem( BrowserBreadcrumbItem *item );
    void clearAdditionalItems();

    QList<BrowserBreadcrumbItem *> additionalItems();

public Q_SLOTS:
    void activate();

    //Called if this category itself is re-clicked in the breadcrumb
    virtual void reActivate() {}

private Q_SLOTS:
    void slotSettingsChanged();

private:
    QString m_name;
    QString m_prettyName;
    QString m_shortDescription;
    QString m_longDescription;
    QIcon   m_icon;
    QString m_imagePath;
    BrowserCategoryList * m_parentList;

    QList<BrowserBreadcrumbItem *> m_additionalItems;

};

#endif
