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

#include "TracklistDBusHandler.h"

#include "amarokconfig.h"
#include "App.h"
#include "collection/CollectionManager.h"
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistController.h"
#include "playlist/PlaylistModelStack.h"
#include "dbus/PlayerDBusHandler.h"
#include "ActionClasses.h"



#include "TracklistAdaptor.h"

namespace Amarok
{

    TracklistDBusHandler::TracklistDBusHandler()
        : QObject( kapp )
    {
        new TracklistAdaptor(this);
        QDBusConnection::sessionBus().registerObject( "/TrackList", this );
        connect( The::playlist(), SIGNAL( rowsInserted( const QModelIndex&, int, int ) ), this, SLOT( slotTrackListChange() ) );
        connect( The::playlist(), SIGNAL( rowsRemoved( const QModelIndex&, int, int ) ), this, SLOT( slotTrackListChange() ) );
    }

    int TracklistDBusHandler::AddTrack( const QString& url, bool playImmediately )
    {
        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( url );
        if( track )
        {
            if( playImmediately )
                The::playlistController()->insertOptioned( track, Playlist::DirectPlay );
            else
                The::playlistController()->insertOptioned( track, Playlist::Append );
            return 0;
        }
        else
            return -1;
    }

    void TracklistDBusHandler::DelTrack( int index )
    {
        if( index < GetLength() )
            The::playlistController()->removeRow( index );
    }

    int TracklistDBusHandler::GetCurrentTrack()
    {
        return The::playlist()->activeRow();
    }

    int TracklistDBusHandler::GetLength()
    {
        return The::playlist()->rowCount();
    }

    QVariantMap TracklistDBusHandler::GetMetadata( int position )
    {
        return The::playerDBusHandler()->GetTrackMetadata( The::playlist()->trackAt( position ) );
    }

    void TracklistDBusHandler::SetLoop( bool enable )
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

    void TracklistDBusHandler::SetRandom( bool enable )
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

    void TracklistDBusHandler::slotTrackListChange()
    {
        emit TrackListChange( The::playlist()->rowCount() );
    }
}

#include "TracklistDBusHandler.moc"

