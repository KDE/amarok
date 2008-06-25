/*
    Copyright (c) 2008 Daniel Jones <danielcjones@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "DynamicModel.h"
#include "DynamicPlaylist.h"
#include "RandomPlaylist.h"

#include <QVariant>


PlaylistBrowserNS::DynamicModel* PlaylistBrowserNS::DynamicModel::s_instance = 0;

PlaylistBrowserNS::DynamicModel*
PlaylistBrowserNS::DynamicModel::instance()
{
    if( s_instance == 0 ) s_instance = new DynamicModel();

    return s_instance;
}


PlaylistBrowserNS::DynamicModel::DynamicModel()
    : QAbstractItemModel()
{
    // TODO: Here we will load biased playlists.
    // For now we just have our dummy random mode
    // so it at least does something
    m_defaultPlaylist = new Meta::RandomPlaylist;
    m_playlistHash[ m_defaultPlaylist->title() ] = m_defaultPlaylist;
    m_playlistList.append( m_defaultPlaylist );

}

Meta::DynamicPlaylistPtr
PlaylistBrowserNS::DynamicModel::retrievePlaylist( QString title )
{
    return m_playlistHash[ title ];
}

Meta::DynamicPlaylistPtr
PlaylistBrowserNS::DynamicModel::retrieveDefaultPlaylist()
{
    return m_defaultPlaylist;
}


PlaylistBrowserNS::DynamicModel::~DynamicModel()
{
}

QModelIndex 
PlaylistBrowserNS::DynamicModel::index( int row, int column, const QModelIndex& parent ) const
{
    Q_UNUSED(parent)
    if( rowCount() <= row ) return QModelIndex();

    return createIndex( row, column, (void*)m_playlistList[row].data() );
}

QVariant 
PlaylistBrowserNS::DynamicModel::data ( const QModelIndex & i, int role ) const
{
    if( !i.isValid() ) return QVariant();

    Meta::DynamicPlaylistPtr item = m_playlistList[i.column()];


    switch( role )
    {
        case Qt::UserRole:
            return QVariant::fromValue( item );
        case Qt::DisplayRole:
        case Qt::EditRole:
            return item->title();
        default:
            return QVariant();
    }
}

QModelIndex
PlaylistBrowserNS::DynamicModel::parent( const QModelIndex& i ) const
{
    Q_UNUSED(i)
    return QModelIndex();
}

int
PlaylistBrowserNS::DynamicModel::rowCount( const QModelIndex& ) const
{
    return m_playlistList.size();
}


int
PlaylistBrowserNS::DynamicModel::columnCount( const QModelIndex& ) const
{
    return 1;
}


