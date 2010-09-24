/****************************************************************************************
 * Copyright (c) 2010 Canonical Ltd                                                     *
 * Author: Aurelien Gateau <aurelien.gateau@canonical.com>                              *
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

#include "Mpris2DBusHandler.h"

#include "amarokconfig.h"
#include "App.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaUtility.h"
#include "core/support/Debug.h"
#include "EngineController.h"
#include "Mpris2PlayerAdaptor.h"
#include "Mpris2RootAdaptor.h"
#include "Osd.h"
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistController.h"
#include "playlist/PlaylistModelStack.h"
#include "SvgHandler.h"

#include <QDBusConnection>
#include <QDBusMessage>

#include <KAboutData>
#include <KCmdLineArgs>
#include <KWindowSystem>

static const char *MPRIS2_OBJECT_PATH = "/org/mpris/MediaPlayer2";
static const char *MPRIS2_PLAYER_INTERFACE = "org.mpris.MediaPlayer2.Player";
static const char *FDO_PROPERTIES_INTERFACE = "org.freedesktop.DBus.Properties";

static QString activeMprisTrackId()
{
    quint64 id = The::playlist()->activeId();
    if( id > 0 )
        return QString( "%1/Track/%2" ).arg( MPRIS2_OBJECT_PATH ).arg( id );
    else
        return QString();
}

enum Status { Playing, Paused, Stopped };

static Status getStatus( Phonon::State state )
{
    Status status;
    switch( state )
    {
        case Phonon::PlayingState:
        case Phonon::BufferingState:
            status = Playing;
            break;
        case Phonon::PausedState:
            status = Paused;
            break;
        case Phonon::LoadingState:
        case Phonon::StoppedState:
        case Phonon::ErrorState:
            status = Stopped;
            break;
    };
    return status;
}

static Status getStatus()
{
    return getStatus( The::engineController()->state() );
}

namespace Amarok
{
    Mpris2DBusHandler::Mpris2DBusHandler()
        : QObject( kapp ),
          EngineObserver( The::engineController() )
    {
        new Mpris2RootAdaptor( this );
        new Mpris2PlayerAdaptor( this );
        QDBusConnection::sessionBus().registerObject( MPRIS2_OBJECT_PATH, this );

        updateTrackProgressionProperties();
        updatePlaybackStatusProperty();
        updateTrackProperties();

        connect( The::playlistActions(), SIGNAL( navigatorChanged() ),
            SLOT( updateTrackProgressionProperties() ) );
    }

    void Mpris2DBusHandler::setProperty( const char *name, const QVariant &value )
    {
        if( qstrcmp( name, "LoopStatus" ) )
            SetLoopStatus( value.toString() );
        else if( qstrcmp( name, "Shuffle" ) )
            SetShuffle( value.toBool() );
        else
            QObject::setProperty( name, value );
    }

    // <org.mpris.MediaPlayer2>
    void Mpris2DBusHandler::Raise()
    {
        MainWindow *window = App::instance()->mainWindow();
        if( !window )
        {
            warning() << "No window!";
            return;
        }
        KWindowSystem::forceActiveWindow( window->winId() );
    }

    void Mpris2DBusHandler::Quit()
    {
        // Same as KStandardAction::Quit
        kapp->quit();
    }

    QString Mpris2DBusHandler::Identity() const
    {
        const KAboutData *about = KCmdLineArgs::aboutData();
        return about->programName();
    }

    QString Mpris2DBusHandler::DesktopEntry() const
    {
        return "amarok";
    }

    QStringList Mpris2DBusHandler::SupportedUriSchemes() const
    {
        // FIXME: Find a way to list supported schemes
        static QStringList lst = QStringList() << "file" << "http";
        return lst;
    }

    QStringList Mpris2DBusHandler::SupportedMimeTypes() const
    {
        return The::engineController()->supportedMimeTypes();
    }
    // </org.mpris.MediaPlayer2>

    // <org.mpris.MediaPlayer2.Player>
    void Mpris2DBusHandler::Pause()
    {
        The::engineController()->playPause();
    }

    void Mpris2DBusHandler::Play()
    {
        The::engineController()->play();
    }

    void Mpris2DBusHandler::PlayPause()
    {
        if( The::engineController()->state() == Phonon::PlayingState )
            The::engineController()->pause();
        else
            The::engineController()->play();
    }

    void Mpris2DBusHandler::Next()
    {
        The::playlistActions()->next();
    }

    void Mpris2DBusHandler::Previous()
    {
        The::playlistActions()->back();
    }

    void Mpris2DBusHandler::Stop()
    {
        The::engineController()->stop();
    }

    void Mpris2DBusHandler::Seek( qlonglong time )
    {
        // convert time from microseconds to milliseconds
        time /= 1000;
        int position = The::engineController()->trackPositionMs() + time;
        if( position < 0 )
            Previous();
        else if( position > The::engineController()->trackLength() )
            Next();
        else
            The::engineController()->seek( position );
    }

    void Mpris2DBusHandler::SetPosition( const QDBusObjectPath &trackId, qlonglong position )
    {
        QString activeTrackId = activeMprisTrackId();
        if( trackId.path() == activeTrackId )
            The::engineController()->seek( position / 1000 );
        else
            warning() << "SetPosition() called with a trackId (" << trackId.path() << ") which is not for the active track (" << activeTrackId << ")";
    }

    void Mpris2DBusHandler::OpenUri( const QString &uri )
    {
        KUrl url( uri );
        The::playlistController()->insertOptioned( KUrl::List() << url, Playlist::AppendAndPlayImmediately );
    }

    void Mpris2DBusHandler::SetLoopStatus( const QString &status )
    {
        AmarokConfig::EnumTrackProgression::type progression;
        if( status == "None" )
            progression = AmarokConfig::EnumTrackProgression::Normal;
        else if( status == "Track" )
            progression = AmarokConfig::EnumTrackProgression::RepeatTrack;
        else if( status == "Playlist" )
            progression = AmarokConfig::EnumTrackProgression::RepeatPlaylist;
        else
        {
            warning() << "Unknown loop status:" << status;
            return;
        }

        AmarokConfig::setTrackProgression( progression );
        The::playlistActions()->playlistModeChanged();
    }

    void Mpris2DBusHandler::SetShuffle( bool on )
    {
        AmarokConfig::EnumTrackProgression::type progression;
        if( on )
            progression = AmarokConfig::EnumTrackProgression::RandomTrack;
        else
            progression = AmarokConfig::EnumTrackProgression::Normal;

        AmarokConfig::setTrackProgression( progression );
        The::playlistActions()->playlistModeChanged();
    }

    int Mpris2DBusHandler::Volume() const
    {
        return The::engineController()->volume();
    }

    void Mpris2DBusHandler::SetVolume( int vol )
    {
        The::engineController()->setVolume( vol );
    }

    //position is specified in microseconds
    qlonglong Mpris2DBusHandler::Position() const
    {
        return The::engineController()->trackPositionMs() * 1000;
    }
    // </org.mpris.MediaPlayer2.Player>

    void Mpris2DBusHandler::schedulePropertiesChangedEmission()
    {
        // For now we don't need to use a timer to avoid multiple calls to
        // emitPropertiesChanged() because it won't do anything if all changed
        // properties have been emitted
        QMetaObject::invokeMethod( this, "emitPropertiesChanged", Qt::QueuedConnection );
    }

    void Mpris2DBusHandler::emitPropertiesChanged()
    {
        if( m_changedProperties.isEmpty() )
            return;

        QVariantMap map;
        Q_FOREACH( const QByteArray &name, m_changedProperties )
            map.insert( name, property( name.data() ) );
        m_changedProperties.clear();

        QDBusMessage msg = QDBusMessage::createSignal( MPRIS2_OBJECT_PATH, FDO_PROPERTIES_INTERFACE, "PropertiesChanged" );
        QVariantList args = QVariantList()
            << MPRIS2_PLAYER_INTERFACE
            << map
            << QStringList();
        msg.setArguments( args );
        QDBusConnection::sessionBus().send( msg );
    }

    void Mpris2DBusHandler::setPropertyInternal( const char *name, const QVariant &value )
    {
        // We use QObject::setProperty here because setPropertyInternal() is
        // called to update our property map from Amarok internal state: as
        // such we do not want to trigger any setter such as SetShuffle() or
        // SetLoopStatus(), other we may end up in an infinite recursion
        // (Amarok state change => setPropertyInternal => setProperty => Amarok
        // state change =>...)
        QVariant oldValue = property( name );
        if( oldValue.isValid() && oldValue != value )
        {
            // Updating existing property
            QObject::setProperty( name, value );
            m_changedProperties << name;
        }
        else
        {
            // New property
            QObject::setProperty( name, value );
        }
    }

    void Mpris2DBusHandler::updateTrackProgressionProperties()
    {
        QString status;
        // Trying to match with AmarokConfig::EnumTrack
        switch( AmarokConfig::trackProgression() )
        {
            case AmarokConfig::EnumTrackProgression::Normal:
            case AmarokConfig::EnumTrackProgression::RandomTrack:
            case AmarokConfig::EnumTrackProgression::RandomAlbum:
                status = "None";
                break;
            case AmarokConfig::EnumTrackProgression::RepeatTrack:
                status = "Track";
                break;
            case AmarokConfig::EnumTrackProgression::RepeatAlbum:
            case AmarokConfig::EnumTrackProgression::RepeatPlaylist:
                status = "Playlist";
                break;
        }
        setPropertyInternal( "LoopStatus", status );

        bool shuffle;
        switch( AmarokConfig::trackProgression() )
        {
            case AmarokConfig::EnumTrackProgression::RandomAlbum:
            case AmarokConfig::EnumTrackProgression::RandomTrack:
                shuffle = true;
                break;
            default:
                shuffle = false;
                break;
        }
        setPropertyInternal( "Shuffle", shuffle );

        schedulePropertiesChangedEmission();
    }

    void Mpris2DBusHandler::updatePlaybackStatusProperty()
    {
        QString status;
        switch( getStatus() )
        {
            case Playing:
                status = "Playing";
                break;
            case Paused:
                status = "Paused";
                break;
            case Stopped:
                status = "Stopped";
                break;
        }
        setPropertyInternal( "PlaybackStatus", status );

        schedulePropertiesChangedEmission();
    }

    void Mpris2DBusHandler::updateTrackProperties()
    {
        int activeRow = The::playlist()->activeRow();
        Status status = getStatus();
        QVariantMap metaData = Meta::Field::mpris20MapFromTrack( The::engineController()->currentTrack() );
        metaData["mpris:trackid"] = activeMprisTrackId();

        setPropertyInternal( "CanGoNext", activeRow < The::playlist()->qaim()->rowCount() - 1 );
        setPropertyInternal( "CanGoPrevious", activeRow > 0 );
        setPropertyInternal( "CanPlay", status != Playing );
        setPropertyInternal( "CanPause", status == Playing );
        setPropertyInternal( "CanSeek", status != Stopped );
        setPropertyInternal( "Metadata", metaData );

        schedulePropertiesChangedEmission();
    }

    // <EngineObserver>
    void Mpris2DBusHandler::engineStateChanged( Phonon::State /*currentState*/, Phonon::State /*oldState*/ )
    {
        QMetaObject::invokeMethod( this, "updatePlaybackStatusProperty", Qt::QueuedConnection );
    }

    void Mpris2DBusHandler::engineTrackChanged( Meta::TrackPtr )
    {
        QMetaObject::invokeMethod( this, "updateTrackProperties", Qt::QueuedConnection );
    }

    void Mpris2DBusHandler::engineTrackPositionChanged( qint64 position, bool userSeek )
    {
        if( userSeek )
            Seeked( position * 1000 );
    }
    // </EngineObserver>
}

#include "Mpris2DBusHandler.moc"
