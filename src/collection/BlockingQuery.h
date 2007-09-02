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
#include "QueryMaker.h"

#include <QHash>

class AMAROK_EXPORT BlockingQuery : private QObject
{
    Q_OBJECT
    public:
        BlockingQuery( QueryMaker *qm );
        virtual ~BlockingQuery();

        void startQuery();

        QStringList collectionIds();

        Meta::DataList data( const QString &id );
        Meta::TrackList tracks( const QString &id );
        Meta::AlbumList albums( const QString &id );
        Meta::ArtistList artists( const QString &id );
        Meta::GenreList genres( const QString &id );
        Meta::ComposerList composers( const QString &id );
        Meta::YearList years( const QString &id );
        QStringList customData( const QString &id );

        QHash<QString, Meta::DataList> data();
        QHash<QString, Meta::TrackList> tracks();
        QHash<QString, Meta::AlbumList> albums();
        QHash<QString, Meta::ArtistList> artists();
        QHash<QString, Meta::GenreList> genres();
        QHash<QString, Meta::ComposerList> composers();
        QHash<QString, Meta::YearList> years();
        QHash<QString, QStringList> customData();

    private slots:
        void queryDone();
        void result( const QString &collectionId, Meta::DataList data );
        void result( const QString &collectionId, Meta::TrackList tracks );
        void result( const QString &collectionId, Meta::AlbumList albums );
        void result( const QString &collectionId, Meta::ArtistList artists );
        void result( const QString &collectionId, Meta::GenreList genres );
        void result( const QString &collectionId, Meta::ComposerList composers );
        void result( const QString &collectionId, Meta::YearList years );
        void result( const QString &collectionId, const QStringList &list );

    private:
        class Private;
        Private * const d;
};

#endif
