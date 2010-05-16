/****************************************************************************************
 * Copyright (c) 2010 Casey Link <unnamedrambler@gmail.com>                             *
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

#include "MimeTypeFilterProxyModel.h"

#include <KDirModel>
#include <KFileItem>
#include <KFile>

MimeTypeFilterProxyModel::MimeTypeFilterProxyModel( QStringList mimeList, QObject *parent )
    : QSortFilterProxyModel( parent )
    , m_mimeList( mimeList )
{
}

bool
MimeTypeFilterProxyModel::filterAcceptsRow( int source_row, const QModelIndex& source_parent ) const
{
    QModelIndex index = sourceModel()->index( source_row, 0, source_parent );

    QVariant qvar = index.data( KDirModel::FileItemRole );
    if( !qvar.canConvert<KFileItem>() )
        return false;

    KFileItem item = qvar.value<KFileItem>();

    if( item.name() == "." )
        return false;
    
    if( item.isDir() || m_mimeList.contains( item.mimetype() ) )
        return QSortFilterProxyModel::filterAcceptsRow( source_row, source_parent );
    
    return false;
}

bool
MimeTypeFilterProxyModel::lessThan( const QModelIndex& left, const QModelIndex& right ) const
{
    const QVariant qvarLeft   = left.data( KDirModel::FileItemRole );
    const QVariant qvarRight  = right.data( KDirModel::FileItemRole );
    const KFileItem itemLeft  = qvarLeft.value<KFileItem>();
    const KFileItem itemRight = qvarRight.value<KFileItem>();

    switch( left.column() )
    {
    case KDirModel::Name:
        // if both items are directories or files, then compare (case insensitive) on name
        if( itemLeft.isDir() == itemRight.isDir() )
            return itemLeft.name( true ) < itemRight.name( true );
        // directories come before files
        if( itemLeft.isDir() )
            return true;
        break;

    case KDirModel::Size:
        return itemLeft.size() < itemRight.size();
        break;

    case KDirModel::ModifiedTime:
        return itemLeft.time( KFileItem::ModificationTime ) < itemRight.time( KFileItem::ModificationTime );
        break;

    case KDirModel::Permissions:
    case KDirModel::Owner:
    case KDirModel::Group:
    case KDirModel::Type:
    default:
        return QString::localeAwareCompare( itemLeft.text(), itemRight.text() ) < 0;
        break;
    }
    return false;
}


