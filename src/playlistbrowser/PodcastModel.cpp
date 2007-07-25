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
}


PodcastModel::~PodcastModel()
{
}

QVariant
PlaylistBrowserNS::PodcastModel::data(const QModelIndex & index, int role) const
{
    return QVariant();
}

QModelIndex
PlaylistBrowserNS::PodcastModel::index(int row, int column, const QModelIndex & parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    PodcastChannelPtr channel;
    PodcastEpisodePtr episode;

    if (!parent.isValid())
        channel = The::podcastCollection()->channels()[row];
    else
        episode = channel->episodes()[row];

    if( episode )
        return createIndex( row, column, episode );
    else if( channel )
        return createIndex( row, column, channel );
    else
        return QModelIndex();
}

QVariant
PlaylistBrowserNS::PodcastModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    DEBUG_BLOCK
    debug() << "section = " << section << endl;
    switch( section )
    {
        case 0: return QString("Type");
        case 1: return QString("Title");
        case 2: return QString("Summary");
        default: return QString( "Section ") + QString::number( section );
    }
}

QModelIndex
PlaylistBrowserNS::PodcastModel::parent(const QModelIndex & index) const
{
    if (!index.isValid())
        return QModelIndex();

    PodcastMetaCommon *podcastMetaCommon = static_cast<PodcastMetaCommon*>(index.internalPointer());

    if ( podcastMetaCommon && podcastMetaCommon->podcastType() == PodcastMetaCommon::ChannelType )
        return QModelIndex();
    else if ( podcastMetaCommon->podcastType() == PodcastMetaCommon::EpisodeType )
    {
        PodcastEpisode * episode = static_cast<PodcastEpisode *>( podcastMetaCommon );
        return createIndex(episode->channel(), 0, podcastMetaCommon);
    }
    else
        return QModelIndex();
}

int
PlaylistBrowserNS::PodcastModel::rowCount(const QModelIndex & parent) const
{
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        return The::podcastCollection()->channels().count();
    else
    {
        PodcastChannel *channel = static_cast<PodcastChannel*>(parent.internalPointer());
        return channel->episodes().count();
    }

}

int
PlaylistBrowserNS::PodcastModel::columnCount(const QModelIndex & parent) const
{
    return 0;
}

Qt::ItemFlags
PlaylistBrowserNS::PodcastModel::flags(const QModelIndex & index) const
{
    if (!index.isValid())
        return 0;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

}

