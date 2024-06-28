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

#include "DirPlaylistTrackFilterProxyModel.h"

#include "core/playlists/PlaylistFormat.h"
#include "core-impl/meta/file/File.h"

#include <KDirModel>
#include <KFileItem>

DirPlaylistTrackFilterProxyModel::DirPlaylistTrackFilterProxyModel( QObject *parent )
    : KDirSortFilterProxyModel( parent )
{
}

bool
DirPlaylistTrackFilterProxyModel::filterAcceptsRow( int source_row,
                                                    const QModelIndex& source_parent ) const
{
    QModelIndex index = sourceModel()->index( source_row, 0, source_parent );

    QVariant qvar = index.data( KDirModel::FileItemRole );
    if( !qvar.canConvert<KFileItem>() )
        return false;

    KFileItem item = qvar.value<KFileItem>();

    if( item.name() == QStringLiteral(".") )
        return false;

    if( item.isDir() ||
        Playlists::isPlaylist( item.url() ) ||
        MetaFile::Track::isTrack( item.url() ) )
    {
        return QSortFilterProxyModel::filterAcceptsRow( source_row, source_parent );
    }

    return false;
}
