/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Casey Link <unnamedrambler@gmail.com>                             *
 * Copyright (c) 2013 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#ifndef AMAROK_COLLECTION_SQLCOLLECTION_H
#define AMAROK_COLLECTION_SQLCOLLECTION_H

#include "amarok_sqlcollection_export.h"
#include "core/capabilities/TranscodeCapability.h"
#include <core/storage/SqlStorage.h>
#include <core-impl/collections/db/DatabaseCollection.h>
#include "SqlRegistry.h"

#include <QSharedPointer>

class SqlScanResultProcessor;
class AbstractDirectoryWatcher;

namespace Collections {

class CollectionLocation;
class SqlCollectionLocationFactory;
class SqlQueryMakerFactory;

/// Configuration group name in amarokrc for preferred transcoding configuration for SqlCollection
static const QString SQL_TRANSCODING_GROUP_NAME = QStringLiteral("Collection Transcoding Preference");

class AMAROK_SQLCOLLECTION_EXPORT SqlCollection : public Collections::DatabaseCollection
{
    Q_OBJECT

    public:
        /** Creates a new SqlCollection.
         *  @param storage The storage this collection should work on. It will be freed by the collection.
         */
        explicit SqlCollection( const QSharedPointer<SqlStorage> &storage );
        ~SqlCollection() override;

        QueryMaker *queryMaker() override;

        /** Returns the protocol for the uid urls of this collection.
            The SqlCollection support "amarok-sqltrackuid" and "file" protocol.
        */
        QString uidUrlProtocol() const override;
        /**
         * Generates uidUrl out of a hash (as returned by tag reader) that can be then
         * fed to Track::setUidUrl().
         */
        QString generateUidUrl( const QString &hash );

        // Local collection cannot have a capacity since it may be spread over multiple
        // physical locations (even network components)

        SqlRegistry* registry() const;
        QSharedPointer<SqlStorage> sqlStorage() const;

        /** Every collection has this function. */
        bool possiblyContainsTrack( const QUrl &url ) const override;

        Meta::TrackPtr trackForUrl( const QUrl &url ) override;

        /** Gets an existing track (or a new one) at the given position.
            This function should only be used by the SqlScanResultProcessor. */
        virtual Meta::TrackPtr getTrack( int deviceId, const QString &rpath, int directoryId, const QString &uidUrl );
        virtual Meta::TrackPtr getTrackFromUid( const QString &uniqueid );
        virtual Meta::AlbumPtr getAlbum( const QString &album, const QString &artist );

        CollectionLocation* location() override;

        bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override;
        Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) override;

    public Q_SLOTS:
        /** Dumps the complete database content.
         *  The content of all Amarok tables is dumped in a couple of files
         *  in the users homedirectory.
         */
        void dumpDatabaseContent() override;

    private Q_SLOTS:
        void slotDeviceAdded( int id ) override;
        void slotDeviceRemoved( int id ) override;

    private:
        SqlRegistry* m_registry;
        QSharedPointer<SqlStorage> m_sqlStorage;

        SqlScanResultProcessor* m_scanProcessor;
        QWeakPointer<AbstractDirectoryWatcher> m_directoryWatcher;

        SqlCollectionLocationFactory* m_collectionLocationFactory;
        SqlQueryMakerFactory* m_queryMakerFactory;
};

class AMAROK_SQLCOLLECTION_EXPORT SqlCollectionTranscodeCapability : public Capabilities::TranscodeCapability
{
    Q_OBJECT
    public:

        ~SqlCollectionTranscodeCapability() override;

        Transcoding::Configuration savedConfiguration() override;
        void setSavedConfiguration( const Transcoding::Configuration &configuration ) override;
};

}

#endif /* AMAROK_COLLECTION_SQLCOLLECTION_H */
