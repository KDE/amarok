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
#include "playlistmanager/PlaylistManager.h"
#include "TheInstances.h"
#include "PodcastCollection.h"
#include "PodcastMeta.h"

#include <QListIterator>

using namespace Meta;

namespace PlaylistBrowserNS {

PodcastModel::PodcastModel()
 : QAbstractItemModel()
{
    QList<PlaylistPtr> playlists =
            The::playlistManager()->playlistsOfCategory( PlaylistManager::PodcastChannel );
    QListIterator<PlaylistPtr> i(playlists);
    while (i.hasNext())
        m_channels << PodcastChannelPtr::staticCast( i.next() );

    connect( The::playlistManager(), SIGNAL(updated()), SLOT(slotUpdate()));
}


PodcastModel::~PodcastModel()
{
}

QVariant
PodcastModel::data(const QModelIndex & index, int role) const
{
    if ( !index.isValid() )
        return QVariant();

    if ( role == Qt::DisplayRole )
    {
        PodcastMetaCommon* pmc = static_cast<PodcastMetaCommon *>( index.internalPointer() );

        bool isChannel = false;
        QString title;
        QString description;
        if ( typeid( * pmc ) == typeid( PodcastChannel ) )
        {
            PodcastChannel *channel = static_cast<PodcastChannel *>(index.internalPointer());
            title = channel->title();
            description = channel->description();
            isChannel = true;
        }
        else if ( typeid( * pmc ) == typeid( PodcastEpisode ) )
        {
            PodcastEpisode *episode = static_cast<PodcastEpisode *>(index.internalPointer());
            title = episode->title();
            description = episode->description();
            isChannel = false;
        }

        switch( index.column() )
        {
            case 0: return isChannel ? QString("Channel") : QString("Episode"); break;
            case 1: return title; break;
            case 2: return description; break;
            default: return QVariant(); break;
        }
    }
    return QVariant();
}

QModelIndex
PodcastModel::index(int row, int column, const QModelIndex & parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    PodcastChannelPtr channel;
    PodcastEpisodePtr episode;

    if (!parent.isValid())
        channel = m_channels[row];
    else
    {
        channel = static_cast<PodcastChannel *>(parent.internalPointer());
        if( !channel.isNull() )
        {
            episode = channel->episodes()[row];
        }
        else
        {
            channel = 0;
        }
    }

    if( !episode.isNull() )
    {
        return createIndex( row, column, episode.data() );
    }
    else if( !channel.isNull() )
    {
        return createIndex( row, column, channel.data() );
    }
    else
        return QModelIndex();
}

QModelIndex
PodcastModel::parent(const QModelIndex & index) const
{
    if (!index.isValid())
        return QModelIndex();

    PodcastMetaCommon *podcastMetaCommon = static_cast<PodcastMetaCommon *>(index.internalPointer());

    if ( typeid( * podcastMetaCommon ) == typeid( PodcastChannel ) )
    {
        return QModelIndex();
    }
    else if ( typeid( * podcastMetaCommon ) == typeid( PodcastEpisode ) )
    {
        //BUG: using static_cast on podcastMetaCommon returns wrong address (exact offset of 12 bytes)
        PodcastEpisode *episode = static_cast<PodcastEpisode *>( index.internalPointer() );
        int row = m_channels.indexOf( episode->channel() );
        return createIndex( row , 0, episode->channel().data() );
    }
    else
    {
        return QModelIndex();
    }
}

int
PodcastModel::rowCount(const QModelIndex & parent) const
{
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
    {
        return m_channels.count();
    }
    else
    {
        PodcastMetaCommon *podcastMetaCommon = static_cast<PodcastMetaCommon *>(parent.internalPointer());

        if ( typeid( * podcastMetaCommon ) == typeid( PodcastChannel ) )
        {
            PodcastChannel *channel = static_cast<PodcastChannel*>(parent.internalPointer());
            return channel->episodes().count();
        }
        else if ( typeid( * podcastMetaCommon ) == typeid( PodcastEpisode ) )
        {
            return 0;
        }
        else
        {
            return 0;
        }
    }
}

int
PodcastModel::columnCount(const QModelIndex & parent) const
{
    return 3;
}

Qt::ItemFlags
PodcastModel::flags(const QModelIndex & index) const
{
    if (!index.isValid())
        return 0;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant
PodcastModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch( section )
        {
            case 0: return QString("Type");
            case 1: return QString("Title");
            case 2: return QString("Summary");
            default: return QVariant();
        }
    }

    return QVariant();
}

void
PodcastModel::slotUpdate()
{
    //TODO: emit dataChanged( QModelIndex(),  QModelIndex() );

    QList<PlaylistPtr> playlists =
    The::playlistManager()->playlistsOfCategory( PlaylistManager::PodcastChannel );
    QListIterator<PlaylistPtr> i(playlists);
    m_channels.clear();
    while (i.hasNext())
    {
        PodcastChannelPtr channel = PodcastChannelPtr::staticCast( i.next() );
        m_channels << channel;
    }

    emit layoutAboutToBeChanged();
    emit layoutChanged();
}

}

#include "PodcastModel.moc"
