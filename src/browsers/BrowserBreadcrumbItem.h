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
 
#ifndef BROWSERBREADCRUMBITEM_H
#define BROWSERBREADCRUMBITEM_H

#include "widgets/BoxWidget.h"

#include <QIcon>

class FileBrowser;
class BrowserCategory;
class BreadcrumbItemButton;
class BreadcrumbItemMenuButton;

struct BreadcrumbSibling {
    BreadcrumbSibling( const QIcon &icon_, const QString &name_, const QString &callback_ )
        : icon( icon_ ), name( name_ ), callback( callback_ ) {}

    QIcon icon;
    QString name;
    QString callback;
};
typedef QList<BreadcrumbSibling> BreadcrumbSiblingList;

/**
 *  A widget representing a single "breadcrumb" item
 *  It has a mainButton and optionally a menuButton
 *
 *  @author Nikolaj Hald Nielsen <nhn@kde.org>
 */

class BrowserBreadcrumbItem : public BoxWidget
{
    Q_OBJECT
public:
    BrowserBreadcrumbItem( BrowserCategory* category, QWidget* parent = 0 );

    /**
     * Overloaded constructor for creating breadcrumb items not bound to a particular BrowserCategory
     */
    BrowserBreadcrumbItem( const QString &name, const QString &callback,
                           const BreadcrumbSiblingList &childItems, FileBrowser *handler,
                           QWidget *parent = 0 );
    ~BrowserBreadcrumbItem();

    void setActive( bool active );

    QSizePolicy sizePolicy () const;

    int nominalWidth() const;

Q_SIGNALS:

    void activated( const QString &callback );

protected Q_SLOTS:
    void updateSizePolicy();
    void activate();
    void activateSibling();

private:
    BreadcrumbItemMenuButton *m_menuButton;
    BreadcrumbItemButton     *m_mainButton;

    QString m_callback;
    int m_nominalWidth;
};

#endif
