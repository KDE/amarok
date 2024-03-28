/***************************************************************************
 *   Copyright (c) 2009 Sven Krohlas <sven@asbest-online.de>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef TESTMETAMULTITRACK_H
#define TESTMETAMULTITRACK_H

#include "core/meta/Observer.h"
#include "core/playlists/Playlist.h"

#include <QObject>

namespace Meta {
    class MultiTrack;
}

class TestMetaMultiTrack : public QObject, private Playlists::PlaylistObserver
{
Q_OBJECT

public:
    TestMetaMultiTrack();

    void tracksLoaded( Playlists::PlaylistPtr playlist ) override;

Q_SIGNALS:
    void tracksLoadedSignal( const Playlists::PlaylistPtr &playlist );

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void init();
    void cleanup();

    void testSources();
    void testSetSourceCurrentNextUrl();
    void testHasCapabilityInterface();

private:
    Playlists::PlaylistPtr m_playlist;
    Meta::MultiTrack *m_testMultiTrack;
};

/**
 * A helper class that waits until all tracks have called notifyObservers at least once
 * and then emits a signal done()
 */
class NotifyObserversWaiter : public QObject, private Meta::Observer
{
    Q_OBJECT

    public:
        NotifyObserversWaiter( const QSet<Meta::TrackPtr> &tracks, QObject *parent = nullptr );

    Q_SIGNALS:
        void done();

    private Q_SLOTS:
        void slotFilterResolved();

    private:
        using Observer::metadataChanged; // silence gcc warning
        virtual void metadataChanged( const Meta::TrackPtr &track ) override;

        QSet<Meta::TrackPtr> m_tracks;
        QMutex m_mutex;
};

#endif // TESTMETAMULTITRACK_H
