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
#include "Mpris2AmarokAppAdaptor.h"
#include "Mpris2AmarokPlayerAdaptor.h"
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
        default:
            Q_ASSERT( false );
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
        // amarok extensions:
        new Mpris2AmarokAppAdaptor( this );
        new Mpris2AmarokPlayerAdaptor( this );
        QDBusConnection::sessionBus().registerObject( MPRIS2_OBJECT_PATH, this );

        updateTrackProgressionProperties();
        updatePlaybackStatusProperty();
        updatePlaylistProperties();
        updateTrackProperties();

        connect( The::playlistActions(), SIGNAL( navigatorChanged() ),
            SLOT( updateTrackProgressionProperties() ) );
        // changing the navigator may also affect whether there is a
        // next or previous track
        connect( The::playlistActions(), SIGNAL( navigatorChanged() ),
            SLOT( updatePlaylistProperties() ) );

        connect( The::playlist()->qaim(), SIGNAL( rowsInserted(QModelIndex,int,int) ),
            SLOT( updatePlaylistProperties() ) );
        connect( The::playlist()->qaim(), SIGNAL( rowsMoved(QModelIndex,int,int,QModelIndex,int) ),
            SLOT( updatePlaylistProperties() ) );
        connect( The::playlist()->qaim(), SIGNAL( rowsRemoved(QModelIndex,int,int) ),
            SLOT( updatePlaylistProperties() ) );
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

    void Mpris2DBusHandler::Pause()
    {
        The::engineController()->pause();
    }

    void Mpris2DBusHandler::Play()
    {
        The::engineController()->play();
    }

    void Mpris2DBusHandler::PlayPause()
    {
        The::engineController()->playPause();
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

    void Mpris2DBusHandler::StopAfterCurrent()
    {
        The::playlistActions()->setStopAfterMode( Playlist::StopAfterCurrent );
    }

    void Mpris2DBusHandler::VolumeUp( int step ) const
    {
        The::engineController()->increaseVolume( step );
    }

    void Mpris2DBusHandler::VolumeDown( int step ) const
    {
        The::engineController()->decreaseVolume( step );
    }

    void Mpris2DBusHandler::Mute() const
    {
        The::engineController()->toggleMute();
    }

    void Mpris2DBusHandler::ShowOSD() const
    {
        Amarok::OSD::instance()->forceToggleOSD();
    }

    void Mpris2DBusHandler::LoadThemeFile( const QString &path ) const
    {
        The::svgHandler()->setThemeFile( path );
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

    double Mpris2DBusHandler::Volume() const
    {
        return static_cast<double>(The::engineController()->volume()) / 100.0;
    }

    void Mpris2DBusHandler::SetVolume( double vol )
    {
        The::engineController()->setVolume( vol * 100 );
    }

    //position is specified in microseconds
    qlonglong Mpris2DBusHandler::Position() const
    {
        return The::engineController()->trackPositionMs() * 1000;
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

            if ( m_changedProperties.isEmpty() )
                QMetaObject::invokeMethod( this, "emitPropertiesChanged", Qt::QueuedConnection );

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
        QString loopStatus;
        // Trying to match with AmarokConfig::EnumTrack
        switch( AmarokConfig::trackProgression() )
        {
            case AmarokConfig::EnumTrackProgression::Normal:
            case AmarokConfig::EnumTrackProgression::RandomTrack:
            case AmarokConfig::EnumTrackProgression::RandomAlbum:
                loopStatus = "None";
                break;
            case AmarokConfig::EnumTrackProgression::RepeatTrack:
                loopStatus = "Track";
                break;
            case AmarokConfig::EnumTrackProgression::RepeatAlbum:
            case AmarokConfig::EnumTrackProgression::RepeatPlaylist:
                loopStatus = "Playlist";
                break;
            default:
                Q_ASSERT( false );
                loopStatus = "None";
                break;
        }
        setPropertyInternal( "LoopStatus", loopStatus );

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
    }

    void Mpris2DBusHandler::updatePlaylistProperties()
    {
        // FIXME: we should really be able to ask the active navigator
        // whether it can produce a next/previous track, rather than
        // depending on this enum.
        //
        // However, likelyLastTrack() and likelyNextTrack() don't do quite what we want
        if ( AmarokConfig::trackProgression() == AmarokConfig::EnumTrackProgression::RepeatPlaylist )
        {
            int rowCount = The::playlist()->qaim()->rowCount();
            setPropertyInternal( "CanGoNext", rowCount > 0 );
            setPropertyInternal( "CanGoPrevious", rowCount > 0 );
        }
        else
        {
            int activeRow = The::playlist()->activeRow();
            setPropertyInternal( "CanGoNext", activeRow < The::playlist()->qaim()->rowCount() - 1 );
            setPropertyInternal( "CanGoPrevious", activeRow > 0 );
        }
    }

    void Mpris2DBusHandler::updateTrackProperties()
    {
        Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
        if ( !currentTrack )
        {
            int rowCount = The::playlist()->qaim()->rowCount();
            setPropertyInternal( "Metadata", QVariantMap() );
            setPropertyInternal( "CanPlay", rowCount > 0 );
            setPropertyInternal( "CanPause", false );
            //setPropertyInternal( "CanSeek", false );
        }
        else
        {
            QVariantMap metaData = Meta::Field::mpris20MapFromTrack( currentTrack );
            metaData["mpris:trackid"] = activeMprisTrackId();

            setPropertyInternal( "CanPlay", true );
            // Phonon doesn't actually tell us whether the media is pausable,
            // so we always claim it is
            setPropertyInternal( "CanPause", true );
            //setPropertyInternal( "CanSeek", The::engineController()->phononMediaObject()->isSeekable() );
            setPropertyInternal( "Metadata", metaData );
        }
    }

    // <EngineObserver>
    void Mpris2DBusHandler::engineStateChanged( Phonon::State /*currentState*/, Phonon::State /*oldState*/ )
    {
        QMetaObject::invokeMethod( this, "updatePlaybackStatusProperty", Qt::QueuedConnection );
    }

    void Mpris2DBusHandler::engineTrackChanged( Meta::TrackPtr )
    {
        QMetaObject::invokeMethod( this, "updateTrackProperties", Qt::QueuedConnection );
        QMetaObject::invokeMethod( this, "updatePlaylistProperties", Qt::QueuedConnection );
    }

    void Mpris2DBusHandler::engineTrackPositionChanged( qint64 position, bool seeked )
    {
        if( seeked )
            Seeked( position * 1000 );
    }

    void Mpris2DBusHandler::engineSeekableChanged( bool seekable )
    {
        setPropertyInternal( "CanSeek", seekable );
    }
    // </EngineObserver>
}

#include "Mpris2DBusHandler.moc"
