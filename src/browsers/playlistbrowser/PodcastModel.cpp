/* This file is part of the KDE project
   Copyright (C) 2007 Bart Cerneels <bart.cerneels@kde.org>

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

#include "PodcastModel.h"

#include "Debug.h"
#include "playlistmanager/PlaylistManager.h"
#include "PodcastProvider.h"
#include "PodcastMeta.h"

#include <QInputDialog>
#include <KIcon>
#include <QListIterator>
#include <typeinfo>

PlaylistBrowserNS::PodcastModel::PodcastModel()
 : QAbstractItemModel()
{
    QList<Meta::PlaylistPtr> playlists =
            The::playlistManager()->playlistsOfCategory( PlaylistManager::PodcastChannel );
    QListIterator<Meta::PlaylistPtr> i(playlists);
    while (i.hasNext())
        m_channels << Meta::PodcastChannelPtr::staticCast( i.next() );

    connect( The::playlistManager(), SIGNAL(updated()), SLOT(slotUpdate()));
}


PlaylistBrowserNS::PodcastModel::~PodcastModel()
{
}

QVariant
PlaylistBrowserNS::PodcastModel::data(const QModelIndex & index, int role) const
{
            
    if ( !index.isValid() )
        return QVariant();

    if ( role == Qt::DisplayRole || role == Qt::DecorationRole || role == ShortDescriptionRole )
    {
        Meta::PodcastMetaCommon* pmc = static_cast<Meta::PodcastMetaCommon *>( index.internalPointer() );

        bool isChannel = false;
        QString title;
        QString description;
        KIcon icon;
        if ( pmc->podcastType() == Meta::ChannelType )
        {
            Meta::PodcastChannel *channel = static_cast<Meta::PodcastChannel *>(index.internalPointer());
            title = channel->title();
            description = channel->description();
            isChannel = true;
            icon = KIcon( "x-media-podcast-amarok" );
        }
        else if ( pmc->podcastType() == Meta::EpisodeType )
        {
            Meta::PodcastEpisode *episode = static_cast<Meta::PodcastEpisode *>( index.internalPointer() );
            title = episode->title();
            description = episode->description();
            isChannel = false;
            icon = KIcon( "podcast_new" );
        }
        else
        {
            debug() << "Model index was neither Channel nor Episode";
            return QVariant();
        }

        switch( role )
        {
            case Qt::DisplayRole: return title; break;
            case Qt::DecorationRole: return QVariant( icon ); break;
            case ShortDescriptionRole: return description ; break;
            default: return QVariant(); break;
        }
    }
    return QVariant();
}

QModelIndex
PlaylistBrowserNS::PodcastModel::index(int row, int column, const QModelIndex & parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    Meta::PodcastChannelPtr channel;
    Meta::PodcastEpisodePtr episode;

    if (!parent.isValid())
        channel = m_channels[row];
    else
    {
        channel = static_cast<Meta::PodcastChannel *>(parent.internalPointer());
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

    if ( !parent.isValid() )
    {
        Meta::PodcastChannelPtr channel = m_channels[row];
        debug() << "invalid parent! ";
        return createIndex( row, column, channel.data() );
    }
    else
    {
        Meta::PodcastMetaCommon *podcastMetaCommon = static_cast<Meta::PodcastMetaCommon *>(parent.internalPointer());

        if ( podcastMetaCommon->podcastType() ==  Meta::ChannelType )
        {
            Meta::PodcastChannel *channel = static_cast<Meta::PodcastChannel*>(parent.internalPointer());
            debug() << "child " << row << " of channel " << channel->title();
            return createIndex( row, column, channel->episodes()[row].data() );
        }
        else if ( podcastMetaCommon->podcastType() ==  Meta::EpisodeType )
        {
            return QModelIndex();
        }
        else
        {
            return QModelIndex();
        }
    }
}

QModelIndex
PlaylistBrowserNS::PodcastModel::parent(const QModelIndex & index) const
{
    if (!index.isValid())
        return QModelIndex();

    Meta::PodcastMetaCommon *podcastMetaCommon = static_cast<Meta::PodcastMetaCommon *>(index.internalPointer());

    if ( podcastMetaCommon->podcastType() ==  Meta::ChannelType )
    {
        return QModelIndex();
    }
    else if ( podcastMetaCommon->podcastType() ==  Meta::EpisodeType )
    {
        //BUG: using static_cast on podcastMetaCommon returns wrong address (exact offset of 12 bytes)
        Meta::PodcastEpisode *episode = static_cast<Meta::PodcastEpisode *>( index.internalPointer() );
        int row = m_channels.indexOf( episode->channel() );
        return createIndex( row , 0, episode->channel().data() );
    }
    else
    {
        return QModelIndex();
    }
}

int
PlaylistBrowserNS::PodcastModel::rowCount(const QModelIndex & parent) const
{
    //DEBUG_BLOCK
            
    if (parent.column() > 0) {
        //debug () << "0, cause 1";
        return 0;
    }

    if (!parent.isValid())
    {
        //debug () << m_channels.count();
        return m_channels.count();
    }
    else
    {
        Meta::PodcastMetaCommon *podcastMetaCommon = static_cast<Meta::PodcastMetaCommon *>(parent.internalPointer());

        if ( podcastMetaCommon->podcastType() ==  Meta::ChannelType )
        {
            Meta::PodcastChannel *channel = static_cast<Meta::PodcastChannel*>(parent.internalPointer());
            return channel->episodes().count();
        }
        else if ( podcastMetaCommon->podcastType() ==  Meta::EpisodeType )
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
PlaylistBrowserNS::PodcastModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 1;
}

Qt::ItemFlags
PlaylistBrowserNS::PodcastModel::flags(const QModelIndex & index) const
{
    if( index.isValid() )
    {
        return ( Qt::ItemIsEnabled     | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled |
                 Qt::ItemIsDragEnabled );
    }
    return Qt::ItemIsDropEnabled;
}

QVariant
PlaylistBrowserNS::PodcastModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch( section )
        {
            case 0: return i18n("Type");
            case 1: return i18n("Title");
            case 2: return i18n("Summary");
            default: return QVariant();
        }
    }

    return QVariant();
}

void
PlaylistBrowserNS::PodcastModel::slotUpdate()
{
    //TODO: emit dataChanged( QModelIndex(),  QModelIndex() );

    QList<Meta::PlaylistPtr> playlists =
    The::playlistManager()->playlistsOfCategory( PlaylistManager::PodcastChannel );
    QListIterator<Meta::PlaylistPtr> i(playlists);
    m_channels.clear();
    while (i.hasNext())
    {
        Meta::PodcastChannelPtr channel = Meta::PodcastChannelPtr::staticCast( i.next() );
        m_channels << channel;
    }

    emit layoutAboutToBeChanged();
    emit layoutChanged();
}

void
PlaylistBrowserNS::PodcastModel::addPodcast()
{
    debug() << "adding Podcast";

    //TODO: allow adding of Podcast to other than the Local Podcasts
    PlaylistProvider *provider = The::playlistManager()->playlistProvider(
            PlaylistManager::PodcastChannel, i18n( "Local Podcasts" ) );
    if( provider )
    {
        bool ok;
        QString url = QInputDialog::getText( 0,
                            i18n("Amarok"), i18n("Enter Podcast URL:"), QLineEdit::Normal,
                            QString(), &ok );
        if ( ok && !url.isEmpty() ) {
        // user entered something and pressed OK
        PodcastProvider * podcastProvider = static_cast<PodcastProvider *>(provider);
        podcastProvider->addPodcast( url );
        } else {
        // user entered nothing or pressed Cancel
            debug() << "invalid input or cancel";
        }
    }
    else
    {
        debug() << "PodcastChannel provider is null";
    }

}

void
PlaylistBrowserNS::PodcastModel::loadItems(QModelIndexList list, Playlist::AddOptions insertMode)
{
    Meta::TrackList episodes;
    Meta::PlaylistList channels;
    foreach( QModelIndex item, list )
    {
        Meta::PodcastMetaCommon *pmc = static_cast<Meta::PodcastMetaCommon *>( item.internalPointer() );
        switch( pmc->podcastType() )
        {
            case Meta::ChannelType:
                channels << Meta::PlaylistPtr( reinterpret_cast<Meta::PodcastChannel *>(pmc) );
                break;
            case Meta::EpisodeType:
                episodes << Meta::TrackPtr( reinterpret_cast<Meta::PodcastEpisode *>(pmc) ); break;
                default: debug() << "error, neither Channel nor Episode";
        }
    }
    The::playlistModel()->insertOptioned( episodes, insertMode );
    The::playlistModel()->insertOptioned( channels, insertMode );
}

void
PlaylistBrowserNS::PodcastModel::refreshItems( QModelIndexList list )
{
    DEBUG_BLOCK
    debug() << "number of items: " << list.count();
    foreach( QModelIndex index, list )
    {
        Meta::PodcastMetaCommon *pmc = static_cast<Meta::PodcastMetaCommon *>(index.internalPointer());
        if( typeid( * pmc ) == typeid( Meta::PodcastChannel ) )
        {
            refreshPodcast( Meta::PodcastChannelPtr(
                            reinterpret_cast<Meta::PodcastChannel *>(pmc) ) );
        }
    }
}

void
PlaylistBrowserNS::PodcastModel::refreshPodcasts()
{
    foreach( Meta::PodcastChannelPtr channel, m_channels )
    {
        refreshPodcast( channel );
    }
}

void
PlaylistBrowserNS::PodcastModel::refreshPodcast( Meta::PodcastChannelPtr channel )
{
    debug() << "refresh Podcast " << channel->title();
    PlaylistProvider *provider = The::playlistManager()->playlistProvider(
            PlaylistManager::PodcastChannel, i18n( "Local Podcasts" ) );
    if( provider )
    {
        PodcastProvider * podcastProvider = static_cast<PodcastProvider *>(provider);
        podcastProvider->update( channel );
    }
    else
    {
        debug() << "PodcastChannel provider is null";
    }
}

void
PlaylistBrowserNS::PodcastModel::downloadItems( QModelIndexList list )
{
    DEBUG_BLOCK
    debug() << "number of items: " << list.count();
    foreach( QModelIndex index, list )
    {
        Meta::PodcastMetaCommon *pmc = static_cast<Meta::PodcastMetaCommon *>(index.internalPointer());
        if( pmc->podcastType() ==  Meta::EpisodeType )
        {
            downloadEpisode( Meta::PodcastEpisodePtr( reinterpret_cast<Meta::PodcastEpisode *>(pmc) ) );
        }
        else if( pmc->podcastType() ==  Meta::ChannelType )
        {
            //TODO: download all (new) episodes
        }
    }
}

void
PlaylistBrowserNS::PodcastModel::downloadEpisode( Meta::PodcastEpisodePtr episode )
{
    DEBUG_BLOCK
    debug() << "downloading " << episode->title();

    PlaylistProvider *provider = The::playlistManager()->playlistProvider(
            PlaylistManager::PodcastChannel, i18n( "Local Podcasts" ) );
    if( provider )
    {
        PodcastProvider * podcastProvider = static_cast<PodcastProvider *>(provider);
        podcastProvider->downloadEpisode( episode );
    }
    else
    {
        debug() << "PodcastChannel provider is null";
    }
}

void
PlaylistBrowserNS::PodcastModel::configureChannels()
{
    debug() << "configure Channels";
}

void
PlaylistBrowserNS::PodcastModel::setPodcastsInterval()
{
    debug() << "set Podcasts update interval";
}

void
PlaylistBrowserNS::PodcastModel::emitLayoutChanged()
{
    DEBUG_BLOCK
    emit( layoutChanged() );
}





#include "PodcastModel.moc"
