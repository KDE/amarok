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
    debug() << k_funcinfo << " index: " << index.row() << ":" << index.column() << endl;
    return QVariant();
}

QModelIndex
PlaylistBrowserNS::PodcastModel::index(int row, int column, const QModelIndex & parent) const
{
    DEBUG_BLOCK
    debug() << k_funcinfo << " parent: " << parent.row() << ":" << parent.column() << endl;
    debug() << k_funcinfo << " row: " << row << " column: " << column << endl;
    if (!hasIndex(row, column, parent))
    {
        debug() << k_funcinfo << "!hasIndex(row, column, parent)" << endl;
        return QModelIndex();
    }

    PodcastChannelPtr channel;
    PodcastEpisodePtr episode;

    if (!parent.isValid())
    {
        debug() << k_funcinfo << "!parent.isValid()" << endl;
        channel = The::podcastCollection()->channels()[row];
    }
    else
    {
        debug() << k_funcinfo << "parent.isValid()" << endl;
        channel = PodcastChannelPtr( static_cast<PodcastChannel *>(parent.internalPointer()) );
        if( !channel.isNull() )
            episode = channel->episodes()[row];
    }

    if( !episode.isNull() )
    {
        debug() << k_funcinfo << "create index for Episode: " << episode->title() << endl;
        debug() << k_funcinfo << "data: " << episode.data() << endl;
        return createIndex( row, column, episode.data() );
    }
    else if( !channel.isNull() )
    {
        debug() << k_funcinfo << "create index for Channel: " << channel->title() << endl;
        debug() << k_funcinfo << "data: " << channel.data() << endl;
        return createIndex( row, column, channel.data() );
    }
    else
        return QModelIndex();
}

QModelIndex
PlaylistBrowserNS::PodcastModel::parent(const QModelIndex & index) const
{
    DEBUG_BLOCK
    debug() << k_funcinfo << " index: " << index.row() << ":" << index.column() << endl;
    if (!index.isValid())
    {
        debug() << k_funcinfo << "!index.isValid()" << endl;
        return QModelIndex();
    }

    PodcastMetaCommon *podcastMetaCommon = static_cast<PodcastMetaCommon*>(index.internalPointer());

   // debug() << k_funcinfo << "internal pointer = " << (void *)podcastMetaCommon << endl;
    if ( typeid( * podcastMetaCommon ) == typeid( PodcastChannel ) )
    {
        debug() << k_funcinfo << "podcastType() == ChannelType" << endl;
        return QModelIndex();
    }
    else if ( typeid( * podcastMetaCommon ) == typeid( PodcastEpisode ) )
    {
        PodcastEpisode *episode = static_cast<PodcastEpisode*>( podcastMetaCommon );
        debug() << k_funcinfo << "podcastType() == EpisodeType" << endl;
        //PodcastEpisode * episode = static_cast<PodcastEpisode *>( podcastMetaCommon );
        return createIndex(episode->channel(), 0, podcastMetaCommon);
    }
    else
    {
        debug() << k_funcinfo << "podcastType() == ?" << endl;
        return QModelIndex();
    }
}

int
PlaylistBrowserNS::PodcastModel::rowCount(const QModelIndex & parent) const
{
    debug() << k_funcinfo << " parent: " << parent.row() << ":" << parent.column() << endl;
    if (parent.column() > 0)
    {
        debug() << k_funcinfo << " parent.column() > 0" << endl;
        return 0;
    }

    if (!parent.isValid())
    {
        debug() << k_funcinfo << " !parent.isValid()" << endl;
        debug() << k_funcinfo << The::podcastCollection()->channels().count() << " channels" <<endl;
        return The::podcastCollection()->channels().count();
    }
    else
    {
        debug() << k_funcinfo << " parent.isValid() return episode count" << endl;
        PodcastChannel *channel = static_cast<PodcastChannel*>(parent.internalPointer());
        debug() << k_funcinfo << channel->episodes().count() << " episodes" << endl;
        return channel->episodes().count();
    }

}

int
PlaylistBrowserNS::PodcastModel::columnCount(const QModelIndex & parent) const
{
    debug() << k_funcinfo << " parent: " << parent.row() << ":" << parent.column() << endl;
    return 3;
}

Qt::ItemFlags
PlaylistBrowserNS::PodcastModel::flags(const QModelIndex & index) const
{
    if (!index.isValid())
        return 0;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

}

QVariant
PlaylistBrowserNS::PodcastModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    debug() << k_funcinfo << "section = " << section << endl;
    switch( section )
    {
        case 0: return QString("Type");
        case 1: return QString("Title");
        case 2: return QString("Summary");
        default: return QString( "Section ") + QString::number( section );
    }
}

void
PlaylistBrowserNS::PodcastModel::slotUpdate()
{
    DEBUG_BLOCK
    //emit dataChanged( QModelIndex(),  QModelIndex() );
    emit layoutChanged();
}

#include "PodcastModel.moc"
