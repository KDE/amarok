/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Casey Link <unnamedrambler@gmail.com>                             *
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
#include "core/capabilities/CollectionScanCapability.h"
#include "core/capabilities/CollectionImportCapability.h"
#include "core/collections/Collection.h"
#include "core-impl/collections/db/DatabaseCollection.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "DatabaseUpdater.h"
#include "SqlRegistry.h"
#include "core/collections/support/SqlStorage.h"
#include "core-impl/collections/db/ScanResultProcessor.h"

#include <KIcon>

namespace Capabilities {
    class AlbumCapabilityDelegate;
    class ArtistCapabilityDelegate;
    class TrackCapabilityDelegate;
}

class ScanManager;
class XesamCollectionBuilder;

namespace Collections {

class CollectionLocation;
class SqlCollectionLocationFactory;
class SqlQueryMakerFactory;

class AMAROK_SQLCOLLECTION_EXPORT SqlCollection : public Collections::DatabaseCollection
{
    Q_OBJECT

    Q_PROPERTY( SqlStorage *sqlStorage
                READ sqlStorage
                SCRIPTABLE false
                DESIGNABLE false )

    Q_PROPERTY( QStringList collectionFolders
                READ collectionFolders
                WRITE setCollectionFolders
                SCRIPTABLE false
                DESIGNABLE false )

    public:
        /** Creates a new SqlCollection.
         *  @param storage The storage this collection should work on. It will be freed by the collection.
         */
        SqlCollection( const QString &id, const QString &prettyName, SqlStorage *storage );
        virtual ~SqlCollection();

        //this method MUST be called before using the collection
        void init();

        virtual QueryMaker* queryMaker();

        /** Returns the protocol for the uid urls of this collection.
            The SqlCollection support "amarok-sqltrackuid" and "file" protocol.
        */
        virtual QString uidUrlProtocol() const;
        virtual QString collectionId() const;
        virtual QString prettyName() const;
        virtual KIcon icon() const { return KIcon("drive-harddisk"); }

        SqlRegistry* registry() const;
        ScanManager* scanManager() const;
        SqlStorage* sqlStorage() const;
        MountPointManager* mountPointManager() const;

        void removeCollection();    //testing, remove later

        /** Returns true if the directory is already known in the database. */
        virtual bool isDirInCollection( const QString &path );

        /** Every collection has this function. */
        virtual bool possiblyContainsTrack( const KUrl &url ) const;

        virtual Meta::TrackPtr trackForUrl( const KUrl &url );

        /** Gets an existing track (or a new one) at the given position.
            This function should only be used by the SqlScanResultProcessor. */
        virtual Meta::TrackPtr getTrack( int deviceId, const QString &rpath, int directoryId, const QString &uidUrl );
        virtual Meta::TrackPtr getTrackFromUid( const QString &uniqueid );
        virtual Meta::AlbumPtr getAlbum( const QString &album, const QString &artist );

        virtual CollectionLocation* location() const;
        virtual bool isWritable() const;
        virtual bool isOrganizable() const;
        QStringList getDatabaseDirectories( QList<int> idList ) const;

        QStringList collectionFolders() const;
        void setCollectionFolders( const QStringList &folders );

        virtual bool hasCapabilityInterface( Capabilities::Capability::Type type ) const;
        virtual Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type );

        virtual Capabilities::AlbumCapabilityDelegate *albumCapabilityDelegate() const
        { return m_albumCapabilityDelegate; }

        virtual Capabilities::ArtistCapabilityDelegate *artistCapabilityDelegate() const
        { return m_artistCapabilityDelegate; }

        virtual Capabilities::TrackCapabilityDelegate *trackCapabilityDelegate() const
        { return m_trackCapabilityDelegate; }

        /** This set's the mount point manager which is actually only useful for testing.
         *  Note: The old MountPointManager is not deleted when the new one is set.
         */
        void setMountPointManager( MountPointManager *mpm );

        /** Call this function to prevent the collection updated signal emitted.
         *  This function can be called in preparation of larger updates.
         */
        void blockUpdatedSignal();

        /** Unblocks one blockUpdatedSignal call. */
        void unblockUpdatedSignal();

        ScanResultProcessor* getNewScanResultProcessor( ScanResultProcessor::ScanType type );

    public slots:
        /** Emit updated if the signal is not blocked by blockUpdatedSignal */
        virtual void collectionUpdated();

        /** Dumps the complete database content.
         *  The content of all Amarok tables is dumped in a couple of files
         *  in the users homedirectory.
         */
        void dumpDatabaseContent();

    private slots:
        void initXesam();
        void slotDeviceAdded( int id );
        void slotDeviceRemoved( int id );

    private:
        SqlRegistry *m_registry;
        Capabilities::AlbumCapabilityDelegate *m_albumCapabilityDelegate;
        Capabilities::ArtistCapabilityDelegate *m_artistCapabilityDelegate;
        Capabilities::TrackCapabilityDelegate *m_trackCapabilityDelegate;

        SqlStorage *m_sqlStorage;
        SqlCollectionLocationFactory *m_collectionLocationFactory;
        SqlQueryMakerFactory *m_queryMakerFactory;
        ScanManager *m_scanManager;
        MountPointManager *m_mpm;

        QString m_collectionId;
        QString m_prettyName;

        XesamCollectionBuilder *m_xesamBuilder;

        int m_blockUpdatedSignalCount;
        bool m_updatedSignalRequested;
        QMutex m_mutex;
};

class SqlCollectionScanCapability : public Capabilities::CollectionScanCapability
{
    Q_OBJECT
    public:

        SqlCollectionScanCapability( ScanManager* scanManager );
        virtual ~SqlCollectionScanCapability();

        virtual void startFullScan();
        virtual void startIncrementalScan( const QString &directory = QString() );
        virtual void stopScan();

    private:
        ScanManager *m_scanManager;
};

class SqlCollectionImportCapability : public Capabilities::CollectionImportCapability
{
    Q_OBJECT
    public:

        SqlCollectionImportCapability( ScanManager* scanManager );
        virtual ~SqlCollectionImportCapability();

        virtual QObject *import( const QString &importFilePath );

    private:
        ScanManager *m_scanManager;
};


}

#endif /* AMAROK_COLLECTION_SQLCOLLECTION_H */

