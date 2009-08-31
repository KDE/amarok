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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "BrowserCategory.h"
#include "BrowserCategoryListModel.h"
#include "Debug.h"

BrowserCategoryListModel::BrowserCategoryListModel()
 : QAbstractListModel()
{
}

BrowserCategoryListModel::~BrowserCategoryListModel()
{
}

int
BrowserCategoryListModel::rowCount( const QModelIndex & parent ) const
{
    Q_UNUSED( parent );
    return m_categories.count();
}

QVariant
BrowserCategoryListModel::data( const QModelIndex &index, int role ) const
{
    //DEBUG_BLOCK
    if( !index.isValid() || m_categories.count() <= index.row() )
        return QVariant();

    switch( role )
    {
        case Qt::DisplayRole:
            return QVariant( m_categories[index.row()]->prettyName() );

        case Qt::DecorationRole:
            return QVariant( m_categories[index.row()]->icon() );

        case CustomCategoryRoles::ShortDescriptionRole:
        case CustomCategoryRoles::SortRole:
            return QVariant( m_categories[index.row()]->shortDescription() );

        case CustomCategoryRoles::LongDescriptionRole:
            return QVariant( m_categories[index.row()]->longDescription() );

        case CustomCategoryRoles::CategoryRole:
            return qVariantFromValue( m_categories[index.row()] );

        case CustomCategoryRoles::AlternateRowRole:
            return ( index.row() % 2 == 1 );

        default:
            return QVariant();
     }
}

void
BrowserCategoryListModel::addCategory( BrowserCategory *category )
{
    if( !category )
    {
        debug() << "Trying to add a nonexistent service to the BrowserCategoryListModel!";
        return;
    }
    beginInsertRows ( QModelIndex(), m_categories.count(), m_categories.count() );
    m_categories.push_back( category );
    endInsertRows();
}

void
BrowserCategoryListModel::removeCategory( BrowserCategory *category )
{
    if( !category )
    {
        debug() << "Trying to remove a nonexistent service from the BrowserCategoryListModel!";
        return;
    }
    int index = m_categories.indexOf( category );
    beginRemoveRows ( QModelIndex(), index, index );
    m_categories.removeAt( index );
    endRemoveRows();
}

