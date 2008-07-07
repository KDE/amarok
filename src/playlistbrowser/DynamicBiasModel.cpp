/***************************************************************************
 * copyright         : (C) 2008 Daniel Caleb Jones <danielcjones@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

#include "DynamicBiasModel.h"
#include "DynamicBiasWidgets.h"
#include "BiasedPlaylist.h"

#include <QVariant>

#include <typeinfo>

PlaylistBrowserNS::DynamicBiasModel::DynamicBiasModel( QListView* listView,
        Dynamic::DynamicPlaylistPtr playlist )
    : m_listView( listView )
{
    Dynamic::BiasedPlaylist* bp = 
        dynamic_cast<Dynamic::BiasedPlaylist*>( playlist.data() );

    if( bp )
    {
        m_playlist = bp;
        foreach( Dynamic::Bias* b, bp->biases() )
        {
            m_widgets.append( b->widget( m_listView ) );
        }

        m_widgets.append( new PlaylistBrowserNS::BiasAddWidget( m_listView ) );
    }
    // TODO: else add the "Unknown Playlist Type" box.
}

PlaylistBrowserNS::DynamicBiasModel::~DynamicBiasModel()
{
    foreach( PlaylistBrowserNS::BiasBoxWidget* b, m_widgets )
    {
        delete b;
    }
}


QVariant
PlaylistBrowserNS::DynamicBiasModel::data( const QModelIndex& index, int role ) const
{
    Q_UNUSED(role)

    if( !m_playlist )
        return QVariant();

    if( role == BiasRole )
    {
        if( index.row() >= m_playlist->biases().size()  )
            return QVariant();
        else
            return QVariant::fromValue( m_playlist->biases().at( index.row() ) );
    }
    else if( role == WidgetRole )
        return QVariant::fromValue( m_widgets.at( index.row() ) );
    else
        return QVariant();
}


QModelIndex
PlaylistBrowserNS::DynamicBiasModel::index( int row, int column,
        const QModelIndex& parent ) const
{
    Q_UNUSED(parent)
    if( rowCount() <= row ) return QModelIndex();

    return createIndex( row, column, 
            reinterpret_cast<void*>(m_widgets.at( row )) );
}

QModelIndex
PlaylistBrowserNS::DynamicBiasModel::parent( const QModelIndex& index ) const
{
    Q_UNUSED(index)
    return QModelIndex();
}

int
PlaylistBrowserNS::DynamicBiasModel::rowCount( const QModelIndex& parent ) const
{
    Q_UNUSED(parent)
    return m_playlist ? m_widgets.size() : 0;
}

int
PlaylistBrowserNS::DynamicBiasModel::columnCount( const QModelIndex& parent ) const 
{
    Q_UNUSED(parent)
    return 1;
}

