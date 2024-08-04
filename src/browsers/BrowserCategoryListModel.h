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

#ifndef BROWSERCATEGORYLISTMODEL_H
#define BROWSERCATEGORYLISTMODEL_H

#include <QAbstractListModel>

#include <QList>

class BrowserCategory;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
Q_DECLARE_METATYPE( BrowserCategory * )
#endif

namespace CustomCategoryRoles
{
    enum CustomCategoryRolesId {
        CategoryRole = Qt::UserRole + 31,
    };
}

/**
A very simple model to hold the available categories

    @author Nikolaj Hald Nielsen <nhn@kde.org> 
*/
class BrowserCategoryListModel : public QAbstractListModel
{
public:
    explicit BrowserCategoryListModel( QObject *parent = nullptr );
    ~BrowserCategoryListModel() override;

    int rowCount( const QModelIndex & parent = QModelIndex() ) const override;
    QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const override;

    /**
     * Adds a new sub-category to this list.
     * This object will take ownership of the new category.
     */
    void addCategory( BrowserCategory* category );
    void removeCategory( BrowserCategory* category );

private:
    QList<BrowserCategory*> m_categories;
};

#endif
