/*
 *  Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>
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

#ifndef AGGREGATECOLLECTION_H
#define AGGREGATECOLLECTION_H

#include "core/collections/Collection.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core/meta/forward_declarations.h"
#include "core/meta/support/MetaKeys.h"

#include <QString>
#include <QHash>
#include <QReadWriteLock>

namespace Meta {
    class AggreagateYear;
    class AggregateTrack;
    class AggregateArtist;
    class AggregateAlbum;
    class AggregateGenre;
    class AggregateComposer;
    class AggregateLabel;
}

namespace Collections {

    class AMAROK_EXPORT AggregateCollection : public Collections::Collection
    {
        Q_OBJECT
        public:
        AggregateCollection();
        ~AggregateCollection();

        // Collections::Collection methods

        virtual QString prettyName() const;
        virtual QIcon icon() const;

        virtual bool possiblyContainsTrack( const QUrl &url ) const;
        virtual Meta::TrackPtr trackForUrl( const QUrl &url );

        virtual QueryMaker* queryMaker();

        virtual QString collectionId() const;

        // AggregateCollection methods

        void removeTrack( const Meta::TrackKey &key );
        Meta::AggregateTrack* getTrack( Meta::TrackPtr track );
        void setTrack( Meta::AggregateTrack *track );
        bool hasTrack( const Meta::TrackKey &key );

        void removeAlbum( const QString &album, const QString &albumArtist );
        Meta::AggregateAlbum* getAlbum( Meta::AlbumPtr album );
        void setAlbum( Meta::AggregateAlbum *album );
        bool hasAlbum( const QString &album, const QString &albumArtist );

        void removeArtist( const QString &artist );
        Meta::AggregateArtist* getArtist( Meta::ArtistPtr artist );
        void setArtist( Meta::AggregateArtist *artist );
        bool hasArtist( const QString &artist );

        void removeGenre( const QString &genre );
        Meta::AggregateGenre* getGenre( Meta::GenrePtr genre );
        void setGenre( Meta::AggregateGenre *genre );
        bool hasGenre( const QString &genre );

        void removeComposer( const QString &name );
        Meta::AggregateComposer* getComposer( Meta::ComposerPtr composer );
        void setComposer( Meta::AggregateComposer *composer );
        bool hasComposer( const QString &name );

        bool hasYear( const QString &name );
        void removeYear( const QString &name );
        Meta::AggreagateYear* getYear( Meta::YearPtr year );
        void setYear( Meta::AggreagateYear *year );

        bool hasLabel( const QString &name );
        void removeLabel( const QString &name );
        Meta::AggregateLabel* getLabel( Meta::LabelPtr label );
        void setLabel( Meta::AggregateLabel *label );

        public Q_SLOTS:
        void removeCollection( const QString &collectionId );
        void removeCollection( Collections::Collection *collection );
        void addCollection( Collections::Collection *collection, CollectionManager::CollectionStatus status );
        void slotUpdated();

        private Q_SLOTS:
        void emptyCache();

        private:
        QHash<QString, Collections::Collection*> m_idCollectionMap;

        QHash<QString, KSharedPtr<Meta::AggreagateYear> > m_yearMap;
        QHash<QString, KSharedPtr<Meta::AggregateGenre> > m_genreMap;
        QHash<QString, KSharedPtr<Meta::AggregateComposer> > m_composerMap;
        QHash<QString, KSharedPtr<Meta::AggregateArtist> > m_artistMap;
        QHash<Meta::AlbumKey, KSharedPtr<Meta::AggregateAlbum> > m_albumMap;
        QHash<Meta::TrackKey, KSharedPtr<Meta::AggregateTrack> > m_trackMap;
        QHash<QString, KSharedPtr<Meta::AggregateLabel> > m_labelMap;

        QReadWriteLock m_yearLock;
        QReadWriteLock m_genreLock;
        QReadWriteLock m_composerLock;
        QReadWriteLock m_artistLock;
        QReadWriteLock m_albumLock;
        QReadWriteLock m_trackLock;
        QReadWriteLock m_labelLock;
    };

} //namespace Collections

#endif
