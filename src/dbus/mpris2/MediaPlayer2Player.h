/***********************************************************************
 * Copyright 2012  Eike Hein <hein@kde.org>
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

#ifndef AMAROK_MEDIAPLAYER2PLAYER_H
#define AMAROK_MEDIAPLAYER2PLAYER_H

#include "DBusAbstractAdaptor.h"

#include "core/meta/forward_declarations.h"

#include <QDBusObjectPath>
#include <QModelIndex>
#include <QVariantMap>

namespace Amarok
{
    class MediaPlayer2Player : public DBusAbstractAdaptor
    {
        Q_OBJECT
        Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Player") // Docs: http://www.mpris.org/2.1/spec/Player_Node.html

        Q_PROPERTY( QString PlaybackStatus READ PlaybackStatus )
        Q_PROPERTY( QString LoopStatus READ LoopStatus WRITE setLoopStatus )
        Q_PROPERTY( double Rate READ Rate WRITE setRate )
        Q_PROPERTY( bool Shuffle READ Shuffle WRITE setShuffle )
        Q_PROPERTY( QVariantMap Metadata READ Metadata )
        Q_PROPERTY( double Volume READ Volume WRITE setVolume )
        Q_PROPERTY( qlonglong Position READ Position )
        Q_PROPERTY( double MinimumRate READ MinimumRate )
        Q_PROPERTY( double MaximumRate READ MaximumRate )
        Q_PROPERTY( bool CanGoNext READ CanGoNext )
        Q_PROPERTY( bool CanGoPrevious READ CanGoPrevious )
        Q_PROPERTY( bool CanPlay READ CanPlay )
        Q_PROPERTY( bool CanPause READ CanPause )
        Q_PROPERTY( bool CanSeek READ CanSeek )
        Q_PROPERTY( bool CanControl READ CanControl )

        public:
            explicit MediaPlayer2Player( QObject* parent );
            ~MediaPlayer2Player();

            QString PlaybackStatus() const;
            QString LoopStatus() const;
            void setLoopStatus( const QString& loopStatus ) const;
            double Rate() const;
            void setRate( double rate ) const;
            bool Shuffle() const;
            void setShuffle( bool shuffle ) const;
            QVariantMap Metadata() const;
            double Volume() const;
            void setVolume( double volume ) const;
            qlonglong Position() const;
            double MinimumRate() const;
            double MaximumRate() const;
            bool CanGoNext() const;
            bool CanGoPrevious() const;
            bool CanPlay() const;
            bool CanPause() const;
            bool CanSeek() const;
            bool CanControl() const;

        Q_SIGNALS:
            void Seeked( qlonglong Position ) const;

        public Q_SLOTS:
            void Next() const;
            void Previous() const;
            void Pause() const;
            void PlayPause() const;
            void Stop() const;
            void Play() const;
            void Seek( qlonglong Offset ) const;
            void SetPosition( const QDBusObjectPath& TrackId, qlonglong Position ) const;
            void OpenUri( QString Uri ) const;

        private Q_SLOTS:
            void trackPositionChanged( qint64 position, bool userSeek );
            void trackChanged( Meta::TrackPtr track );
            void trackMetadataChanged( Meta::TrackPtr track );
            void albumMetadataChanged( Meta::AlbumPtr album );
            void seekableChanged( bool seekable );
            void volumeChanged( int newVolPercent );
            void trackLengthChanged( qint64 milliseconds );
            void playbackStateChanged();
            void playlistNavigatorChanged();
            void playlistRowsInserted(const QModelIndex &, int, int );
            void playlistRowsMoved(const QModelIndex &, int, int, const QModelIndex &, int );
            void playlistRowsRemoved(const QModelIndex &, int, int );
            void playlistReplaced();
            void playlistActiveTrackChanged( quint64 );

        private:
            QVariantMap metadataForTrack( Meta::TrackPtr track ) const;

            qint64 m_lastPosition;
    };
}

#endif // AMAROK_MEDIAPLAYER2PLAYER_H
