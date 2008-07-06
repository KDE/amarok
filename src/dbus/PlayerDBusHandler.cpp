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

#include "PlayerDBusHandler.h"

#include "App.h"
#include "EngineController.h"
#include "PlayerAdaptor.h"

namespace Amarok
{
    PlayerDBusHandler::PlayerDBusHandler()
        : QObject(kapp)
    {
        QObject* pa = new PlayerAdaptor( this );
        setObjectName("PlayerDBusHandler");
//todo: signal: trackchange, statusChange,capChangeSlot

        connect( this, SIGNAL( CapsChange( int ) ), pa, SIGNAL( CapsChange( int ) ) );

        QDBusConnection::sessionBus().registerObject("/Player", this);
    }


    //from the first integer of http://wiki.xmms2.xmms.se/index.php/MPRIS#GetStatus
    //0 = Playing, 1 = Paused, 2 = Stopped.
    int PlayerDBusHandler::GetStatus()
    {
            switch( The::engineController()->state() )
            {
            case Phonon::PlayingState:
            case Phonon::BufferingState:
                return Playing;
            case Phonon::PausedState:
                return Paused;
            case Phonon::LoadingState:
            case Phonon::StoppedState:
                return Stopped;
            case Phonon::ErrorState:
                return -1;
            }
            return -1;
    }

    void PlayerDBusHandler::PlayPause()
    {
        The::engineController() ->playPause();
    }

    void PlayerDBusHandler::Pause()
    {
        The::engineController()->pause();
    }

    void PlayerDBusHandler::Play()
    {
        The::engineController() ->play();
    }

    //position is specified in milliseconds
    int PlayerDBusHandler::PositionGet()
    {
        return The::engineController()->trackPosition() * 1000;
    }

    void PlayerDBusHandler::PositionSet( int time )
    {
        if ( time > 0 && The::engineController()->state() != Phonon::StoppedState )
            The::engineController()->seek( time );
    }

    void PlayerDBusHandler::Stop()
    {
        The::engineController()->stop();
    }

    int PlayerDBusHandler::VolumeGet()
    {
        return The::engineController()->volume();
    }

    void PlayerDBusHandler::VolumeSet( int vol )
    {
        The::engineController()->setVolume(vol);
    }

    QVariantMap PlayerDBusHandler::GetMetadata()
    {
        QVariantMap map;
        Meta::TrackPtr track = The::engineController()->currentTrack();

        if( track ) {
            map["title"] = track->name(); 
            if( track->artist() )
                map["artist"] = track->artist()->name();
            if( track->album() )
                map["album"] = track->album()->name();
            map["tracknumber"] = track->trackNumber();
            map["time"] = track->length();
            map["mtime"] = track->length() * 1000;
            if( track->genre() )
                map["genre"] = track->genre()->name();
            map["comment"] = track->comment();
            map["rating"] = track->rating();  // TODO Are we using the same scale?
            if( track->year() )
                map["year"] = track->year()->name();
        }

        return map;
    }

    int PlayerDBusHandler::GetCaps()
    {
        int caps = NONE;
//todo: add function
        caps |= CAN_PROVIDE_METADATA;
        return caps;
    }

    void PlayerDBusHandler::capsChangeSlot()
    {
        emit CapsChange( GetCaps() );
    }

} // namespace Amarok

#include "PlayerDBusHandler.moc"
