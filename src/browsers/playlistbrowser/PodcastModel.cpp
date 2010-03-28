/****************************************************************************************
 * Copyright (c) 2007-2010 Bart Cerneels <bart.cerneels@kde.org>                        *
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
#include "core/support/Debug.h"
#include "OpmlParser.h"
#include "core/podcasts/PodcastMeta.h"
#include "core/podcasts/PodcastProvider.h"
#include "core/podcasts/PodcastImageFetcher.h"
#include "context/popupdropper/libpud/PopupDropperItem.h"
#include "context/popupdropper/libpud/PopupDropper.h"
#include "PodcastCategory.h"
#include "playlistmanager/PlaylistManager.h"
#include "SvgHandler.h"

#include <ThreadWeaver/Weaver>

#include <QInputDialog>
#include <KIcon>
#include <QListIterator>
#include <QtAlgorithms>
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

// to be used with qSort
static bool
lessThanChannelTitles(const Podcasts::PodcastChannelPtr & lhs, const Podcasts::PodcastChannelPtr & rhs)
{
    return lhs->title().toLower() < rhs->title().toLower();
}

PlaylistBrowserNS::PodcastModel::PodcastModel()
 : QAbstractItemModel()
 , m_appendAction( 0 )
 , m_loadAction( 0 )
 , m_setNewAction( 0 )
{
    s_instance = this;
    QList<Meta::PlaylistPtr> playlists =
            The::playlistManager()->playlistsOfCategory( PlaylistManager::PodcastChannel );
    QListIterator<Meta::PlaylistPtr> i(playlists);
    while (i.hasNext())
        m_channels << Podcasts::PodcastChannelPtr::staticCast( i.next() );

    qSort(m_channels.begin(), m_channels.end(), lessThanChannelTitles);

    connect( The::playlistManager(), SIGNAL( updated() ), SLOT( slotUpdate() ) );
    connect( The::playlistManager(), SIGNAL( providerRemoved( PlaylistProvider*, int ) ),
             SLOT( slotUpdate() ) );
}

PlaylistBrowserNS::PodcastModel::~PodcastModel()
{
}

bool
PlaylistBrowserNS::PodcastModel::isOnDisk( Podcasts::PodcastMetaCommon *pmc ) const
{
    bool isOnDisk = false;
    if( pmc->podcastType() == Podcasts::EpisodeType )
    {
        Podcasts::PodcastEpisode *episode = static_cast<Podcasts::PodcastEpisode *>( pmc );
        KUrl episodeFile( episode->localUrl() );

        if( !episodeFile.isEmpty() )
        {
            isOnDisk = QFileInfo( episodeFile.toLocalFile() ).exists();
            //reset localUrl because the file is not there.
            if( !isOnDisk )
                episode->setLocalUrl( KUrl() );
        }            
    }

    return isOnDisk;
}

QVariant
PlaylistBrowserNS::PodcastModel::icon( Podcasts::PodcastMetaCommon *pmc ) const
{
    Podcasts::PodcastChannel *channel = 0;
    Podcasts::PodcastEpisode *episode = 0;
    QStringList emblems;
    
    switch( pmc->podcastType() )
    {
        case Podcasts::ChannelType:
            channel = static_cast<Podcasts::PodcastChannel *>( pmc );

            //TODO: only check visible episodes. For now those are all returned by episodes().
            foreach( const Podcasts::PodcastEpisodePtr ep, channel->episodes() )
            {
                if( ep->isNew() )
                {
                    emblems << "rating";
                    break;
                }
            }

            if( channel->hasImage() )
            {
                QSize size( channel->image().size() );
                QPixmap pixmap( 32, 32 );
                pixmap.fill( Qt::transparent );

                size.scale( 32, 32, Qt::KeepAspectRatio );

                int x = 32 / 2 - size.width()  / 2;
                int y = 32 / 2 - size.height() / 2;

                QPainter p( &pixmap );
                p.drawPixmap( x, y, channel->image().scaled( size,
                                                             Qt::KeepAspectRatio,
                                                             Qt::SmoothTransformation ) );
                
                // if it's a new episode draw the overlay:
                if( !emblems.isEmpty() )
                {
                    // draw the overlay the same way KIconLoader does:
                    p.drawPixmap( 2, 32 - 16 - 2, KIcon( "rating" ).pixmap( 16, 16 ) );
                }

                p.end();

                return pixmap;
            }
            else
            {

                return KIcon( "podcast-amarok", 0, emblems ).pixmap( 32, 32 );
            }

        case Podcasts::EpisodeType:
            episode = static_cast<Podcasts::PodcastEpisode *>( pmc );

            if( isOnDisk( pmc ) )
                emblems << "go-down";

            if( episode->isNew() )
                return KIcon( "rating", 0, emblems ).pixmap( 24, 24 );
            else
                return KIcon( "podcast-amarok", 0, emblems ).pixmap( 24, 24 );
    }

    return QVariant();
}

QVariant
PlaylistBrowserNS::PodcastModel::data( const QModelIndex &index, int role ) const
{
    if( index.row() == -1 && index.column() == ProviderColumn )
    {
        QVariantList displayList;
        QVariantList iconList;
        QVariantList playlistCountList;
        QVariantList providerActionsCountList;
        QVariantList providerActionsList;
        QVariantList providerByLineList;

        //get data from empty providers
        PlaylistProviderList providerList =
                The::playlistManager()->providersForCategory( PlaylistManager::PodcastChannel );
        foreach( PlaylistProvider *provider, providerList )
        {
            if( provider->playlistCount() > 0 || provider->playlists().count() > 0 )
                continue;

            displayList << provider->prettyName();
            iconList << provider->icon();
            playlistCountList << provider->playlists().count();
            providerActionsCountList << provider->providerActions().count();
            providerActionsList <<  QVariant::fromValue( provider->providerActions() );
            providerByLineList << i18ncp( "number of podcasts from one source", "One channel",
                           "%1 channels", provider->providerActions().count() );
        }

        switch( role )
        {
            case Qt::DisplayRole:
            case DescriptionRole:
            case Qt::ToolTipRole:
                return displayList;
            case Qt::DecorationRole: return iconList;
            case MetaPlaylistModel::ActionCountRole: return providerActionsCountList;
            case MetaPlaylistModel::ActionRole: return providerActionsList;
            case MetaPlaylistModel::ByLineRole: return providerByLineList;
            case Qt::EditRole: return QVariant();
        }
    }

    if( !index.isValid() )
        return QVariant();
    
    Podcasts::PodcastMetaCommon* pmc =
            static_cast<Podcasts::PodcastMetaCommon *>( index.internalPointer() );

    if( !pmc )
        return QVariant();

    switch( role )
    {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            switch( index.column() )
            {
                case MetaPlaylistModel::PlaylistColumn:
                    return pmc->title();

                case SubtitleColumn:
                    return pmc->subtitle();

                case AuthorColumn:
                    return pmc->author();

                case KeywordsColumn:
                    return pmc->keywords();

                case FilesizeColumn:
                    if( pmc->podcastType() == Podcasts::EpisodeType )
                        return static_cast<Podcasts::PodcastEpisode *>( pmc )
                            ->filesize();
                    break;

                case ImageColumn:
                    if( pmc->podcastType() == Podcasts::ChannelType )
                    {
                        Podcasts::PodcastChannel *pc =
                                static_cast<Podcasts::PodcastChannel *>( pmc );
                        KUrl imageUrl( PodcastImageFetcher::cachedImagePath( pc ) );

                        if( !QFile( imageUrl.toLocalFile() ).exists() )
                        {
                            imageUrl = pc->imageUrl();
                        }
                        return imageUrl;
                    }
                    break;

                case DateColumn:
                    if( pmc->podcastType() == Podcasts::EpisodeType )
                        return static_cast<Podcasts::PodcastEpisode *>( pmc )
                            ->pubDate();
                    else
                        return static_cast<Podcasts::PodcastChannel *>( pmc )
                            ->subscribeDate();

                case IsEpisodeColumn:
                    return bool( pmc->podcastType() == Podcasts::EpisodeType );

                case ProviderColumn:
                {
                    Podcasts::PodcastProvider *provider = providerForPmc( pmc );
                    if( !provider )
                        break;

                    return provider->prettyName();
                }

                case OnDiskColumn:
                    return isOnDisk( pmc );
            }
            break;

        case ShortDescriptionRole:
            if( index.column() == MetaPlaylistModel::PlaylistColumn )
                return pmc->description();
            break;

        case Qt::DecorationRole:
        {
            switch( index.column() )
            {
                case MetaPlaylistModel::PlaylistColumn:
                    return icon( pmc );
                case ProviderColumn:
                {
                    PlaylistProvider *provider = providerForPmc(
                            static_cast<Podcasts::PodcastMetaCommon *>( index.internalPointer() ) );
                    if( !provider )
                        return KIcon( "server-database" );

                    return provider->icon();
                }
            }
            break;
        }

        case PlaylistBrowserNS::MetaPlaylistModel::ByLineRole:
        {
            if( index.column() == ProviderColumn )
            {
                PlaylistProvider *provider = providerForPmc(
                        static_cast<Podcasts::PodcastMetaCommon *>( index.internalPointer() ) );
                if( !provider )
                    return QString();

                //TODO: first check playlistCount and show "loading"
                int playlistCount = provider->playlists().count();

                return i18ncp( "number of podcasts from one source", "One channel",
                               "%1 channels", playlistCount );
            }
            return QString();
        }

        case PlaylistBrowserNS::MetaPlaylistModel::ActionCountRole:
        {
            if( index.column() == ProviderColumn )
            {
                PlaylistProvider *provider = providerForPmc(
                        static_cast<Podcasts::PodcastMetaCommon *>( index.internalPointer() ) );
                if( provider )
                    return provider->providerActions().count();
            }
        }

        case PlaylistBrowserNS::MetaPlaylistModel::ActionRole:
        {
            PlaylistProvider *provider = providerForPmc(
                    static_cast<Podcasts::PodcastMetaCommon *>( index.internalPointer() ) );
            if( !provider )
                return QVariant();

            if( index.column() == ProviderColumn )
                return QVariant::fromValue( provider->providerActions() );
            else if( index.column() == PlaylistColumn )
            {
                Podcasts::PodcastProvider *podcastProvider = dynamic_cast<Podcasts::PodcastProvider *>( provider );
                if( !podcastProvider )
                    return QVariant();

                switch( podcastItemType( index ) )
                {
                    case Podcasts::ChannelType:
                    {
                        Podcasts::PodcastChannelPtr channel = channelForIndex( index );
                        Podcasts::PodcastChannelList channels;
                        channels << channel;
                        return QVariant::fromValue( podcastProvider->channelActions( channels ) );
                    }
                    case Podcasts::EpisodeType:
                    {
                        Podcasts::PodcastEpisodePtr episode = episodeForIndex( index );
                        Podcasts::PodcastEpisodeList episodes;
                        episodes << episode;
                        return QVariant::fromValue( podcastProvider->episodeActions( episodes ) );
                    }
                }
            }
        }
    }

    return QVariant();
}

bool
PlaylistBrowserNS::PodcastModel::setData( const QModelIndex &idx, const QVariant &value, int role )
{
    if( !idx.isValid() )
        return false;

    if( idx.column() == ProviderColumn )
    {
        if( role == Qt::DisplayRole )
        {
            PlaylistProvider *provider = getProviderByName( value.toString() );
            if( !provider )
                return false;
            Podcasts::PodcastProvider *podcastProvider = dynamic_cast<Podcasts::PodcastProvider *>( provider );
            if( !podcastProvider )
                    return false;

            switch( podcastItemType( idx ) )
            {
                case Podcasts::ChannelType:
                {
                    Podcasts::PodcastChannelPtr channel = channelForIndex( idx );
                    if( !channel )
                        return false;
                    debug() << QString( "Copy podcast channel \"%1\" to \"%2\"." )
                            .arg( channel->prettyName() ).arg( provider->prettyName() );
                    return !podcastProvider->addChannel( channel ).isNull();
                }
                case Podcasts::EpisodeType:
                {
                    Podcasts::PodcastEpisodePtr episode = episodeForIndex( idx );
                    if( !episode )
                        return false;
                    debug() << QString( "Copy podcast episode \"%1\" to \"%2\"." )
                            .arg( episode->prettyName() ).arg( provider->prettyName() );
                    return !podcastProvider->addEpisode( episode ).isNull();
                }
                default: return false;
            }
        }
        //return true even for the data we didn't handle to get QAbstractItemModel::setItemData to work
        //TODO: implement setItemData()
        return true;
    }

    return false;
}

QModelIndex
PlaylistBrowserNS::PodcastModel::index(int row, int column, const QModelIndex & parent) const
{
    //there are valid indexes available with row == -1 for empty groups and providers
    if( !parent.isValid() && row == -1 && column >= 0 )
        return createIndex( row, column, row );

    if( !hasIndex(row, column, parent) )
        return QModelIndex();

    Podcasts::PodcastChannelPtr channel;
    Podcasts::PodcastEpisodePtr episode;

    if( !parent.isValid() )
        channel = m_channels[row];
    else
    {
        channel = static_cast<Podcasts::PodcastChannel *>(parent.internalPointer());
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
}

QModelIndex
PlaylistBrowserNS::PodcastModel::parent( const QModelIndex &index ) const
{
    if (!index.isValid())
        return QModelIndex();

    Podcasts::PodcastMetaCommon *podcastMetaCommon =
            static_cast<Podcasts::PodcastMetaCommon *>(index.internalPointer());

    if ( !podcastMetaCommon )
        return QModelIndex();

    if ( podcastMetaCommon->podcastType() ==  Podcasts::ChannelType )
    {
        return QModelIndex();
    }
    else if ( podcastMetaCommon->podcastType() ==  Podcasts::EpisodeType )
    {
        Podcasts::PodcastEpisode *episode =
                static_cast<Podcasts::PodcastEpisode *>( index.internalPointer() );
        if( !episode )
            return QModelIndex();

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
        Podcasts::PodcastMetaCommon *podcastMetaCommon =
                static_cast<Podcasts::PodcastMetaCommon *>(parent.internalPointer());

        if( !podcastMetaCommon )
            return 0;

        if ( podcastMetaCommon->podcastType() ==  Podcasts::ChannelType )
        {
            Podcasts::PodcastChannel *channel =
                    static_cast<Podcasts::PodcastChannel *>(parent.internalPointer());
            if( !channel )
                return 0;
            return channel->episodes().count();
        }
        else if ( podcastMetaCommon->podcastType() ==  Podcasts::EpisodeType )
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
    return ColumnCount;
}

Qt::ItemFlags
PlaylistBrowserNS::PodcastModel::flags(const QModelIndex & index) const
{
    if( index.row() == -1 )
    {
        switch( index.column() )
        {
            case ProviderColumn:
                return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
            default: break;
        }
    }

    Qt::ItemFlags channelFlags;
    if( podcastItemType( index ) == Podcasts::ChannelType )
    {
        channelFlags = Qt::ItemIsDropEnabled;
        if( index.column() == ProviderColumn )
            channelFlags |= Qt::ItemIsEditable;
    }

    return ( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled
                 | channelFlags );
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

    Podcasts::PodcastChannelList channels;
    Podcasts::PodcastEpisodeList episodes;

    foreach( const QModelIndex &index, indexes )
    {
        Podcasts::PodcastMetaCommon* pmc =
                static_cast<Podcasts::PodcastMetaCommon *>( index.internalPointer() );
        if( !pmc )
            return new QMimeData();

        if ( pmc->podcastType() == Podcasts::ChannelType )
        {
            Podcasts::PodcastChannel *channel =
                    static_cast<Podcasts::PodcastChannel *>(index.internalPointer());
            if( !channel )
                return new QMimeData();

            channels << Podcasts::PodcastChannelPtr( channel );
        }
        else if ( pmc->podcastType() == Podcasts::EpisodeType )
        {
            Podcasts::PodcastEpisode *episode =
                    static_cast<Podcasts::PodcastEpisode *>(index.internalPointer());
            if( !episode )
                return new QMimeData();

            episodes << Podcasts::PodcastEpisodePtr( episode );
        }
    }

    mime->setPodcastChannels( channels );
    mime->setPodcastEpisodes( episodes );
    QList<QUrl> urls;
    foreach( const Podcasts::PodcastChannelPtr channel, channels )
        urls << channel->url();
    foreach( const Podcasts::PodcastEpisodePtr episode, episodes )
        urls << episode->playableUrl();
    mime->setUrls( urls );

    return mime;
}

bool
PlaylistBrowserNS::PodcastModel::dropMimeData( const QMimeData * data, Qt::DropAction action,
                                               int row, int column, const QModelIndex &parent )
{
    Q_UNUSED( column );
    Q_UNUSED( row );

    if( action == Qt::IgnoreAction )
        return true;

    if( data->hasFormat( OpmlParser::OPML_MIME ) )
    {
        importOpml( KUrl( data->data( OpmlParser::OPML_MIME ) ) );
        return true;
    }

    const AmarokMimeData* amarokMime = dynamic_cast<const AmarokMimeData*>( data );
    if( !amarokMime )
        return false;

    if( data->hasFormat( AmarokMimeData::PODCASTCHANNEL_MIME ) )
    {
        debug() << "Dropped podcastchannel mime type";

        Podcasts::PodcastChannelList channels = amarokMime->podcastChannels();

        foreach( Podcasts::PodcastChannelPtr channel, channels )
        {
            if( !m_channels.contains( channel ) )
            {
                debug() << "unknown podcast channel dragged in: " << channel->title();
                debug() << "TODO: start synchronization";
            }
        }

        return true;
    }
    else if( data->hasFormat( AmarokMimeData::PODCASTEPISODE_MIME ) )
    {
        debug() << "Dropped podcast episode mime type";
        debug() << "on " << parent << " row: " << row;

       if( podcastItemType( parent ) != Podcasts::ChannelType )
           return false;

        Podcasts::PodcastChannelPtr channel = channelForIndex( parent );
        if( !channel )
            return false;

        Podcasts::PodcastEpisodeList episodes = amarokMime->podcastEpisodes();
        bool allAdded = true;
        foreach( Podcasts::PodcastEpisodePtr episode, episodes )
        {
            //TODO: implement PodcastChannel::operator=( PodcastChannelPtr )
            if( episode->channel()->title() == channel->title() )
            {
                if( episode->channel() == channel )
                    allAdded = false; //don't add episode to it's parent channel. Causes duplicates.
                else
                    allAdded = channel->addEpisode( episode ) ? allAdded : false;
            }
        }

        return allAdded;
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
    while( i.hasNext() )
    {
        Podcasts::PodcastChannelPtr channel = Podcasts::PodcastChannelPtr::staticCast( i.next() );
        m_channels << channel;
    }

    qSort(m_channels.begin(), m_channels.end(), lessThanChannelTitles);

    emit layoutAboutToBeChanged();
    emit layoutChanged();
}

void
PlaylistBrowserNS::PodcastModel::addPodcast()
{
    debug() << "adding Podcast";

    //TODO: request the user to which PodcastProvider he wants to add it in case
    // of multiple (enabled) Podcast Providers.
    Podcasts::PodcastProvider * podcastProvider = The::playlistManager()->defaultPodcasts();
    if( podcastProvider )
    {
        bool ok;
        QString url = QInputDialog::getText( 0,
                            i18n("Add Podcast"),
                            i18n("Enter RSS 1.0/2.0 or Atom feed URL:"),
                            QLineEdit::Normal,
                            QString(),
                            &ok );
        if( ok && !url.isEmpty() )
        {
            // user entered something and pressed OK
            podcastProvider->addPodcast( Podcasts::PodcastProvider::toFeedUrl( url.trimmed() ) );
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
        Podcasts::PodcastMetaCommon *pmc =
                static_cast<Podcasts::PodcastMetaCommon *>( item.internalPointer() );
        if( !pmc )
            return;

        switch( pmc->podcastType() )
        {
            case Podcasts::ChannelType:
                channels << Meta::PlaylistPtr( reinterpret_cast<Podcasts::PodcastChannel *>(pmc) );
                break;
            case Podcasts::EpisodeType:
                episodes <<
                        Meta::TrackPtr( reinterpret_cast<Podcasts::PodcastEpisode *>(pmc) ); break;
                default: debug() << "error, neither Channel nor Episode";
        }
    }
    The::playlistController()->insertOptioned( episodes, insertMode );
    The::playlistController()->insertOptioned( channels, insertMode );
}

void
PlaylistBrowserNS::PodcastModel::trackAdded( Meta::PlaylistPtr playlist, Meta::TrackPtr track,
                                 int position )
{
    DEBUG_BLOCK
    debug() << "From playlist: " << playlist->prettyName();
    debug() << "Track: " << track->prettyName() << "position: " << position;
    //TODO: rowInserted()
}

void
PlaylistBrowserNS::PodcastModel::trackRemoved( Meta::PlaylistPtr playlist, int position )
{
    DEBUG_BLOCK
    debug() << "From playlist: " << playlist->prettyName();
    debug() << "position: " << position;
    //TODO: beginRemoveRows() && endRemoveRows()
}

void
PlaylistBrowserNS::PodcastModel::refreshItems( QModelIndexList list )
{
    DEBUG_BLOCK
    debug() << "number of items: " << list.count();
    foreach( const QModelIndex &index, list )
    {
        Podcasts::PodcastMetaCommon *pmc =
                static_cast<Podcasts::PodcastMetaCommon *>(index.internalPointer());
        if( !pmc )
            return;
        if( pmc->podcastType() == Podcasts::ChannelType )
        {
            refreshPodcast( Podcasts::PodcastChannelPtr(
                            reinterpret_cast<Podcasts::PodcastChannel *>(pmc) ) );
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
        Podcasts::PodcastMetaCommon *pmc =
                static_cast<Podcasts::PodcastMetaCommon *>(index.internalPointer());
        if( !pmc )
            return;

        if( pmc->podcastType() == Podcasts::ChannelType )
        {
            beginRemoveRows( QModelIndex(), index.row(), index.row() );
            removeSubscription( Podcasts::PodcastChannelPtr(
                            reinterpret_cast<Podcasts::PodcastChannel *>(pmc) ) );
            endRemoveRows();
        }
    }
}

void
PlaylistBrowserNS::PodcastModel::removeSubscription( Podcasts::PodcastChannelPtr channel )
{
    debug() << "remove Podcast subscription for " << channel->title();
    //HACK: since we only have one PodcastProvider implementation
    PlaylistProvider *provider = The::playlistManager()->defaultPodcasts();
    if( provider )
    {
        Podcasts::PodcastProvider *podcastProvider = dynamic_cast<Podcasts::PodcastProvider *>(provider);
        if( !podcastProvider )
            return;
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
    foreach( Podcasts::PodcastChannelPtr channel, m_channels )
    {
        refreshPodcast( channel );
    }
}

void
PlaylistBrowserNS::PodcastModel::refreshPodcast( Podcasts::PodcastChannelPtr channel )
{
    debug() << "refresh Podcast " << channel->title();
    //HACK: since we only have one PodcastProvider implementation
    PlaylistProvider *provider = The::playlistManager()->defaultPodcasts();
    if( provider )
    {
        Podcasts::PodcastProvider * podcastProvider = dynamic_cast<Podcasts::PodcastProvider *>(provider);
        if( !podcastProvider )
            return;

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
        Podcasts::PodcastMetaCommon *pmc =
                static_cast<Podcasts::PodcastMetaCommon *>(index.internalPointer());
        if( !pmc )
            return;

        if( pmc->podcastType() ==  Podcasts::EpisodeType )
        {
            Podcasts::PodcastEpisodePtr episode
                = Podcasts::PodcastEpisodePtr( static_cast<Podcasts::PodcastEpisode *>(pmc) );
            if( episode.isNull() )
                debug() << "could not downcast PodcastMetaCommon pointer!";
            else
                downloadEpisode( episode );
        }
        else if( pmc->podcastType() ==  Podcasts::ChannelType )
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
        Podcasts::PodcastMetaCommon *pmc =
                static_cast<Podcasts::PodcastMetaCommon *>(index.internalPointer());
        if( !pmc )
            return;

        if( pmc->podcastType() ==  Podcasts::EpisodeType )
        {
            deleteDownloadedEpisode( Podcasts::PodcastEpisodePtr( reinterpret_cast<Podcasts::PodcastEpisode *>(pmc) ) );
        }
        else if( pmc->podcastType() ==  Podcasts::ChannelType )
        {
            //TODO: put something here or take this if clause out
            // ignore
        }
    }
}

void
PlaylistBrowserNS::PodcastModel::downloadEpisode( Podcasts::PodcastEpisodePtr episode )
{
    DEBUG_BLOCK
    debug() << "downloading " << episode->title();
    //HACK: since we only have one PodcastProvider implementation
    PlaylistProvider *provider = The::playlistManager()->defaultPodcasts();
    if( provider )
    {
        Podcasts::PodcastProvider *podcastProvider = dynamic_cast<Podcasts::PodcastProvider *>(provider);
        if( !podcastProvider )
            return;
        podcastProvider->downloadEpisode( episode );
    }
    else
    {
        debug() << "PodcastChannel provider is null";
    }
}

void
PlaylistBrowserNS::PodcastModel::deleteDownloadedEpisode( Podcasts::PodcastEpisodePtr episode )
{
    DEBUG_BLOCK
    debug() << "deleting " << episode->title();
    //HACK: since we only have one PodcastProvider implementation
    PlaylistProvider *provider = The::playlistManager()->playlistProvider(
    PlaylistManager::PodcastChannel, i18n( "Local Podcasts" ) );
    if( provider )
    {
        Podcasts::PodcastProvider *podcastProvider = dynamic_cast<Podcasts::PodcastProvider *>(provider);
        if( !podcastProvider )
            return;

        podcastProvider->deleteDownloadedEpisode( episode );
    }
    else
    {
        debug() << "PodcastChannel provider is null";
    }
}

void
PlaylistBrowserNS::PodcastModel::configureChannel( Podcasts::PodcastChannelPtr channel )
{
    DEBUG_BLOCK
    debug() << "configuring " << channel->title();
    //HACK: since we only have one PodcastProvider implementation
    PlaylistProvider *provider = The::playlistManager()->defaultPodcasts();
    if( provider )
    {
        Podcasts::PodcastProvider *podcastProvider = dynamic_cast<Podcasts::PodcastProvider *>(provider);
        if( !podcastProvider )
            return;

        podcastProvider->configureChannel( channel );
    }
    else
    {
        debug() << "PodcastChannel provider is null";
    }
}

void
PlaylistBrowserNS::PodcastModel::importOpml( const KUrl &url )
{
    if( !url.isValid() )
        return;

    debug() << "Importing OPML file from " << url;

    OpmlParser *parser = new OpmlParser( url.toLocalFile() );
    connect( parser, SIGNAL( outlineParsed( OpmlOutline * ) ),
             SLOT( slotOpmlOutlineParsed( OpmlOutline * ) ) );
    connect( parser, SIGNAL( doneParsing() ), SLOT( slotOpmlParsingDone() ) );

    ThreadWeaver::Weaver::instance()->enqueue( parser );
}

void
PlaylistBrowserNS::PodcastModel::slotOpmlOutlineParsed( OpmlOutline *outline )
{
    if( !outline )
        return;

    if( outline->hasChildren() )
        return; //TODO grouping handling once PodcastCategory has it.

    if( outline->attributes().contains( "xmlUrl" ) )
    {
        KUrl url( outline->attributes().value( "xmlUrl" ).trimmed() );
        if( !url.isValid() )
        {
            debug() << "OPML outline contained an invalid url: " << url;
            return; //TODO signal invalid feed to user
        }

        //TODO: handle multiple providers
        Podcasts::PodcastProvider *podcastProvider = The::playlistManager()->defaultPodcasts();
        if( podcastProvider )
            podcastProvider->addPodcast( url );
    }
}

void
PlaylistBrowserNS::PodcastModel::slotOpmlParsingDone()
{
    debug() << "Done parsing OPML file";
    //TODO: print number of imported channels
    sender()->deleteLater();
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

    if( !m_selectedChannels.isEmpty() )
    {
        QMultiMap<Podcasts::PodcastProvider *,Podcasts::PodcastChannelPtr> channelMap;
        foreach( Podcasts::PodcastChannelPtr channel, m_selectedChannels )
        {
            Podcasts::PodcastProvider *provider = dynamic_cast<Podcasts::PodcastProvider *>( channel->provider() );
            if( !provider )
                continue;

            channelMap.insert( provider, channel );
        }

        foreach( Podcasts::PodcastProvider *provider, channelMap.keys() )
            actions << provider->channelActions( channelMap.values( provider ) );
    }
    else if( !m_selectedEpisodes.isEmpty() )
    {
        actions << createEpisodeActions( m_selectedEpisodes );

        QMultiMap<Podcasts::PodcastProvider *,Podcasts::PodcastEpisodePtr> episodeMap;
        foreach( Podcasts::PodcastEpisodePtr episode, m_selectedEpisodes )
        {
            Podcasts::PodcastChannelPtr channel = episode->channel();
            if( !channel )
                continue;

            Podcasts::PodcastProvider *provider = dynamic_cast<Podcasts::PodcastProvider *>( channel->provider() );
            if( !provider )
                continue;

            episodeMap.insert( provider, episode );
        }

        foreach( Podcasts::PodcastProvider *provider, episodeMap.keys() )
            actions << provider->episodeActions( episodeMap.values( provider ) );

    }

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

QList< QAction * >
PlaylistBrowserNS::PodcastModel::createEpisodeActions( Podcasts::PodcastEpisodeList episodes )
{
    if( m_setNewAction == 0 )
    {
        m_setNewAction = new QAction( KIcon( "rating" ),
                        i18nc( "toggle the \"new\" status of this podcast episode",
                               "&New" ),
                                     this
                                    );
        m_setNewAction->setProperty( "popupdropper_svg_id", "new" );
        m_setNewAction->setCheckable( true );
        connect( m_setNewAction, SIGNAL( triggered( bool ) ), SLOT( slotSetNew( bool ) ) );
    }

    /* by default a list of podcast episodes can only be changed to isNew = false, except
       when all selected episodes are the same state */
    m_setNewAction->setChecked( false );
    foreach( const Podcasts::PodcastEpisodePtr episode, episodes )
    {
        if( episode->isNew() )
            m_setNewAction->setChecked( true );
    }

    return QList< QAction *>() << m_setNewAction;
}

void
PlaylistBrowserNS::PodcastModel::slotSetNew( bool newState )
{
    foreach( Podcasts::PodcastEpisodePtr episode, m_selectedEpisodes )
    {
        episode->setNew( newState );
    }
}

PlaylistProvider *
PlaylistBrowserNS::PodcastModel::getProviderByName( const QString &name )
{
    QList<PlaylistProvider *> providers =
            The::playlistManager()->providersForCategory( PlaylistManager::PodcastChannel );
    foreach( PlaylistProvider *provider, providers )
    {
        if( provider->prettyName() == name )
            return provider;
    }
    return 0;
}

int
PlaylistBrowserNS::PodcastModel::podcastItemType( const QModelIndex &index )
{
    Podcasts::PodcastMetaCommon *pmc =
            static_cast<Podcasts::PodcastMetaCommon *>( index.internalPointer() );
    if( !pmc )
        return Podcasts::NoType;

    return pmc->podcastType();
}

Podcasts::PodcastChannelPtr
PlaylistBrowserNS::PodcastModel::channelForIndex( const QModelIndex &index )
{
    if( index.isValid() )
    {
        switch( podcastItemType( index ) )
        {
            case Podcasts::EpisodeType:
                return Podcasts::PodcastChannelPtr();
            case Podcasts::ChannelType:
                return Podcasts::PodcastChannelPtr(
                        static_cast<Podcasts::PodcastChannel *>( index.internalPointer() ) );
            default:
                return Podcasts::PodcastChannelPtr();
        }
    }

    return Podcasts::PodcastChannelPtr();
}

Podcasts::PodcastEpisodePtr
PlaylistBrowserNS::PodcastModel::episodeForIndex( const QModelIndex &index )
{
    if( !index.isValid() )
        return Podcasts::PodcastEpisodePtr();

    switch( podcastItemType( index ) )
    {
        case Podcasts::EpisodeType:
            return Podcasts::PodcastEpisodePtr(
                static_cast<Podcasts::PodcastEpisode *>( index.internalPointer() ) );
        case Podcasts::ChannelType:
        default:
            return Podcasts::PodcastEpisodePtr();
    }

}

Podcasts::PodcastChannelList
PlaylistBrowserNS::PodcastModel::selectedChannels( const QModelIndexList &indices )
{
    Podcasts::PodcastChannelList channels;
    foreach( const QModelIndex &index, indices )
    {
        Podcasts::PodcastChannelPtr channel = channelForIndex( index );
        if( channel )
            channels << channel;
    }
    return channels;
}

Podcasts::PodcastEpisodeList
PlaylistBrowserNS::PodcastModel::selectedEpisodes( const QModelIndexList &indices )
{
    Podcasts::PodcastEpisodeList episodes;
    foreach( const QModelIndex &index, indices )
    {
        if( !index.isValid() )
            break;

        switch( podcastItemType( index ) )
        {
            case Podcasts::EpisodeType:
                episodes << Podcasts::PodcastEpisodePtr(
                          static_cast<Podcasts::PodcastEpisode *>( index.internalPointer() ) );
                break;
            case Podcasts::ChannelType:
                episodes <<
                        static_cast<Podcasts::PodcastChannel *>( index.internalPointer() )->episodes();
                break;
        }
    }
    return episodes;
}

void
PlaylistBrowserNS::PodcastModel::slotAppend()
{
    Podcasts::PodcastEpisodeList episodes = selectedEpisodes();
    if( !episodes.empty() )
        The::playlistController()->insertOptioned(
                podcastEpisodesToTracks( episodes ), Playlist::Append );
}

void
PlaylistBrowserNS::PodcastModel::slotLoad()
{
    Podcasts::PodcastEpisodeList episodes = selectedEpisodes();
    if( !episodes.empty() )
        The::playlistController()->insertOptioned(
                podcastEpisodesToTracks( episodes ), Playlist::Replace );
}

Meta::TrackList
PlaylistBrowserNS::PodcastModel::podcastEpisodesToTracks( Podcasts::PodcastEpisodeList episodes )
{
    Meta::TrackList tracks;
    foreach( Podcasts::PodcastEpisodePtr episode, episodes )
        tracks << Meta::TrackPtr::staticCast( episode );
    return tracks;
}

Podcasts::PodcastProvider *
PlaylistBrowserNS::PodcastModel::providerForPmc( Podcasts::PodcastMetaCommon *pmc ) const
{
    PlaylistProvider *provider = 0;
    if( pmc->podcastType() == Podcasts::ChannelType )
    {
        Podcasts::PodcastChannel *pc =
                static_cast<Podcasts::PodcastChannel *>( pmc );
        provider = pc->provider();
    }
    else if( pmc->podcastType() == Podcasts::EpisodeType )
    {
        Podcasts::PodcastEpisode *pe =
                static_cast<Podcasts::PodcastEpisode *>( pmc );
        if( pe->channel().isNull() )
            return 0;
        provider = pe->channel()->provider();
    }
    return dynamic_cast<Podcasts::PodcastProvider *>( provider );
}

#include "PodcastModel.moc"
