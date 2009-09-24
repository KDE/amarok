/****************************************************************************************
 * Copyright (c) 2007 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include "PodcastModel.h"

#include "AmarokMimeData.h"
#include "Debug.h"
#include "PodcastMeta.h"
#include "PodcastProvider.h"
#include "context/popupdropper/libpud/PopupDropperItem.h"
#include "context/popupdropper/libpud/PopupDropper.h"
#include "PodcastCategory.h"
#include "playlistmanager/PlaylistManager.h"
#include "SvgHandler.h"

#include <QInputDialog>
#include <KIcon>
#include <QListIterator>
#include <typeinfo>

namespace The
{
    PlaylistBrowserNS::PodcastModel* podcastModel()
    {
        return PlaylistBrowserNS::PodcastModel::instance();
    }
}

PlaylistBrowserNS::PodcastModel* PlaylistBrowserNS::PodcastModel::s_instance = 0;

PlaylistBrowserNS::PodcastModel*
PlaylistBrowserNS::PodcastModel::instance()
{
    return s_instance ? s_instance : new PodcastModel();
}

void
PlaylistBrowserNS::PodcastModel::destroy()
{
    if (s_instance) {
        delete s_instance;
        s_instance = 0;
    }
}

PlaylistBrowserNS::PodcastModel::PodcastModel()
 : QAbstractItemModel()
 , m_appendAction( 0 )
 , m_loadAction( 0 )
{
    s_instance = this;
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

    if ( !index.isValid() ||
         ( role != Qt::DisplayRole && role != Qt::DecorationRole
           && role != Qt::ToolTipRole
           && role != ShortDescriptionRole && role != OnDiskRole ) )
    {
        return QVariant();
    }

    Meta::PodcastMetaCommon* pmc = static_cast<Meta::PodcastMetaCommon *>( index.internalPointer() );

    bool isChannel = false;
    QString title;
    QString description;
    KIcon icon;
    bool isOnDisk = false;
    if ( pmc->podcastType() == Meta::ChannelType )
    {
        Meta::PodcastChannel *channel = static_cast<Meta::PodcastChannel *>(index.internalPointer());
        title = channel->title();
        description = channel->description();
        isChannel = true;
        icon = KIcon( "podcast-amarok" );
    }
    else if ( pmc->podcastType() == Meta::EpisodeType )
    {
        Meta::PodcastEpisode *episode = static_cast<Meta::PodcastEpisode *>( index.internalPointer() );
        title = episode->title();
        description = episode->description();
        isChannel = false;
        isOnDisk = !episode->localUrl().isEmpty();
        if( isOnDisk )
        {
            icon = KIcon( "go-down" );
        }
        else
        {
            if( episode->isNew() )
                icon = KIcon( "rating" );
            else
                icon = KIcon( "podcast-amarok" );
        }
    }
    else
    {
        debug() << "Model index was neither Channel nor Episode";
        return QVariant();
    }

    if( role == Qt::DisplayRole )
        return title.simplified(); //whitespace removed from start and end + extra whitespace in between
    else if( role == Qt::DecorationRole )
        return QVariant( icon );
    else if( role == OnDiskRole )
        return QVariant( isOnDisk );
    else if( role == Qt::ToolTipRole )
        return title;

    // At this point role can only be ShortDescriptionRole
    return description;
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
            episode = channel->episodes()[row];
        else
            channel = 0;
    }

    if( !episode.isNull() )
        return createIndex( row, column, episode.data() );
    else if( !channel.isNull() )
        return createIndex( row, column, channel.data() );
    else
        return QModelIndex();

#if 0
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
#endif
}

QModelIndex
PlaylistBrowserNS::PodcastModel::parent(const QModelIndex & index) const
{
    if (!index.isValid())
        return QModelIndex();

    Meta::PodcastMetaCommon *podcastMetaCommon = static_cast<Meta::PodcastMetaCommon *>(index.internalPointer());

    if ( !podcastMetaCommon )
        return QModelIndex();

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

QStringList
PlaylistBrowserNS::PodcastModel::mimeTypes() const
{
    QStringList ret;
    ret << AmarokMimeData::PODCASTCHANNEL_MIME;
    ret << AmarokMimeData::PODCASTEPISODE_MIME;
    ret << "text/uri-list";
    return ret;
}

QMimeData*
PlaylistBrowserNS::PodcastModel::mimeData( const QModelIndexList &indexes ) const
{
    DEBUG_BLOCK
    AmarokMimeData* mime = new AmarokMimeData();

    Meta::PodcastChannelList channels;
    Meta::PodcastEpisodeList episodes;

    foreach( const QModelIndex &index, indexes )
    {
        Meta::PodcastMetaCommon* pmc = static_cast<Meta::PodcastMetaCommon *>( index.internalPointer() );

        if ( pmc->podcastType() == Meta::ChannelType )
        {
            Meta::PodcastChannel *channel = static_cast<Meta::PodcastChannel *>(index.internalPointer());
            channels << Meta::PodcastChannelPtr( channel );
        }
        else if ( pmc->podcastType() == Meta::EpisodeType )
        {
            Meta::PodcastEpisode *episode = static_cast<Meta::PodcastEpisode *>(index.internalPointer());
            episodes << Meta::PodcastEpisodePtr( episode );
        }
    }

    mime->setPodcastChannels( channels );
    mime->setPodcastEpisodes( episodes );

    return mime;
}

bool
PlaylistBrowserNS::PodcastModel::dropMimeData ( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent ) //reimplemented
{
    Q_UNUSED( column );
    Q_UNUSED( row );
    Q_UNUSED( parent );
//     DEBUG_BLOCK

    if( action == Qt::IgnoreAction )
        return true;

//     PlaylistGroupPtr parentGroup;

    if( data->hasFormat( AmarokMimeData::PODCASTCHANNEL_MIME ) )
    {
        debug() << "Found podcastchannel mime type";

        const AmarokMimeData* amarokMime = dynamic_cast<const AmarokMimeData*>( data );
        if( amarokMime )
        {
            Meta::PodcastChannelList channels = amarokMime->podcastChannels();

            foreach( Meta::PodcastChannelPtr channel, channels )
            {
                if( !m_channels.contains( channel ) )
                {
                    debug() << "unknown podcast channel dragged in: " << channel->title();
                    debug() << "TODO: start synchronization";
                }
                //else if( parent.contains(channel) )
                //TODO: reparent this channel
            }

            return true;
        }
    }
    else if( data->hasFormat( AmarokMimeData::PODCASTEPISODE_MIME ) )
    {
        debug() << "Found podcast episode mime type";
        debug() << "We don't support podcast episode drags yet.";
    }

    return false;
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

    //TODO: request the user to which PodcastProvider he wants to add it in case
    // of multiple (enabled) Podcast Providers.
    PodcastProvider * podcastProvider = The::playlistManager()->defaultPodcasts();
    if( podcastProvider )
    {
        bool ok;
        QString url = QInputDialog::getText( 0,
                            i18n("Add Podcast"), i18n("Enter RSS 2.0 feed URL:"), QLineEdit::Normal,
                            QString(), &ok );
        if ( ok && !url.isEmpty() )
        {
            // user entered something and pressed OK
            podcastProvider->addPodcast( url.trimmed() );
        }
        else
        {
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
PlaylistBrowserNS::PodcastModel::loadItems( QModelIndexList list, Playlist::AddOptions insertMode )
{
    Meta::TrackList episodes;
    Meta::PlaylistList channels;
    foreach( const QModelIndex &item, list )
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
    The::playlistController()->insertOptioned( episodes, insertMode );
    The::playlistController()->insertOptioned( channels, insertMode );
}

void
PlaylistBrowserNS::PodcastModel::refreshItems( QModelIndexList list )
{
    DEBUG_BLOCK
    debug() << "number of items: " << list.count();
    foreach( const QModelIndex &index, list )
    {
        Meta::PodcastMetaCommon *pmc = static_cast<Meta::PodcastMetaCommon *>(index.internalPointer());
        if( pmc->podcastType() == Meta::ChannelType )
        {
            refreshPodcast( Meta::PodcastChannelPtr(
                            reinterpret_cast<Meta::PodcastChannel *>(pmc) ) );
        }
    }
}

void
PlaylistBrowserNS::PodcastModel::removeSubscription( QModelIndexList list )
{
    DEBUG_BLOCK
    debug() << "number of items: " << list.count();
    foreach( const QModelIndex &index, list )
    {
        Meta::PodcastMetaCommon *pmc = static_cast<Meta::PodcastMetaCommon *>(index.internalPointer());
        if( pmc->podcastType() == Meta::ChannelType )
        {
            beginRemoveRows( QModelIndex(), index.row(), index.row() );
            removeSubscription( Meta::PodcastChannelPtr(
                            reinterpret_cast<Meta::PodcastChannel *>(pmc) ) );
            endRemoveRows();
        }
    }
}

void
PlaylistBrowserNS::PodcastModel::removeSubscription( Meta::PodcastChannelPtr channel )
{
    debug() << "remove Podcast subscription for " << channel->title();
    //HACK: since we only have one PodcastProvider implementation
    PlaylistProvider *provider = The::playlistManager()->defaultPodcasts();
    if( provider )
    {
        PodcastProvider * podcastProvider = static_cast<PodcastProvider *>(provider);
        podcastProvider->removeSubscription( channel );
    }
    else
    {
        debug() << "PodcastChannel provider is null";
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
    //HACK: since we only have one PodcastProvider implementation
    PlaylistProvider *provider = The::playlistManager()->defaultPodcasts();
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
    foreach( const QModelIndex &index, list )
    {
        Meta::PodcastMetaCommon *pmc = static_cast<Meta::PodcastMetaCommon *>(index.internalPointer());
        if( pmc->podcastType() ==  Meta::EpisodeType )
        {
            Meta::PodcastEpisodePtr episode
                = Meta::PodcastEpisodePtr( dynamic_cast<Meta::PodcastEpisode *>(pmc) );
            if( episode.isNull() )
                debug() << "could not downcast PodcastMetaCommon pointer!";
            else
                downloadEpisode( episode );
        }
        else if( pmc->podcastType() ==  Meta::ChannelType )
        {
            //TODO: download all (new) episodes
        }
    }
}

void
PlaylistBrowserNS::PodcastModel::deleteItems( QModelIndexList list )
{
    DEBUG_BLOCK
    debug() << "number of items: " << list.count();
    foreach( const QModelIndex &index, list )
    {
        Meta::PodcastMetaCommon *pmc = static_cast<Meta::PodcastMetaCommon *>(index.internalPointer());
        if( pmc->podcastType() ==  Meta::EpisodeType )
        {
            deleteDownloadedEpisode( Meta::PodcastEpisodePtr( reinterpret_cast<Meta::PodcastEpisode *>(pmc) ) );
        }
        else if( pmc->podcastType() ==  Meta::ChannelType )
        {
            //TODO: put something here or take this if clause out
            // ignore
        }
    }
}

void
PlaylistBrowserNS::PodcastModel::downloadEpisode( Meta::PodcastEpisodePtr episode )
{
    DEBUG_BLOCK
    debug() << "downloading " << episode->title();
    //HACK: since we only have one PodcastProvider implementation
    PlaylistProvider *provider = The::playlistManager()->defaultPodcasts();
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
PlaylistBrowserNS::PodcastModel::deleteDownloadedEpisode( Meta::PodcastEpisodePtr episode )
{
    DEBUG_BLOCK
    debug() << "deleting " << episode->title();
    //HACK: since we only have one PodcastProvider implementation
    PlaylistProvider *provider = The::playlistManager()->playlistProvider(
    PlaylistManager::PodcastChannel, i18n( "Local Podcasts" ) );
    if( provider )
    {
        PodcastProvider * podcastProvider = static_cast<PodcastProvider *>(provider);
        podcastProvider->deleteDownloadedEpisode( episode );
    }
    else
    {
        debug() << "PodcastChannel provider is null";
    }
}

void
PlaylistBrowserNS::PodcastModel::configureChannels( QModelIndexList list )
{
    foreach( const QModelIndex &index, list )
    {
        Meta::PodcastMetaCommon *pmc = static_cast<Meta::PodcastMetaCommon *>(index.internalPointer());
        if( pmc->podcastType() ==  Meta::ChannelType )
        {
            configureChannel( Meta::PodcastChannelPtr( reinterpret_cast<Meta::PodcastChannel *>(pmc) ) );
            return;
        }
    }
}

void
PlaylistBrowserNS::PodcastModel::configureChannel( Meta::PodcastChannelPtr channel )
{
    DEBUG_BLOCK
    debug() << "configuring " << channel->title();
    //HACK: since we only have one PodcastProvider implementation
    PlaylistProvider *provider = The::playlistManager()->defaultPodcasts();
    if( provider )
    {
        PodcastProvider * podcastProvider = static_cast<PodcastProvider *>(provider);
        podcastProvider->configureChannel( channel );
    }
    else
    {
        debug() << "PodcastChannel provider is null";
    }
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

QList<QAction *>
PlaylistBrowserNS::PodcastModel::actionsFor( const QModelIndexList &indices )
{
    QList<QAction *> actions;

    m_selectedEpisodes.clear();
    m_selectedChannels.clear();
    m_selectedEpisodes << selectedEpisodes( indices );
    m_selectedChannels << selectedChannels( indices );

    if( indices.isEmpty() )
        return actions;

    actions << createCommonActions( indices );

    //HACK: since we only have one PodcastProvider implementation
    PodcastProvider *provider = The::playlistManager()->defaultPodcasts();
    if( !provider )
        return actions;

    if( !m_selectedChannels.isEmpty() )
        actions << provider->channelActions( m_selectedChannels );
    else if( !m_selectedEpisodes.isEmpty() )
        actions << provider->episodeActions( m_selectedEpisodes );

    return actions;
}

QList< QAction * >
PlaylistBrowserNS::PodcastModel::createCommonActions( QModelIndexList indices )
{
    Q_UNUSED( indices )
    QList< QAction * > actions;

    if( m_appendAction == 0 )
    {
        m_appendAction = new QAction(
            KIcon( "media-track-add-amarok" ),
            i18n( "&Add to Playlist" ),
            this
        );
        m_appendAction->setProperty( "popupdropper_svg_id", "append" );
        connect( m_appendAction, SIGNAL( triggered() ), this, SLOT( slotAppend() ) );
    }

    if( m_loadAction == 0 )
    {
        m_loadAction = new QAction(
            KIcon( "folder-open" ),
            i18nc( "Replace the currently loaded tracks with these",
            "&Replace Playlist" ),
            this
        );
        m_loadAction->setProperty( "popupdropper_svg_id", "load" );
        connect( m_loadAction, SIGNAL( triggered() ), this, SLOT( slotLoad() ) );
    }

    actions << m_appendAction;
    actions << m_loadAction;

    return actions;
}

Meta::PodcastChannelList
PlaylistBrowserNS::PodcastModel::selectedChannels( const QModelIndexList &indices )
{
    Meta::PodcastChannelList channels;
    Meta::PodcastMetaCommon *pmc = 0;
    foreach( const QModelIndex &index, indices )
    {
        if( !index.isValid() )
            break;

        pmc = static_cast<Meta::PodcastMetaCommon *>(index.internalPointer());
        if( !pmc )
            break;

        switch( pmc->podcastType() )
        {
            case Meta::EpisodeType:
                break;
            case Meta::ChannelType:
                channels << Meta::PodcastChannelPtr( static_cast<Meta::PodcastChannel *>(pmc) );
                break;
        }
    }
    return channels;
}

Meta::PodcastEpisodeList
PlaylistBrowserNS::PodcastModel::selectedEpisodes( const QModelIndexList &indices )
{
    Meta::PodcastEpisodeList episodes;
    Meta::PodcastMetaCommon *pmc = 0;
    foreach( const QModelIndex &index, indices )
    {
        if( !index.isValid() )
            break;

        pmc = static_cast<Meta::PodcastMetaCommon *>(index.internalPointer());
        switch( pmc->podcastType() )
        {
            case Meta::EpisodeType:
                episodes << Meta::PodcastEpisodePtr( static_cast<Meta::PodcastEpisode *>(pmc) );
                break;
            case Meta::ChannelType:
                episodes << static_cast<Meta::PodcastChannel *>(pmc)->episodes();
                break;
        }
    }
    return episodes;
}

void
PlaylistBrowserNS::PodcastModel::slotAppend()
{
    Meta::PodcastEpisodeList episodes = selectedEpisodes();
    if( !episodes.empty() )
        The::playlistController()->insertOptioned(
                podcastEpisodesToTracks( episodes ), Playlist::Append );
}

void
PlaylistBrowserNS::PodcastModel::slotLoad()
{
    Meta::PodcastEpisodeList episodes = selectedEpisodes();
    if( !episodes.empty() )
        The::playlistController()->insertOptioned(
                podcastEpisodesToTracks( episodes ), Playlist::Replace );
}

Meta::TrackList
PlaylistBrowserNS::PodcastModel::podcastEpisodesToTracks( Meta::PodcastEpisodeList episodes )
{
    Meta::TrackList tracks;
    foreach( Meta::PodcastEpisodePtr episode, episodes )
        tracks << Meta::TrackPtr::staticCast( episode );
    return tracks;
}
#include "PodcastModel.moc"
