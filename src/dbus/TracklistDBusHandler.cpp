/******************************************************************************
 * Copyright (C) 2008 Ian Monroe <ian@monroe.nu>                              *
 *           (C) 2008 Peter ZHOU <peterzhoulei@gmail.com>                     *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/

#include "TracklistDBusHandler.h"

#include "amarokconfig.h"
#include "App.h"
#include "collection/CollectionManager.h"
//#include "mediabrowser.h"
#include "playlist/PlaylistModel.h"
#include "dbus/PlayerDBusHandler.h"
#include "ActionClasses.h"


#include <KUrl>
#include <KSelectAction>

#include "TracklistAdaptor.h"

namespace Amarok
{

    TracklistDBusHandler::TracklistDBusHandler()
        : QObject( kapp )
    {
        new TracklistAdaptor(this);
        QDBusConnection::sessionBus().registerObject( "/TrackList", this );
        // FIXME: currently The::playlistModel() returns null at this point
        //        so the signals don't get connected
        connect( The::playlistModel(), SIGNAL( playlistCountChanged( int ) ), this, SLOT( slotTrackListChange() ) );
        connect( The::playlistModel(), SIGNAL( rowMoved( int, int ) ), this, SLOT( slotTrackListChange() ) );
    }

    int TracklistDBusHandler::AddTrack( const QString& url, bool playImmediately )
    {
        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( url );
        if( track )
        {
            if( playImmediately )
                The::playlistModel()->insertOptioned( track, Playlist::DirectPlay );
            else
                The::playlistModel()->insertOptioned( track, Playlist::Append );
            return 0;
        }
        else
            return -1;
    }

    void TracklistDBusHandler::DelTrack( int index )
    {
        if( index < GetLength() )
            The::playlistModel()->removeRows( index, 1 );
    }

    int TracklistDBusHandler::GetCurrentTrack()
    {
        return The::playlistModel()->activeRow();
    }

    int TracklistDBusHandler::GetLength()
    {
        return The::playlistModel()->rowCount();
    }

    QVariantMap TracklistDBusHandler::GetMetadata( int position )
    {
        return The::playerDBusHandler()->GetTrackMetadata( The::playlistModel()->trackForRow( position ) );
    }

    void TracklistDBusHandler::SetLoop(bool enable)
    {
        static_cast<SelectAction*>( Amarok::actionCollection()->action( "repeat" ) )
        ->setCurrentItem( enable ? AmarokConfig::EnumRepeat::Playlist : AmarokConfig::EnumRepeat::Off );
    }

    void TracklistDBusHandler::SetRandom( bool enable )
    {
        static_cast<SelectAction*>( Amarok::actionCollection()->action( "random_mode" ) )
        ->setCurrentItem( enable ? AmarokConfig::EnumRandomMode::Tracks : AmarokConfig::EnumRandomMode::Off );
    }

    void TracklistDBusHandler::slotTrackListChange()
    {
        emit TrackListChange( The::playlistModel()->rowCount() );
    }
}

#include "TracklistDBusHandler.moc"
