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

#ifndef PROXYCOLLECTION_H
#define PROXYCOLLECTION_H

#include "core/collections/Collection.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaUtility.h"

#include <QString>
#include <QHash>
#include <QReadWriteLock>

namespace Meta {
    class ProxyYear;
    class ProxyTrack;
    class ProxyArtist;
    class ProxyAlbum;
    class ProxyGenre;
    class ProxyComposer;
}

namespace Collections {

    class AMAROK_EXPORT_TESTS ProxyCollection : public Collections::Collection
    {
        Q_OBJECT
        public:
        ProxyCollection();
        ~ProxyCollection();

        //Collections::Collection methods

        virtual QString prettyName() const;
        virtual KIcon icon() const;

        virtual bool possiblyContainsTrack( const KUrl &url ) const;
        virtual Meta::TrackPtr trackForUrl( const KUrl &url );

        virtual QueryMaker* queryMaker();

        virtual QString collectionId() const;

        //ProxyCollection::Collection methods

        void removeTrack( const TrackKey &key );
        Meta::ProxyTrack* getTrack( Meta::TrackPtr track );
        void setTrack( Meta::ProxyTrack *track );
        bool hasTrack( const TrackKey &key );

        void removeAlbum( const QString &album, const QString &albumArtist );
        Meta::ProxyAlbum* getAlbum( Meta::AlbumPtr album );
        void setAlbum( Meta::ProxyAlbum *album );
        bool hasAlbum( const QString &album, const QString &albumArtist );

        void removeArtist( const QString &artist );
        Meta::ProxyArtist* getArtist( Meta::ArtistPtr artist );
        void setArtist( Meta::ProxyArtist *artist );
        bool hasArtist( const QString &artist );

        void removeGenre( const QString &genre );
        Meta::ProxyGenre* getGenre( Meta::GenrePtr genre );
        void setGenre( Meta::ProxyGenre *genre );
        bool hasGenre( const QString &genre );

        void removeComposer( const QString &name );
        Meta::ProxyComposer* getComposer( Meta::ComposerPtr composer );
        void setComposer( Meta::ProxyComposer *composer );
        bool hasComposer( const QString &name );

        bool hasYear( const QString &name );
        void removeYear( const QString &name );
        Meta::ProxyYear* getYear( Meta::YearPtr year );
        void setYear( Meta::ProxyYear *year );

        public slots:
        void removeCollection( const QString &collectionId );
        void removeCollection( Collections::Collection *collection );
        void addCollection( Collections::Collection *collection, CollectionManager::CollectionStatus status );
        void slotUpdated();

        private slots:
        void emptyCache();

        private:
        QHash<QString, Collections::Collection*> m_idCollectionMap;

        QHash<QString, KSharedPtr<Meta::ProxyYear> > m_yearMap;
        QHash<QString, KSharedPtr<Meta::ProxyGenre> > m_genreMap;
        QHash<QString, KSharedPtr<Meta::ProxyComposer> > m_composerMap;
        QHash<QString, KSharedPtr<Meta::ProxyArtist> > m_artistMap;
        QHash<AlbumKey, KSharedPtr<Meta::ProxyAlbum> > m_albumMap;
        QHash<TrackKey, KSharedPtr<Meta::ProxyTrack> > m_trackMap;

        QReadWriteLock m_yearLock;
        QReadWriteLock m_genreLock;
        QReadWriteLock m_composerLock;
        QReadWriteLock m_artistLock;
        QReadWriteLock m_albumLock;
        QReadWriteLock m_trackLock;

    };

} //namespace Collections

#endif
