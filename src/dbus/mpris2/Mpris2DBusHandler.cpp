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
        // dropped out of the playlist
        return QString( "%1/OrphanTrack" ).arg( MPRIS2_OBJECT_PATH );
}

enum Status { Playing, Paused, Stopped };

static Status getStatus()
{
    if( The::engineController()->isPlaying() )
        return Playing;
    else if( The::engineController()->isPaused() )
        return Paused;
    else
        return Stopped;
}

namespace Amarok
{
    Mpris2DBusHandler::Mpris2DBusHandler()
        : QObject( kapp )
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
        setPropertyInternal( "Volume", static_cast<double>(The::engineController()->volume()) / 100.0 );

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

        EngineController *engine = The::engineController();

        connect( engine, SIGNAL( playbackStateChanged() ),
                 this, SLOT( updatePlaybackStatusProperty() ) );

        connect( engine, SIGNAL( trackChanged( Meta::TrackPtr ) ),
                 this, SLOT( updatePlaylistProperties() ) );

        connect( engine, SIGNAL( trackChanged( Meta::TrackPtr ) ),
                 this, SLOT( updateTrackProperties() ) );

        connect( engine, SIGNAL( trackPositionChanged( qint64, bool ) ),
                 this, SLOT( trackPositionChanged( qint64, bool ) ) );
        connect( engine, SIGNAL( seekableChanged( bool ) ),
                 this, SLOT( seekableChanged( bool ) ) );
        connect( engine, SIGNAL( volumeChanged( int ) ),
                 this, SLOT( volumeChanged( int ) ) );

    }

    void Mpris2DBusHandler::setProperty( const char *name, const QVariant &value )
    {
        if( qstrcmp( name, "LoopStatus" ) == 0 )
            SetLoopStatus( value.toString() );
        else if( qstrcmp( name, "Shuffle" ) == 0 )
            SetShuffle( value.toBool() );
        else if( qstrcmp( name, "Muted" ) == 0 )
            SetMuted( value.toBool() );
        else if( qstrcmp( name, "Volume" ) == 0 )
            SetVolume( value.toDouble() );
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
        window->show();
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
        // Amarok desktop file is installed in $prefix/share/applications/kde4/
        // rather than in $prefix/share/applications. The standard way to
        // represent this dir is with a "kde4-" prefix. See:
        // http://standards.freedesktop.org/menu-spec/1.0/go01.html#term-desktop-file-id
        return "kde4-amarok";
    }

    QStringList Mpris2DBusHandler::SupportedUriSchemes() const
    {
        // FIXME: Find a way to list supported schemes
        static QStringList lst = QStringList() << "file" << "http";
        return lst;
    }

    QStringList Mpris2DBusHandler::SupportedMimeTypes() const
    {
        // FIXME: this is likely to change when
        // Phonon::BackendCapabilities::notifier()'s capabilitiesChanged signal
        // is emitted (and so a propertiesChanged D-Bus signal should be emitted)
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

    void Mpris2DBusHandler::AdjustVolume( double increaseBy )
    {
        The::engineController()->setVolume( The::engineController()->volume() + increaseBy*100 );
    }

    void Mpris2DBusHandler::SetMuted( bool muted )
    {
        The::engineController()->setMuted( muted );
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
            case AmarokConfig::EnumTrackProgression::OnlyQueue:
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
            // FIXME: activeMprisTrackId changes when the current track is put in or removed from
            //        the playlist...
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

    void Mpris2DBusHandler::trackPositionChanged( qint64 position, bool seeked )
    {
        if( seeked )
            Seeked( position * 1000 );
    }

    void Mpris2DBusHandler::seekableChanged( bool seekable )
    {
        setPropertyInternal( "CanSeek", seekable );
    }

    void Mpris2DBusHandler::volumeChanged( int percent )
    {
        setPropertyInternal( "Volume", static_cast<double>(percent) / 100.0 );
    }
}

#include "Mpris2DBusHandler.moc"
