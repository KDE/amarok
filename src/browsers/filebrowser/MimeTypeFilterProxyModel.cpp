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
#include <kfile.h>

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
    // get the KFileItems
    QVariant qvarLeft = left.data( KDirModel::FileItemRole );
    QVariant qvarRight = right.data( KDirModel::FileItemRole );

    KFileItem itemLeft = qvarLeft.value<KFileItem>();
    KFileItem itemRight = qvarRight.value<KFileItem>();

    // if both items are directories or files, then compare (case insensitive) on name
    if( itemLeft.isDir() == itemRight.isDir()) {
        return itemLeft.name(true) < itemRight.name(true);
    }

    // directories come before files
    if (itemLeft.isDir()) {
        return true;
    } else {
        return false;
    }
}


