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

#include "Debug.h"
#include "DynamicBiasWidgets.h"
#include "BiasedPlaylist.h"

#include <QVariant>

#include <typeinfo>

PlaylistBrowserNS::DynamicBiasModel::DynamicBiasModel( QListView* listView )
    : m_listView( listView )
{
}

PlaylistBrowserNS::DynamicBiasModel::~DynamicBiasModel()
{
    clearWidgets();
}

void
PlaylistBrowserNS::DynamicBiasModel::setPlaylist( Dynamic::DynamicPlaylistPtr playlist )
{
    DEBUG_BLOCK

    if( playlist.data() == m_playlist.data() )
        return;

    clearWidgets();

    Dynamic::BiasedPlaylist* bp = 
        dynamic_cast<Dynamic::BiasedPlaylist*>( playlist.data() );

    if( bp )
    {
        beginInsertRows( QModelIndex(), 0, bp->biases().size() );
        m_playlist = bp;
        foreach( Dynamic::Bias* b, bp->biases() )
        {
            debug() << "BIAS ADDED";
            PlaylistBrowserNS::BiasWidget* widget = b->widget( m_listView->viewport() );

            connect( widget, SIGNAL(widgetChanged(QWidget*)),
                    SLOT(widgetChanged(QWidget*)) );
            connect( widget, SIGNAL(biasRemoved(Dynamic::Bias*)),
                    SLOT(removeBias(Dynamic::Bias*)) );
            connect( widget, SIGNAL(biasChanged(Dynamic::Bias*)),
                    SLOT(biasChanged(Dynamic::Bias*)) );

            m_widgets.append( widget );
        }

        PlaylistBrowserNS::BiasAddWidget* adder =
            new PlaylistBrowserNS::BiasAddWidget( m_listView->viewport() );
        connect( adder, SIGNAL(addBias(Dynamic::Bias*)),
                SLOT(appendBias(Dynamic::Bias*)) );

        m_widgets.append( adder );
        connect( m_widgets.back(), SIGNAL(widgetChanged(QWidget*)),
                this, SLOT(widgetChanged(QWidget*)) );
        endInsertRows();
    }
    // TODO: else add the "Unknown Playlist Type" box.
}


void
PlaylistBrowserNS::DynamicBiasModel::appendBias( Dynamic::Bias* b )
{
    if( !m_playlist )
        return;

    beginInsertRows( QModelIndex(), rowCount()-1, rowCount()-1 );
    m_playlist->biases().append( b );

    PlaylistBrowserNS::BiasWidget* widget = b->widget( m_listView->viewport() );

    connect( widget, SIGNAL(widgetChanged(QWidget*)),
            SLOT(widgetChanged(QWidget*)) );
    connect( widget, SIGNAL(biasRemoved(Dynamic::Bias*)),
            SLOT(removeBias(Dynamic::Bias*)) );
    connect( widget, SIGNAL(biasChanged(Dynamic::Bias*)),
            SLOT(biasChanged(Dynamic::Bias*)) );

    m_widgets.insert( rowCount()-1, widget );
    endInsertRows();

    emit playlistModified( m_playlist );
}

void
PlaylistBrowserNS::DynamicBiasModel::removeBias( Dynamic::Bias* b )
{
    int index = m_playlist->biases().indexOf( b );
    if( index == -1 )
        return;

    beginRemoveRows( QModelIndex(), index, index );
    delete m_widgets[index];
    m_widgets.removeAt( index );
    m_playlist->biases().removeAt( index );
    endRemoveRows();

    emit playlistModified( m_playlist );
}

void
PlaylistBrowserNS::DynamicBiasModel::biasChanged( Dynamic::Bias* b )
{
    DEBUG_BLOCK
    Q_UNUSED(b);
    emit playlistModified( m_playlist );
}


void
PlaylistBrowserNS::DynamicBiasModel::clearWidgets()
{
    if( rowCount() <= 0 ) return;

    beginRemoveRows( QModelIndex(), 0, rowCount() - 1 );

    foreach( PlaylistBrowserNS::BiasBoxWidget* b, m_widgets )
    {
        delete b;
    }
    m_widgets.clear();

    endRemoveRows();
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

    //return createIndex( row, column, 
            //reinterpret_cast<void*>(m_widgets.at( row )) );
    return createIndex( row, column, 0 );
}

QModelIndex
PlaylistBrowserNS::DynamicBiasModel::indexOf( PlaylistBrowserNS::BiasBoxWidget* widget )
{
    return index( m_widgets.indexOf( widget ), 0 );
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

void
PlaylistBrowserNS::DynamicBiasModel::widgetChanged( QWidget* w )
{
    DEBUG_BLOCK
    Q_UNUSED(w)
    // more or less a hack to get the delegate to redraw the list correctly when
    // the size of one of the widgets changes.

    //int i;
    //if( w )
    //{
        //for( i = 0; i < m_widgets.size(); ++i )
        //{
            //if( m_widgets[i] == w )
                //break;
        //}
    //}
    //else i = 0;

    beginInsertRows( QModelIndex(), 0, 0 );
    endInsertRows();
}

