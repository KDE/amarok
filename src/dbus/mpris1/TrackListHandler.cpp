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

#include "amarokconfig.h"
#include "App.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaUtility.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistController.h"
#include "playlist/PlaylistModelStack.h"
#include "dbus/mpris1/PlayerHandler.h"
#include "ActionClasses.h"

#include "Mpris1TrackListAdaptor.h"

namespace Mpris1
{

    TrackListHandler::TrackListHandler()
        : QObject( kapp )
    {
        new Mpris1TrackListAdaptor(this);
        QDBusConnection::sessionBus().registerObject( "/TrackList", this );
        connect( The::playlist()->qaim(), SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(slotTrackListChange()) );
        connect( The::playlist()->qaim(), SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(slotTrackListChange()) );
    }

    int TrackListHandler::AddTrack( const QString& url, bool playImmediately )
    {
        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( url );
        if( track )
        {
            if( playImmediately )
                The::playlistController()->insertOptioned( track, Playlist::AppendAndPlayImmediately );
            else
                The::playlistController()->insertOptioned( track, Playlist::Append );
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
        emit TrackListChange( The::playlist()->qaim()->rowCount() );
    }
}

#include "TrackListHandler.moc"

