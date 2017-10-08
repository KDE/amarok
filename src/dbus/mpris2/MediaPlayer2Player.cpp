/***********************************************************************
 * Copyright 2010  Canonical Ltd
 *   (author: Aurelien Gateau <aurelien.gateau@canonical.com>)
 * Copyright 2012  Eike Hein <hein@kde.org>
 * Copyright 2012  Alex Merry <alex.merry@kdemail.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************/

#include "MediaPlayer2Player.h"

#include "EngineController.h"
#include "amarokconfig.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaUtility.h"
#include "core/support/Debug.h"
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistController.h"
#include "playlist/PlaylistModelStack.h"

#include <QCryptographicHash>

#include <QUrl>

static QDBusObjectPath mprisTrackId(quint64 playlistTrackId)
{
    QString path;
    if( playlistTrackId > 0 ) {
        path = QString( "/org/kde/amarok/Track/%1" ).arg( playlistTrackId );
    } else {
        // dropped out of the playlist
        path = QLatin1String( "/org/kde/amarok/OrphanTrack" );
    }
    return QDBusObjectPath( path );
}

static QDBusObjectPath activeMprisTrackId()
{
    return mprisTrackId( The::playlist()->activeId() );
}

using namespace Amarok;

MediaPlayer2Player::MediaPlayer2Player(QObject* parent)
    : DBusAbstractAdaptor(parent)
    , m_lastPosition(-1)
{
    connect( The::engineController(), &EngineController::trackPositionChanged,
             this, &MediaPlayer2Player::trackPositionChanged );
    // it is important that we receive this signal *after* the playlist code
    // has dealt with it, in order to get the right value for mpris:trackid
    connect( The::engineController(), &EngineController::trackChanged,
             this, &MediaPlayer2Player::trackChanged,
             Qt::QueuedConnection );
    connect( The::engineController(), &EngineController::trackMetadataChanged,
             this, &MediaPlayer2Player::trackMetadataChanged );
    connect( The::engineController(), &EngineController::albumMetadataChanged,
             this, &MediaPlayer2Player::albumMetadataChanged );
    connect( The::engineController(), &EngineController::seekableChanged,
             this, &MediaPlayer2Player::seekableChanged );
    connect( The::engineController(), &EngineController::volumeChanged,
             this, &MediaPlayer2Player::volumeChanged );
    connect( The::engineController(), &EngineController::trackLengthChanged,
             this, &MediaPlayer2Player::trackLengthChanged );
    connect( The::engineController(), &EngineController::playbackStateChanged,
             this, &MediaPlayer2Player::playbackStateChanged );
    connect( The::playlistActions(),  &Playlist::Actions::navigatorChanged,
             this, &MediaPlayer2Player::playlistNavigatorChanged );
    connect( The::playlist()->qaim(), &QAbstractItemModel::rowsInserted,
             this, &MediaPlayer2Player::playlistRowsInserted );
    connect( The::playlist()->qaim(), &QAbstractItemModel::rowsMoved,
             this, &MediaPlayer2Player::playlistRowsMoved );
    connect( The::playlist()->qaim(), &QAbstractItemModel::rowsRemoved,
             this, &MediaPlayer2Player::playlistRowsRemoved );
    connect( The::playlist()->qaim(), &QAbstractItemModel::modelReset,
             this, &MediaPlayer2Player::playlistReplaced );
    connect( qobject_cast<Playlist::ProxyBase*>(The::playlist()->qaim()),
             &Playlist::ProxyBase::activeTrackChanged,
             this, &MediaPlayer2Player::playlistActiveTrackChanged );
}

MediaPlayer2Player::~MediaPlayer2Player()
{
}

bool MediaPlayer2Player::CanGoNext() const
{
    if ( AmarokConfig::trackProgression() == AmarokConfig::EnumTrackProgression::RepeatPlaylist )
    {
        return The::playlist()->qaim()->rowCount() > 0;
    }
    else
    {
        int activeRow = The::playlist()->activeRow();
        return activeRow < The::playlist()->qaim()->rowCount() - 1;
    }
}

void MediaPlayer2Player::Next() const
{
    The::playlistActions()->next();
}

bool MediaPlayer2Player::CanGoPrevious() const
{
    if ( AmarokConfig::trackProgression() == AmarokConfig::EnumTrackProgression::RepeatPlaylist )
    {
        return The::playlist()->qaim()->rowCount() > 0;
    }
    else
    {
        return The::playlist()->activeRow() > 0;
    }
}

void MediaPlayer2Player::Previous() const
{
    The::playlistActions()->back();
}

bool MediaPlayer2Player::CanPause() const
{
    return The::engineController()->currentTrack();
}

void MediaPlayer2Player::Pause() const
{
    The::engineController()->pause();
}

void MediaPlayer2Player::PlayPause() const
{
    The::engineController()->playPause();
}

void MediaPlayer2Player::Stop() const
{
    The::engineController()->stop();
}

bool MediaPlayer2Player::CanPlay() const
{
    return The::playlist()->qaim()->rowCount() > 0;
}

void MediaPlayer2Player::Play() const
{
    The::engineController()->play();
}

void MediaPlayer2Player::SetPosition( const QDBusObjectPath& TrackId, qlonglong position ) const
{
    QDBusObjectPath activeTrackId = activeMprisTrackId();
    if( TrackId == activeTrackId )
        The::engineController()->seekTo( position / 1000 );
    else
        debug() << "SetPosition() called with a trackId (" << TrackId.path() << ") which is not for the active track (" << activeTrackId.path() << ")";
}

void MediaPlayer2Player::OpenUri( QString Uri ) const
{
    QUrl url( Uri );
    The::playlistController()->insertOptioned( url, Playlist::OnPlayMediaAction );
}

QString MediaPlayer2Player::PlaybackStatus() const
{
    if( The::engineController()->isPlaying() )
        return QLatin1String( "Playing" );
    else if ( The::engineController()->isPaused() )
        return QLatin1String( "Paused" );
    else
        return QLatin1String( "Stopped" );
}

QString MediaPlayer2Player::LoopStatus() const
{
    switch( AmarokConfig::trackProgression() )
    {
        case AmarokConfig::EnumTrackProgression::Normal:
        case AmarokConfig::EnumTrackProgression::OnlyQueue:
        case AmarokConfig::EnumTrackProgression::RandomTrack:
        case AmarokConfig::EnumTrackProgression::RandomAlbum:
            return QLatin1String( "None" );
        case AmarokConfig::EnumTrackProgression::RepeatTrack:
            return QLatin1String( "Track" );
        case AmarokConfig::EnumTrackProgression::RepeatAlbum:
        case AmarokConfig::EnumTrackProgression::RepeatPlaylist:
            return QLatin1String( "Playlist" );
        default:
            Q_ASSERT( false );
            return QLatin1String( "None" );
    }
}

void MediaPlayer2Player::setLoopStatus( const QString& status ) const
{
    AmarokConfig::EnumTrackProgression::type progression;
    if( status == QLatin1String( "None" ) )
        progression = AmarokConfig::EnumTrackProgression::Normal;
    else if( status == QLatin1String( "Track" ) )
        progression = AmarokConfig::EnumTrackProgression::RepeatTrack;
    else if( status == QLatin1String( "Playlist" ) )
        progression = AmarokConfig::EnumTrackProgression::RepeatPlaylist;
    else
    {
        debug() << "Unknown loop status:" << status;
        return;
    }

    AmarokConfig::setTrackProgression( progression );
    The::playlistActions()->playlistModeChanged();
}

double MediaPlayer2Player::Rate() const
{
    return 1.0;
}

void MediaPlayer2Player::setRate( double rate ) const
{
    Q_UNUSED(rate)
}

bool MediaPlayer2Player::Shuffle() const
{
    switch( AmarokConfig::trackProgression() )
    {
        case AmarokConfig::EnumTrackProgression::RandomAlbum:
        case AmarokConfig::EnumTrackProgression::RandomTrack:
            return true;
        default:
            return false;
    }
}

void MediaPlayer2Player::setShuffle( bool shuffle ) const
{
    AmarokConfig::EnumTrackProgression::type progression;
    if( shuffle )
        progression = AmarokConfig::EnumTrackProgression::RandomTrack;
    else
        progression = AmarokConfig::EnumTrackProgression::Normal;

    AmarokConfig::setTrackProgression( progression );
    The::playlistActions()->playlistModeChanged();
}

QVariantMap MediaPlayer2Player::metadataForTrack( Meta::TrackPtr track ) const
{
    if ( !track )
        return QVariantMap();

    QVariantMap metaData = Meta::Field::mpris20MapFromTrack( track );
    if ( track == The::playlist()->activeTrack() )
        metaData["mpris:trackid"] = QVariant::fromValue<QDBusObjectPath>( activeMprisTrackId() );
    else {
        // we should be updated shortly
        QString path = QLatin1String( "/org/kde/amarok/PendingTrack" );
        metaData["mpris:trackid"] = QVariant::fromValue<QDBusObjectPath>( QDBusObjectPath( path ) );
    }
    return metaData;
}

QVariantMap MediaPlayer2Player::Metadata() const
{
    return metadataForTrack( The::engineController()->currentTrack() );
}

double MediaPlayer2Player::Volume() const
{
    return static_cast<double>(The::engineController()->volume()) / 100.0;
}

void MediaPlayer2Player::setVolume( double volume ) const
{
    if (volume < 0.0)
        volume = 0.0;
    if (volume > 1.0)
        volume = 1.0;
    The::engineController()->setVolume( volume * 100 );
}

qlonglong MediaPlayer2Player::Position() const
{
    return The::engineController()->trackPositionMs() * 1000;
}

double MediaPlayer2Player::MinimumRate() const
{
    return 1.0;
}

double MediaPlayer2Player::MaximumRate() const
{
    return 1.0;
}

bool MediaPlayer2Player::CanSeek() const
{
    return The::engineController()->isSeekable();
}

void MediaPlayer2Player::Seek( qlonglong Offset ) const
{
    qlonglong offsetMs = Offset / 1000;
    int position = The::engineController()->trackPositionMs() + offsetMs;
    if( position < 0 )
        Previous();
    else if( position > The::engineController()->trackLength() )
        Next();
    else
        The::engineController()->seekTo( position );
}

bool MediaPlayer2Player::CanControl() const
{
    return true;
}

void MediaPlayer2Player::trackPositionChanged( qint64 position, bool userSeek )
{
    if ( userSeek )
        emit Seeked( position * 1000 );
    m_lastPosition = position;
}

void MediaPlayer2Player::trackChanged( Meta::TrackPtr track )
{
    signalPropertyChange( "CanPause", CanPause() );
    signalPropertyChange( "Metadata", metadataForTrack( track ) );
}

void MediaPlayer2Player::trackMetadataChanged( Meta::TrackPtr track )
{
    signalPropertyChange( "Metadata", metadataForTrack( track ) );
}

void MediaPlayer2Player::albumMetadataChanged( Meta::AlbumPtr album )
{
    Q_UNUSED( album )
    signalPropertyChange( "Metadata", Metadata() );
}

void MediaPlayer2Player::seekableChanged( bool seekable )
{
    signalPropertyChange( "CanSeek", seekable );
}

void MediaPlayer2Player::volumeChanged( int percent )
{
    DEBUG_BLOCK
    signalPropertyChange( "Volume", static_cast<double>(percent) / 100.0 );
}

void MediaPlayer2Player::trackLengthChanged( qint64 milliseconds )
{
    // milliseconds < 0 is not a helpful value, and generally happens
    // when the track changes or playback is stopped; these cases are
    // dealt with better by other signal handlers
    if ( milliseconds >= 0 )
        signalPropertyChange( "Metadata", Metadata() );
}

void MediaPlayer2Player::playbackStateChanged()
{
    signalPropertyChange( "PlaybackStatus", PlaybackStatus() );
}

void MediaPlayer2Player::playlistNavigatorChanged()
{
    signalPropertyChange( "CanGoNext", CanGoNext() );
    signalPropertyChange( "CanGoPrevious", CanGoPrevious() );
    signalPropertyChange( "LoopStatus", LoopStatus() );
    signalPropertyChange( "Shuffle", Shuffle() );
}

void MediaPlayer2Player::playlistRowsInserted( QModelIndex, int, int )
{
    signalPropertyChange( "CanGoPrevious", CanGoPrevious() );
    signalPropertyChange( "CanGoPrevious", CanGoPrevious() );
}

void MediaPlayer2Player::playlistRowsMoved( QModelIndex, int, int, QModelIndex, int )
{
    signalPropertyChange( "CanGoPrevious", CanGoPrevious() );
    signalPropertyChange( "CanGoPrevious", CanGoPrevious() );
}

void MediaPlayer2Player::playlistRowsRemoved( QModelIndex, int, int )
{
    signalPropertyChange( "CanGoPrevious", CanGoPrevious() );
    signalPropertyChange( "CanGoPrevious", CanGoPrevious() );
}

void MediaPlayer2Player::playlistReplaced()
{
    signalPropertyChange( "CanGoPrevious", CanGoPrevious() );
    signalPropertyChange( "CanGoPrevious", CanGoPrevious() );
}

void MediaPlayer2Player::playlistActiveTrackChanged( quint64 )
{
    signalPropertyChange( "CanGoPrevious", CanGoPrevious() );
    signalPropertyChange( "CanGoPrevious", CanGoPrevious() );
}


