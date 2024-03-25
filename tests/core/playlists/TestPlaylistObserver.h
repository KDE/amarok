/****************************************************************************************
 * Copyright (c) 2012 Tatjana Gornak <t.gornak@gmail.com>                               *
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

#ifndef TESTPLAYLISTOBSERVER_H
#define TESTPLAYLISTOBSERVER_H

#include "core/playlists/Playlist.h"

#include <QObject>
#include <QSharedPointer>
#include <QString>

class Observer : public QObject, public Playlists::PlaylistObserver
{
    Q_OBJECT

    public:
        void metadataChanged( const Playlists::PlaylistPtr &) override
        {
            Q_EMIT metadataChangedSignal();
        }
        void tracksLoaded( Playlists::PlaylistPtr ) override
        {
            Q_EMIT tracksLoadedSignal();
        }
        void trackAdded( const Playlists::PlaylistPtr &, const Meta::TrackPtr &, int ) override
        {
            Q_EMIT trackAddedSignal();
        }
        void trackRemoved( const Playlists::PlaylistPtr &, int ) override
        {
            Q_EMIT trackRemovedSignal();
        }

    Q_SIGNALS:
        void metadataChangedSignal();
        void tracksLoadedSignal();
        void trackAddedSignal();
        void trackRemovedSignal();
};

class TestPlaylistObserver : public QObject
{
    Q_OBJECT

    public:
        TestPlaylistObserver();

    private Q_SLOTS:
        void initTestCase();
        void cleanupTestCase();

        void init();
        void cleanup();

        void testMetadataChanged();
        void testTracksLoaded();
        void testTrackAdded();
        void testTrackRemoved();

    private:
        QString dataPath( const QString &relPath );

        Playlists::PlaylistPtr m_testPlaylist;
        Observer *m_observer;
};

#endif // TESTPLAYLISTOBSERVER_H
