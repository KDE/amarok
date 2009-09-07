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

#include "PlayerDBusHandler.h"

#include "amarokconfig.h"
#include "ActionClasses.h"
#include "App.h"
#include "Debug.h"
#include "EngineController.h"
#include "meta/Meta.h"
#include "PlayerAdaptor.h"
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistModelStack.h"
#include "Osd.h"
#include "SvgHandler.h"

// Marshall the DBusStatus data into a D-BUS argument
QDBusArgument &operator<<(QDBusArgument &argument, const DBusStatus &status)
{
    argument.beginStructure();
    argument << status.Play;
    argument << status.Random;
    argument << status.Repeat;
    argument << status.RepeatPlaylist;
    argument.endStructure();
    return argument;
}

// Retrieve the DBusStatus data from the D-BUS argument
const QDBusArgument &operator>>(const QDBusArgument &argument, DBusStatus &status)
{
    argument.beginStructure();
    argument >> status.Play;
    argument >> status.Random;
    argument >> status.Repeat;
    argument >> status.RepeatPlaylist;
    argument.endStructure();
    return argument;
}


namespace Amarok
{

    PlayerDBusHandler *PlayerDBusHandler::s_instance = 0;

    PlayerDBusHandler::PlayerDBusHandler()
        : QObject(kapp)
    {
        qDBusRegisterMetaType<DBusStatus>();

        s_instance = this;
        setObjectName("PlayerDBusHandler");

        new PlayerAdaptor( this );
        QDBusConnection::sessionBus().registerObject("/Player", this);

        //HACK
        SelectAction* repeatAction = qobject_cast<SelectAction*>( Amarok::actionCollection()->action( "repeat" ) );
        Q_ASSERT( repeatAction );
        connect( repeatAction, SIGNAL( triggered( int ) ), this, SLOT( updateStatus() ) );
    }

    DBusStatus PlayerDBusHandler::GetStatus()
    {
        DBusStatus status;
        switch( The::engineController()->state() )
        {
            case Phonon::PlayingState:
            case Phonon::BufferingState:
                status.Play = 0; //Playing
                break;
            case Phonon::PausedState:
                status.Play = 1; //Paused
                break;
            case Phonon::LoadingState:
            case Phonon::StoppedState:
            case Phonon::ErrorState:
                status.Play = 2; //Stopped
        };
        if ( AmarokConfig::randomMode() )
            status.Random = 1;
        else
            status.Random = 0;
        if ( Amarok::repeatTrack() )
            status.Repeat = 1;
        else
            status.Repeat = 0;
        if ( Amarok::repeatPlaylist() || Amarok::repeatAlbum() || AmarokConfig::randomMode() )
            status.RepeatPlaylist = 1;
        else
            status.RepeatPlaylist = 0; //the music will not end if we play random
        return status;
    }

    void PlayerDBusHandler::Pause()
    {
        The::engineController()->playPause();
    }

    void PlayerDBusHandler::Play()
    {
        The::engineController()->play();
    }

    void PlayerDBusHandler::PlayPause()
    {
        if(The::engineController()->state() == Phonon::PlayingState) {
            The::engineController()->pause();
        } else {
            The::engineController()->play();
        }
    }

    void PlayerDBusHandler::Next()
    {
        The::playlistActions()->next();
    }

    void PlayerDBusHandler::Prev()
    {
        The::playlistActions()->back();
    }

    void PlayerDBusHandler::Repeat( bool on )
    {
        debug() << (on ? "Turning repeat on" : "Turning repeat off");
        if ( on == Amarok::repeatTrack() ) {
            // Don't turn off repeatAlbum or repeatPlaylist because
            // we were asked to turn off repeatTrack
            return;
        }

        SelectAction* repeatAction = qobject_cast<SelectAction*>( Amarok::actionCollection()->action( "repeat" ) );
        Q_ASSERT(repeatAction);

        if (repeatAction)
            repeatAction->setCurrentItem( on ? int(AmarokConfig::EnumRepeat::Track)
                                             : int(AmarokConfig::EnumRepeat::Off) );
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

    void PlayerDBusHandler::VolumeUp( int step ) const
    {
        The::engineController()->increaseVolume( step );
    }

    void PlayerDBusHandler::VolumeDown( int step ) const
    {
        The::engineController()->decreaseVolume( step );
    }

    void PlayerDBusHandler::Mute() const
    {
        The::engineController()->toggleMute();
    }

    void PlayerDBusHandler::ShowOSD() const
    {
        Amarok::OSD::instance()->show();
    }

    void PlayerDBusHandler::LoadThemeFile( const QString &path ) const
    {
        The::svgHandler()->setThemeFile( path );
    }

    void PlayerDBusHandler::Forward( int time )
    {
        if ( time > 0 && The::engineController()->state() != Phonon::StoppedState )
            The::engineController()->seek( The::engineController()->trackPosition() * 1000 + time );
    }

    void PlayerDBusHandler::Backward( int time )
    {
        if ( time > 0 && The::engineController()->state() != Phonon::StoppedState )
            The::engineController()->seek( The::engineController()->trackPosition() * 1000 - time );
    }
        
    QVariantMap PlayerDBusHandler::GetMetadata()
    {
        return GetTrackMetadata( The::engineController()->currentTrack() );
    }

    int PlayerDBusHandler::GetCaps()
    {
        int caps = NONE;
        Meta::TrackPtr track = The::engineController()->currentTrack();
        caps |= CAN_HAS_TRACKLIST;
        if ( track ) caps |= CAN_PROVIDE_METADATA;
        if ( GetStatus().Play == 0 /*playing*/ ) caps |= CAN_PAUSE;
        if ( ( GetStatus().Play == 1 /*paused*/ ) || ( GetStatus().Play == 2 /*stoped*/ ) ) caps |= CAN_PLAY;
        if ( ( GetStatus().Play == 0 /*playing*/ ) || ( GetStatus().Play == 1 /*paused*/ ) ) caps |= CAN_SEEK;
        if ( ( The::playlist()->activeRow() >= 0 ) && ( The::playlist()->activeRow() <= The::playlist()->rowCount() ) )
        {
            caps |= CAN_GO_NEXT;
            caps |= CAN_GO_PREV;
        }
        return caps;
    }

    void PlayerDBusHandler::updateStatus()
    {
        DBusStatus status = GetStatus();
        emit StatusChange( status );
        emit CapsChange( GetCaps() );
    }

    QVariantMap PlayerDBusHandler::GetTrackMetadata( Meta::TrackPtr track )
    {
        QVariantMap map;
        if( track )
        {
            // MANDATORY:
            map["location"] = track->playableUrl().url();

            // INFORMATIONAL:
            map["title"] = track->prettyName();
            
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
            map["rating"] = track->rating()/2;  //out of 5, not 10.
            
            if( track->year() )
                map["year"] = track->year()->name();

            if( track->album() )
                map["arturl"] = track->album()->imageLocation().url();

            //TODO: external service meta info

            // TECHNICAL:
            map["audio-bitrate"] = track->bitrate();
            map["audio-samplerate"] = track->sampleRate();
            //amarok has no video-bitrate

            // EXTRA Amarok specific
            const QString lyrics = track->cachedLyrics();
            if( !lyrics.isEmpty() )
                map["lyrics"] = lyrics;
        }
        return map;
    }

    void PlayerDBusHandler::engineTrackChanged( Meta::TrackPtr track )
    {
        Q_UNUSED( track );
        emit TrackChange( GetMetadata() );
        updateStatus();
    }
    void PlayerDBusHandler::engineStateChanged( Phonon::State currentState, Phonon::State oldState )
    {
        Q_UNUSED( currentState );
        Q_UNUSED( oldState );
        updateStatus();
    }
} // namespace Amarok

namespace The {
    Amarok::PlayerDBusHandler* playerDBusHandler() { return Amarok::PlayerDBusHandler::s_instance; }
}

#include "PlayerDBusHandler.moc"
