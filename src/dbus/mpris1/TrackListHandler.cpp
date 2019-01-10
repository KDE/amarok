/****************************************************************************************
 * Copyright (c) 2008 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
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

#include "TrackListHandler.h"

#include "ActionClasses.h"
#include "amarokconfig.h"
#include "App.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaUtility.h"
#include "core/podcasts/PodcastProvider.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "dbus/mpris1/PlayerHandler.h"
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistController.h"
#include "playlistmanager/PlaylistManager.h"
#include "playlist/PlaylistModelStack.h"

#include "Mpris1TrackListAdaptor.h"

namespace Mpris1
{

    TrackListHandler::TrackListHandler()
        : QObject( qApp )
    {
        new Mpris1TrackListAdaptor(this);
        QDBusConnection::sessionBus().registerObject( QStringLiteral("/TrackList"), this );
        connect( The::playlist()->qaim(), &QAbstractItemModel::rowsInserted, this, &TrackListHandler::slotTrackListChange );
        connect( The::playlist()->qaim(), &QAbstractItemModel::rowsRemoved, this, &TrackListHandler::slotTrackListChange );
    }

    int TrackListHandler::AddTrack( const QString &url, bool playImmediately )
    {
        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( QUrl::fromUserInput(url) );
        if( track )
        {
            Playlist::AddOptions options = playImmediately ?
                    Playlist::OnPlayMediaAction : Playlist::OnAppendToPlaylistAction;
            The::playlistController()->insertOptioned( track, options );
            return 0;
        }
        else
            return -1;
    }

    void TrackListHandler::DelTrack( int index )
    {
        if( index < GetLength() )
            The::playlistController()->removeRow( index );
    }

    int TrackListHandler::GetCurrentTrack()
    {
        return The::playlist()->activeRow();
    }

    int TrackListHandler::GetLength()
    {
        return The::playlist()->qaim()->rowCount();
    }

    QVariantMap TrackListHandler::GetMetadata( int position )
    {
        return Meta::Field::mprisMapFromTrack( The::playlist()->trackAt( position ) );
    }

    void TrackListHandler::SetLoop( bool enable )
    {
        if( enable )
        {
            AmarokConfig::setTrackProgression( AmarokConfig::EnumTrackProgression::RepeatPlaylist );
            The::playlistActions()->playlistModeChanged();
        }
        else if( AmarokConfig::trackProgression() == AmarokConfig::EnumTrackProgression::RepeatPlaylist )
        {
             AmarokConfig::setTrackProgression( AmarokConfig::EnumTrackProgression::Normal );
            The::playlistActions()->playlistModeChanged();
        }
    }

    void TrackListHandler::SetRandom( bool enable )
    {
        if( enable )
        {
            AmarokConfig::setTrackProgression( AmarokConfig::EnumTrackProgression::RandomTrack );
            The::playlistActions()->playlistModeChanged();
        }
        else if( AmarokConfig::trackProgression() == AmarokConfig::EnumTrackProgression::RandomTrack )
        {
             AmarokConfig::setTrackProgression( AmarokConfig::EnumTrackProgression::Normal );
            The::playlistActions()->playlistModeChanged();
        }
    }

    void TrackListHandler::PlayTrack( int index )
    {
        The::playlistActions()->play( index );
    }

    void TrackListHandler::slotTrackListChange()
    {
        Q_EMIT TrackListChange( The::playlist()->qaim()->rowCount() );
    }

    void TrackListHandler::UpdateAllPodcasts()
    {
        foreach( Playlists::PlaylistProvider *provider,
                 The::playlistManager()->providersForCategory( PlaylistManager::PodcastChannel ) )
        {
            Podcasts::PodcastProvider *podcastProvider = dynamic_cast<Podcasts::PodcastProvider*>( provider );
            if( podcastProvider )
                podcastProvider->updateAll();
        }
    }

    void TrackListHandler::AddPodcast( const QString& url )
    {
        //RSS 1.0/2.0 or Atom feed URL
        Podcasts::PodcastProvider *podcastProvider = The::playlistManager()->defaultPodcasts();
        if( podcastProvider )
        {
            if( !url.isEmpty() )
                podcastProvider->addPodcast( Podcasts::PodcastProvider::toFeedUrl( url.trimmed() ) );
            else
                error() << "Invalid input string";
        }
        else
            error() << "PodcastChannel provider is null";
    }

}


