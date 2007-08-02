/* This file is part of the KDE project
   Copyright (C) 2007 Bart Cerneels <bart.cerneels@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include "debug.h"
#include "PodcastModel.h"
#include "TheInstances.h"
#include "PodcastCollection.h"
#include "PodcastMeta.h"

using namespace Meta;

namespace PlaylistBrowserNS {

PodcastModel::PodcastModel()
 : QAbstractItemModel()
{
    DEBUG_BLOCK
    connect( The::podcastCollection(), SIGNAL(updated()), SLOT(slotUpdate()));
}


PodcastModel::~PodcastModel()
{
}

QVariant
PlaylistBrowserNS::PodcastModel::data(const QModelIndex & index, int role) const
{
    Q_UNUSED( role )
    debug() << k_funcinfo << " index: " << index.row() << ":" << index.column();
    switch( index.column() )
    {
        case 0: return QString("data 0"); break;
        case 1: return QString("data 1"); break;
        case 2: return QString("data 2"); break;
        case 3: return QString("data 3"); break;
        case 4: return QString("data 4"); break;
        default: return QString("data ?"); break;
    }
    return QVariant();
}

QModelIndex
PlaylistBrowserNS::PodcastModel::index(int row, int column, const QModelIndex & parent) const
{
    DEBUG_BLOCK
    debug() << k_funcinfo << " parent: " << parent.row() << ":" << parent.column();
    debug() << k_funcinfo << " row: " << row << " column: " << column;
    if (!hasIndex(row, column, parent))
    {
        debug() << k_funcinfo << "!hasIndex(row, column, parent)";
        return QModelIndex();
    }

    PodcastChannelPtr channel;
    PodcastEpisodePtr episode;

    if (!parent.isValid())
    {
        debug() << k_funcinfo << "!parent.isValid()";
        channel = The::podcastCollection()->channels()[row];
    }
    else
    {
        debug() << k_funcinfo << "parent.isValid()";
        channel = static_cast<PodcastChannel *>(parent.internalPointer());
        if( !channel.isNull() )
        {
            episode = channel->episodes()[row];
        }
        else
        {
            debug() << k_funcinfo << "but channel == null";
            channel = 0;
        }
    }

    if( !episode.isNull() )
    {
        debug() << k_funcinfo << "create index for Episode: " << episode->title();
        debug() << k_funcinfo << "data: " << episode.data();
        return createIndex( row, column, episode.data() );
    }
    else if( !channel.isNull() )
    {
        debug() << k_funcinfo << "create index for Channel: " << channel->title();
        debug() << k_funcinfo << "data: " << channel.data();
        return createIndex( row, column, channel.data() );
    }
    else
        return QModelIndex();
}

QModelIndex
PlaylistBrowserNS::PodcastModel::parent(const QModelIndex & index) const
{
    DEBUG_BLOCK
    debug() << k_funcinfo << " index: " << index.row() << ":" << index.column();
    if (!index.isValid())
    {
        debug() << k_funcinfo << "!index.isValid()";
        return QModelIndex();
    }

    PodcastMetaCommon *podcastMetaCommon = static_cast<PodcastMetaCommon*>(index.internalPointer());

    if ( typeid( * podcastMetaCommon ) == typeid( PodcastChannel ) )
    {
        debug() << k_funcinfo << "podcastType() == ChannelType";
        return QModelIndex();
    }
    else if ( typeid( * podcastMetaCommon ) == typeid( PodcastEpisode ) )
    {
        PodcastEpisode *episode = static_cast<PodcastEpisode*>( podcastMetaCommon );
        debug() << k_funcinfo << "podcastType() == EpisodeType";
        return createIndex( The::podcastCollection()->channels().indexOf( episode->channel() ),
                           0, episode->channel().data() );
    }
    else
    {
        debug() << k_funcinfo << "podcastType() == ?";
        return QModelIndex();
    }
}

int
PlaylistBrowserNS::PodcastModel::rowCount(const QModelIndex & parent) const
{
    debug() << k_funcinfo << " parent: " << parent.row() << ":" << parent.column();
    if (parent.column() > 0)
    {
        debug() << k_funcinfo << " parent.column() > 0";
        return 0;
    }

    if (!parent.isValid())
    {
        debug() << k_funcinfo << " !parent.isValid()";
        debug() << k_funcinfo << The::podcastCollection()->channels().count() << " channels";
        return The::podcastCollection()->channels().count();
    }
    else
    {
        debug() << k_funcinfo << " parent.isValid() return episode count";
        PodcastChannel *channel = static_cast<PodcastChannel*>(parent.internalPointer());
        debug() << k_funcinfo << channel->episodes().count() << " episodes";
        return channel->episodes().count();
    }

}

int
PlaylistBrowserNS::PodcastModel::columnCount(const QModelIndex & parent) const
{
    debug() << k_funcinfo << " parent: " << parent.row() << ":" << parent.column();
    return 3;
}

Qt::ItemFlags
PlaylistBrowserNS::PodcastModel::flags(const QModelIndex & index) const
{
    if (!index.isValid())
        return 0;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant
PodcastModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED( orientation )
            Q_UNUSED( role )

            debug() << k_funcinfo << "section = " << section;
    switch( section )
    {
        case 0: return QString("Type");
        case 1: return QString("Title");
        case 2: return QString("Summary");
        default: return QString( "Section ") + QString::number( section );
    }
}

void
PodcastModel::slotUpdate()
{
    DEBUG_BLOCK
    //emit dataChanged( QModelIndex(),  QModelIndex() );
    emit layoutChanged();
}

}

#include "PodcastModel.moc"
