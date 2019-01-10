/****************************************************************************************
 * Copyright (c) 2011 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include "PlaylistBrowserFilterProxy.h"

#include "core/support/Debug.h"

PlaylistBrowserFilterProxy::PlaylistBrowserFilterProxy( QObject *parent ) :
    QSortFilterProxyModel( parent )
{
}

void
PlaylistBrowserFilterProxy::setSourceModel( QAbstractItemModel *model )
{
    if( sourceModel() )
        sourceModel()->disconnect();

    QSortFilterProxyModel::setSourceModel( model );
    connect( sourceModel(), SIGNAL(renameIndex(QModelIndex)),
             SLOT(slotRenameIndex(QModelIndex)) );
}

void
PlaylistBrowserFilterProxy::slotRenameIndex( const QModelIndex &sourceIdx )
{
    const QModelIndex &idx = mapFromSource( sourceIdx );
    if( idx.isValid() )
        Q_EMIT renameIndex( idx );
}
