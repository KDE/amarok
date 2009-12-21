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
Q_DECLARE_METATYPE( BrowserCategory * )

namespace CustomCategoryRoles
{
    enum CustomCategoryRolesId {
        ShortDescriptionRole = Qt::UserRole + 1,
        LongDescriptionRole = Qt::UserRole + 2,
        CategoryRole = Qt::UserRole + 3,
        AlternateRowRole = Qt::UserRole + 4,
        SortRole = Qt::UserRole + 5
    };
}

/**
A very simple model to hold the available categories

    @author Nikolaj Hald Nielsen <nhn@kde.org> 
*/
class BrowserCategoryListModel : public QAbstractListModel
{
public:
    BrowserCategoryListModel();
    ~BrowserCategoryListModel();

    int rowCount( const QModelIndex & parent = QModelIndex() ) const;
    QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;

    void addCategory( BrowserCategory * category );
    void removeCategory( BrowserCategory * category );

private:
    QList<BrowserCategory*> m_categories;
};

#endif
