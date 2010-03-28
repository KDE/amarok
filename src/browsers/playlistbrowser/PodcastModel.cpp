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

using namespace Podcasts;

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
    : MetaPlaylistModel( PlaylistManager::PodcastChannel )
 , m_setNewAction( 0 )
{
    s_instance = this;
    m_setNewAction = new QAction( KIcon( "rating" ),
                    i18nc( "toggle the \"new\" status of this podcast episode",
                           "&New" ),
                                 this
                                );
    m_setNewAction->setProperty( "popupdropper_svg_id", "new" );
    m_setNewAction->setCheckable( true );
    connect( m_setNewAction, SIGNAL( triggered( bool ) ), SLOT( slotSetNew( bool ) ) );
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
PlaylistBrowserNS::PodcastModel::data( const QModelIndex &idx, int role ) const
{
    if( idx.isValid() )
    {
        Podcasts::PodcastMetaCommon *pmc;
        if( IS_TRACK(idx) )
        {
            pmc = dynamic_cast<Podcasts::PodcastMetaCommon *>(
                    trackFromIndex( idx ).data() );
        }
        else
        {
            pmc = dynamic_cast<Podcasts::PodcastMetaCommon *>(
                    playlistFromIndex( idx ).data() );
        }

        if( !pmc )
            return QVariant();

        switch( role )
        {
            case Qt::DisplayRole:
            case Qt::ToolTipRole:
                switch( idx.column() )
                {
                    case MetaPlaylistModel::PlaylistColumn:
                        return pmc->title();

                    case SubtitleColumn:
                        return pmc->subtitle();

                    case AuthorColumn:
                        return pmc->author();
                    switch( idx.column() )
                    {
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

                    }
                    break;

                case ShortDescriptionRole:
                    if( idx.column() == MetaPlaylistModel::PlaylistColumn )
                        return pmc->description();
                    break;

                case Qt::DecorationRole:
                {
                    if( idx.column() == MetaPlaylistModel::PlaylistColumn )
                            return icon( pmc );
                    break;
                }
            }
        }
    }

    return MetaPlaylistModel::data( idx, role );
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
            Playlists::PlaylistProvider *provider = getProviderByName( value.toString() );
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

int
PlaylistBrowserNS::PodcastModel::columnCount( const QModelIndex &parent ) const
{
    Q_UNUSED( parent )
    return ColumnCount;
}

Qt::ItemFlags
PlaylistBrowserNS::PodcastModel::flags( const QModelIndex &idx ) const
{
    if( idx.row() == -1 )
    {
        switch( idx.column() )
        {
            case ProviderColumn:
                return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
            default: break;
        }
    }

    Qt::ItemFlags channelFlags;
    if( podcastItemType( idx ) == Podcasts::ChannelType )
    {
        channelFlags = Qt::ItemIsDropEnabled;
        if( idx.column() == ProviderColumn )
            channelFlags |= Qt::ItemIsEditable;
    }

    return ( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled
                 | channelFlags );
}

QVariant
PlaylistBrowserNS::PodcastModel::headerData( int section, Qt::Orientation orientation,
                                             int role) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole )
    {
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

    foreach( const QModelIndex &idx, indexes )
    {
        switch( podcastItemType( idx ) )
        {
            case Podcasts::ChannelType:
            {
                Podcasts::PodcastChannelPtr channel = channelForIndex( idx );
                if( channel.isNull() )
                    return new QMimeData();

                channels << channel;
                break;
            }

            case Podcasts::EpisodeType:
            {
                Podcasts::PodcastEpisodePtr episode = episodeForIndex( idx );
                if( episode.isNull() )
                    return new QMimeData();

                episodes << episode;
                break;
            }

            default: return mime;
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

#if(0)
bool
PlaylistBrowserNS::PodcastModel::dropMimeData( const QMimeData *data, Qt::DropAction action,
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

#endif

void
PlaylistBrowserNS::PodcastModel::addPodcast()
{
    debug() << "adding Podcast";

    //TODO: request the user to which PodcastProvider he wants to add it in case
    // of multiple (enabled) Podcast Providers.
    Podcasts::PodcastProvider *podcastProvider = The::playlistManager()->defaultPodcasts();
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
PlaylistBrowserNS::PodcastModel::refreshPodcasts()
{
    foreach( Playlists::PlaylistProvider *provider,
             The::playlistManager()->providersForCategory( PlaylistManager::PodcastChannel ) )
    {
        PodcastProvider *podcastProvider = dynamic_cast<PodcastProvider *>( provider );
        if( podcastProvider )
            podcastProvider->updateAll();
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

QActionList
PlaylistBrowserNS::PodcastModel::actionsFor( const QModelIndex &idx ) const
{
    QActionList actions = MetaPlaylistModel::actionsFor( idx );

    /* by default a list of podcast episodes can only be changed to isNew = false, except
       when all selected episodes are the same state */
    m_setNewAction->setChecked( false );
    Podcasts::PodcastEpisodeList episodes = m_setNewAction->data().value<Podcasts::PodcastEpisodeList>();
    if( IS_TRACK(idx) )
        episodes << episodeForIndex( idx );
    else
        episodes << channelForIndex( idx )->episodes();

    foreach( const Podcasts::PodcastEpisodePtr episode, episodes )
    {
        if( episode->isNew() )
            m_setNewAction->setChecked( true );
    }
    m_setNewAction->setData( QVariant::fromValue( episodes ) );

    actions << m_setNewAction;

    return actions;
}

void
PlaylistBrowserNS::PodcastModel::slotSetNew( bool newState )
{
    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == 0 )
        return;

    Podcasts::PodcastEpisodeList episodes = action->data().value<Podcasts::PodcastEpisodeList>();

    foreach( Podcasts::PodcastEpisodePtr episode, episodes )
    {
        if( !episode.isNull() )
            episode->setNew( action->isChecked() );
    }
}

int
PlaylistBrowserNS::PodcastModel::podcastItemType( const QModelIndex &idx ) const
{
    if( !idx.isValid() )
        return NoType;

    if( IS_TRACK(idx) )
        return EpisodeType;
    else
        return ChannelType;
}

Podcasts::PodcastChannelPtr
PlaylistBrowserNS::PodcastModel::channelForIndex( const QModelIndex &idx ) const
{
    return Podcasts::PodcastChannelPtr::dynamicCast( playlistFromIndex( idx ) );
}

Podcasts::PodcastEpisodePtr
PlaylistBrowserNS::PodcastModel::episodeForIndex( const QModelIndex &idx ) const
{
    return Podcasts::PodcastEpisodePtr::dynamicCast( trackFromIndex( idx ) );
}

Meta::TrackList
PlaylistBrowserNS::PodcastModel::podcastEpisodesToTracks( Podcasts::PodcastEpisodeList episodes )
{
    Meta::TrackList tracks;
    foreach( Podcasts::PodcastEpisodePtr episode, episodes )
        tracks << Meta::TrackPtr::staticCast( episode );
    return tracks;
}

#include "PodcastModel.moc"
