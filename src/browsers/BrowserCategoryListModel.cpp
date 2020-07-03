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

#include "BrowserCategoryListModel.h"
#include "BrowserCategory.h"
#include "core/support/Debug.h"

#include "widgets/PrettyTreeRoles.h"

BrowserCategoryListModel::BrowserCategoryListModel( QObject* parent )
 : QAbstractListModel( parent )
{
}

BrowserCategoryListModel::~BrowserCategoryListModel()
{
    qDeleteAll( m_categories );
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

        case PrettyTreeRoles::SortRole:
        case PrettyTreeRoles::ByLineRole:
            return QVariant( m_categories[index.row()]->shortDescription() );

        case Qt::ToolTipRole:
            return QVariant( m_categories[index.row()]->longDescription() );

        case CustomCategoryRoles::CategoryRole:
            return QVariant::fromValue( m_categories[index.row()] );

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

