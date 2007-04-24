/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#ifndef AMAROK_COLLECTION_BLOCKINGQUERY_H
#define AMAROK_COLLECTION_BLOCKINGQUERY_H

#include "amarok_export.h"
#include "meta.h"
#include "querymaker.h"

#include <QHash>

using namespace Meta;

class AMAROK_EXPORT BlockingQuery : private QObject
{
    Q_OBJECT
    public:
        BlockingQuery( QueryMaker *qm );
        virtual ~BlockingQuery();

        void startQuery();

        QStringList collectionIds();

        DataList data( const QString &id );
        TrackList tracks( const QString &id );
        AlbumList albums( const QString &id );
        ArtistList artists( const QString &id );
        GenreList genres( const QString &id );
        ComposerList composers( const QString &id );
        YearList years( const QString &id );
        QStringList customData( const QString &id );

        QHash<QString, DataList> data();
        QHash<QString, TrackList> tracks();
        QHash<QString, AlbumList> albums();
        QHash<QString, ArtistList> artists();
        QHash<QString, GenreList> genres();
        QHash<QString, ComposerList> composers();
        QHash<QString, YearList> years();
        QHash<QString, QStringList> customData();

    private slots:
        void queryDone();
        void result( const QString &collectionId, DataList data );
        void result( const QString &collectionId, TrackList tracks );
        void result( const QString &collectionId, AlbumList albums );
        void result( const QString &collectionId, ArtistList artists );
        void result( const QString &collectionId, GenreList genres );
        void result( const QString &collectionId, ComposerList composers );
        void result( const QString &collectionId, YearList years );
        void result( const QString &collectionId, QStringList list );

    private:
        class Private;
        Private * const d;
};

#endif
