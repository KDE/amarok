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
// #include "context/popupdropper/libpud/PopupDropper.h"
// #include "context/popupdropper/libpud/PopupDropperItem.h"
#include "core/podcasts/PodcastImageFetcher.h"
#include "core/podcasts/PodcastMeta.h"
#include "core/support/Debug.h"
#include "playlistmanager/PlaylistManager.h"
#include "playlistmanager/SyncedPodcast.h"
#include "PodcastCategory.h"
#include "SvgHandler.h"
#include "widgets/PrettyTreeRoles.h"

#include <QAction>
#include <QGuiApplication>
#include <QIcon>
#include <QInputDialog>
#include <QListIterator>
#include <QPainter>

#include <KIconEngine>
#include <KIconLoader>

using namespace Podcasts;

namespace The
{
    PlaylistBrowserNS::PodcastModel* podcastModel()
    {
        return PlaylistBrowserNS::PodcastModel::instance();
    }
}

PlaylistBrowserNS::PodcastModel* PlaylistBrowserNS::PodcastModel::s_instance = nullptr;
QPixmap *PlaylistBrowserNS::PodcastModel::m_shadedStar = nullptr;

PlaylistBrowserNS::PodcastModel*
PlaylistBrowserNS::PodcastModel::instance()
{
    return s_instance ? s_instance : new PodcastModel();
}

void
PlaylistBrowserNS::PodcastModel::destroy()
{
    if ( s_instance )
    {
        delete s_instance;
        s_instance = nullptr;
    }
}

PlaylistBrowserNS::PodcastModel::PodcastModel()
    : PlaylistBrowserModel( PlaylistManager::PodcastChannel )
{
    s_instance = this;
}

bool
PlaylistBrowserNS::PodcastModel::isOnDisk( PodcastEpisodePtr episode ) const
{
    bool isOnDisk = false;
    QUrl episodeFile( episode->localUrl() );

    if( !episodeFile.isEmpty() )
    {
        isOnDisk = QFileInfo( episodeFile.toLocalFile() ).exists();
        // reset localUrl because the file is not there.
        // FIXME: changing a podcast in innocent-looking getter method is convoluted
        if( !isOnDisk )
            episode->setLocalUrl( QUrl() );
    }

    return isOnDisk;
}

QVariant
PlaylistBrowserNS::PodcastModel::icon( const PodcastChannelPtr &channel ) const
{
    QStringList emblems;
    //TODO: only check visible episodes. For now those are all returned by episodes().
    for( const Podcasts::PodcastEpisodePtr &ep : channel->episodes() )
    {
        if( ep->isNew() )
        {
            emblems << QStringLiteral("rating");
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
        p.drawPixmap( x, y, QPixmap::fromImage( channel->image().scaled( size,
                Qt::KeepAspectRatio, Qt::SmoothTransformation ) ) );

        // if it's a new episode draw the overlay:
        if( !emblems.isEmpty() )
        {
            if( !m_shadedStar )
            {
                // Prepare a background-foreground pair of stars for better visibility, BR 219518
                m_shadedStar = new QPixmap( 32, 32 );
                m_shadedStar->fill(Qt::transparent);
                QPainter iconp( m_shadedStar );

                QPalette pal = QGuiApplication::palette();
                pal.setColor( QPalette::WindowText , pal.color( QPalette::Window ) );
                KIconLoader::global()->setCustomPalette(pal);
                iconp.drawPixmap( 0, 0, KIconLoader::global()->loadScaledIcon( QStringLiteral("rating"), KIconLoader::NoGroup, 1, QSize( 32, 32 ) ) );

                KIconLoader::global()->resetPalette();
                iconp.drawPixmap( 4, 4, KIconLoader::global()->loadScaledIcon( QStringLiteral("rating"), KIconLoader::NoGroup, 1, QSize( 24, 24 ) ) );
                iconp.end();
            }
            p.drawPixmap( 2, 32 - 16 - 2, 16, 16, *m_shadedStar );
        }
        p.end();

        return pixmap;
    }
    else
        return ( QIcon(new KIconEngine( "podcast-amarok", KIconLoader::global(), emblems )).pixmap( 32, 32 ) );
}

QVariant
PlaylistBrowserNS::PodcastModel::icon( const PodcastEpisodePtr &episode ) const
{
    QStringList emblems;
    if( isOnDisk( episode ) )
        emblems << QStringLiteral("go-down");

    if( episode->isNew() )
        return ( QIcon( new KIconEngine( QStringLiteral("rating"), KIconLoader::global(), emblems )).pixmap( 24, 24 ) );
    else
        return ( QIcon( new KIconEngine( QStringLiteral("podcast-amarok"), KIconLoader::global(), emblems )).pixmap( 24, 24 ));
}

QVariant
PlaylistBrowserNS::PodcastModel::data( const QModelIndex &idx, int role ) const
{
    if( !idx.isValid() )
        return PlaylistBrowserModel::data( idx, role );

    if( IS_TRACK(idx) )
        return episodeData( episodeForIndex( idx ), idx, role );
    else
        return channelData( channelForIndex( idx ), idx, role );
}

QVariant
PlaylistBrowserNS::PodcastModel::channelData( const PodcastChannelPtr &channel,
                                              const QModelIndex &idx, int role ) const
{
    if( !channel )
        return QVariant();

    switch( role )
    {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            switch( idx.column() )
            {
                case PlaylistBrowserModel::PlaylistItemColumn:
                    return channel->title();
                case SubtitleColumn:
                    return channel->subtitle();
                case AuthorColumn:
                    return channel->author();
                case KeywordsColumn:
                    return channel->keywords();
                case ImageColumn:
                {
                    QUrl imageUrl( PodcastImageFetcher::cachedImagePath( channel ) );
                    if( !QFile( imageUrl.toLocalFile() ).exists() )
                        imageUrl = channel->imageUrl();
                    return imageUrl;
                }
                case DateColumn:
                    return channel->subscribeDate();
                case IsEpisodeColumn:
                    return false;
            }
            break;
        case PrettyTreeRoles::ByLineRole:
            if( idx.column() == PlaylistBrowserModel::ProviderColumn )
            {
                Playlists::PlaylistProvider *provider = providerForIndex( idx );
                if( provider )
                    return i18ncp( "number of podcasts from one source",
                                    "One Channel", "%1 channels",
                                    provider->playlists().count() );
            }
            if( idx.column() == PlaylistBrowserModel::PlaylistItemColumn )
                return channel->description();
            break;
        case PrettyTreeRoles::HasCoverRole:
            return idx.column() == PlaylistBrowserModel::PlaylistItemColumn;
        case Qt::DecorationRole:
            if( idx.column() == PlaylistBrowserModel::PlaylistItemColumn )
                return icon( channel );
            break;
    }

    return PlaylistBrowserModel::data( idx, role );
}

QVariant
PlaylistBrowserNS::PodcastModel::episodeData( const PodcastEpisodePtr &episode,
                                              const QModelIndex &idx, int role ) const
{
    if( !episode )
        return QVariant();

    switch( role )
    {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            switch( idx.column() )
            {
                case PlaylistBrowserModel::PlaylistItemColumn:
                    return episode->title();
                case SubtitleColumn:
                    return episode->subtitle();
                case AuthorColumn:
                    return episode->author();
                case KeywordsColumn:
                    return episode->keywords();
                case FilesizeColumn:
                    return episode->filesize();
                case DateColumn:
                    return episode->pubDate();
                case IsEpisodeColumn:
                    return true;
            }
            break;
        case PrettyTreeRoles::ByLineRole:
            if( idx.column() == PlaylistBrowserModel::ProviderColumn )
            {
                Playlists::PlaylistProvider *provider = providerForIndex( idx );
                if( provider )
                    return i18ncp( "number of podcasts from one source",
                                    "One Channel", "%1 channels",
                                    provider->playlists().count() );
            }
            if( idx.column() == PlaylistBrowserModel::PlaylistItemColumn )
                return episode->description();
            break;
        case PrettyTreeRoles::HasCoverRole:
            return ( idx.column() == PlaylistBrowserModel::PlaylistItemColumn );
        case Qt::DecorationRole:
            if( idx.column() == PlaylistBrowserModel::PlaylistItemColumn )
                return icon( episode );
            break;
        case EpisodeIsNewRole:
            return episode->isNew();
    }

    return PlaylistBrowserModel::data( idx, role );
}

bool
PlaylistBrowserNS::PodcastModel::setData( const QModelIndex &idx, const QVariant &value, int role )
{
    PodcastEpisodePtr episode = episodeForIndex( idx );
    if( !episode || !value.canConvert<bool>() || role != EpisodeIsNewRole )
    {
        return PlaylistBrowserModel::setData( idx, value, role );
    }

    bool checked = value.toBool();
    episode->setNew( checked );
    if( checked )
        Q_EMIT episodeMarkedAsNew( episode );
    Q_EMIT dataChanged( idx, idx );
    return true;
}

int
PlaylistBrowserNS::PodcastModel::columnCount( const QModelIndex &parent ) const
{
    Q_UNUSED( parent )
    return ColumnCount;
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
        QString url = QInputDialog::getText( nullptr,
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
    for( Playlists::PlaylistProvider *provider :
             The::playlistManager()->providersForCategory( PlaylistManager::PodcastChannel ) )
    {
        PodcastProvider *podcastProvider = dynamic_cast<PodcastProvider *>( provider );
        if( podcastProvider )
            podcastProvider->updateAll();
    }
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
    for( Podcasts::PodcastEpisodePtr episode : episodes )
        tracks << Meta::TrackPtr::staticCast( episode );
    return tracks;
}
