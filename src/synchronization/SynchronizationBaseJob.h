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

#include "meta/Meta.h"
#include "meta/MetaUtility.h"

#include <QHash>
#include <QObject>
#include <QPair>
#include <QSet>
#include <QString>
#include <QTimer>

class QueryMaker;

namespace Amarok
{
    class Collection;
}

class SynchronizationBaseJob : public QObject
{
    Q_OBJECT
    Q_ENUMS( State )
    Q_ENUMS( InSet )
    public:
        enum State
        {
            NotStarted,
            ComparingArtists,
            ComparingAlbums,
            ComparingTracks,
            Syncing
        };

        enum InSet
        {
            OnlyInA,
            OnlyInB,
            InBoth
        };

        SynchronizationBaseJob();
        ~SynchronizationBaseJob();

        void setFilter( const QString &filter );


    public slots:
        virtual void synchronize();

    private slots:
        void slotResultReady( const QString &id, const Meta::TrackList &artists );
        void slotResultReady( const QString &id, const Meta::AlbumList &albums );
        void slotResultReady( const QString &id, const Meta::ArtistList &tracks );
        void slotQueryDone();
        void slotSyncTracks( const QString &id, const Meta::TrackList &tracks );
        void slotSyncQueryDone();

        void timeout();

    protected:
        void setCollectionA( Amarok::Collection *collection );
        void setCollectionB( Amarok::Collection *collection );

        virtual void doSynchronization( const Meta::TrackList &tracks, InSet syncDirection, Amarok::Collection *collA, Amarok::Collection *collB ) = 0;

    private:
        QueryMaker* createQueryMaker( Amarok::Collection *collection );

        void handleAlbumResult();
        void handleArtistResult();
        void handleTrackResult();

    private:
        QueryMaker* setupArtistQuery( Amarok::Collection *coll );
        QueryMaker* setupAlbumQuery( Amarok::Collection *coll );
        QueryMaker* setupTrackQuery( Amarok::Collection *coll );

        State m_state;
        int m_currentResultCount;
        Amarok::Collection *m_collectionA;
        Amarok::Collection *m_collectionB;

        QSet<QString> m_artistsA;
        QSet<QString> m_artistsB;
        QSet<AlbumKey> m_albumsA;
        QSet<AlbumKey> m_albumsB;
        QSet<TrackKey> m_tracksA;
        QSet<TrackKey> m_tracksB;
        QHash<TrackKey, Meta::TrackPtr> m_keyToTrackA;
        QHash<TrackKey, Meta::TrackPtr> m_keyToTrackB;
        QHash<QString, InSet> m_artistResult;
        QHash<AlbumKey, InSet> m_albumResult;
        Meta::TrackList m_trackResultOnlyInA;
        Meta::TrackList m_trackResultOnlyInB;
        QTimer m_timer;
};

#endif

