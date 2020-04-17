/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#ifndef SYNCHRONIZATIONBASEJOB_H
#define SYNCHRONIZATIONBASEJOB_H

#include "core/meta/forward_declarations.h"
#include "core/meta/support/MetaKeys.h"

#include <QHash>
#include <QObject>
#include <QPair>
#include <QSet>
#include <QString>
#include <QTimer>

namespace Collections {
    class Collection;
    class QueryMaker;
}

class SynchronizationBaseJob : public QObject
{
    Q_OBJECT
    public:
        enum State
        {
            NotStarted,
            ComparingArtists,
            ComparingAlbums,
            ComparingTracks,
            Syncing
        };
        Q_ENUM( State )

        enum InSet
        {
            OnlyInA,
            OnlyInB,
            InBoth
        };
        Q_ENUM( InSet )

        SynchronizationBaseJob();
        ~SynchronizationBaseJob() override;

        void setFilter( const QString &filter );


    public Q_SLOTS:
        virtual void synchronize();

    private Q_SLOTS:
        void slotTracksReady( const Meta::TrackList &artists );
        void slotAlbumsReady( const Meta::AlbumList &albums );
        void slotArtistsReady( const Meta::ArtistList &tracks );
        void slotQueryDone();
        void slotSyncTracks( const Meta::TrackList &tracks );
        void slotSyncQueryDone();

        void timeout();

    protected:
        void setCollectionA( Collections::Collection *collection );
        void setCollectionB( Collections::Collection *collection );

        /**
          * perform the actual synchronization in this method.
          * SynchronizationBaseJob will delete itself afterwards.
          */
        virtual void doSynchronization( const Meta::TrackList &tracks, InSet syncDirection, Collections::Collection *collA, Collections::Collection *collB ) = 0;

    private:
        Collections::QueryMaker* createQueryMaker( Collections::Collection *collection );

        void handleAlbumResult();
        void handleArtistResult();
        void handleTrackResult();

    private:
        Collections::QueryMaker* setupArtistQuery( Collections::Collection *coll );
        Collections::QueryMaker* setupAlbumQuery( Collections::Collection *coll );
        Collections::QueryMaker* setupTrackQuery( Collections::Collection *coll );

        State m_state;
        int m_currentResultCount;
        Collections::Collection *m_collectionA;
        Collections::Collection *m_collectionB;

        /** All the query makers we created and the collection they belong to. */
        QHash<Collections::QueryMaker*, Collections::Collection*> m_queryMakers;

        QSet<QString> m_artistsA;
        QSet<QString> m_artistsB;
        QSet<Meta::AlbumKey> m_albumsA;
        QSet<Meta::AlbumKey> m_albumsB;
        QSet<Meta::TrackKey> m_tracksA;
        QSet<Meta::TrackKey> m_tracksB;
        QHash<Meta::TrackKey, Meta::TrackPtr> m_keyToTrackA;
        QHash<Meta::TrackKey, Meta::TrackPtr> m_keyToTrackB;
        QHash<QString, InSet> m_artistResult;
        QHash<Meta::AlbumKey, InSet> m_albumResult;
        Meta::TrackList m_trackResultOnlyInA;
        Meta::TrackList m_trackResultOnlyInB;
        QTimer m_timer;
};

#endif

