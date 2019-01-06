/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Casey Link <unnamedrambler@gmail.com>                             *
 * Copyright (c) 2010 Jeff Mitchell <mitchell@kde.org>                                  *
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

#ifndef AMAROK_COLLECTION_DATABASECOLLECTION_H
#define AMAROK_COLLECTION_DATABASECOLLECTION_H

#include "core/collections/Collection.h"
#include "core/capabilities/CollectionScanCapability.h"
#include "core/capabilities/CollectionImportCapability.h"

#include <QIcon>

#include <QMutex>

typedef QHash<QString, QString> TrackUrls;
typedef QHash<QString, QPair<QString, QString> > ChangedTrackUrls;

class MountPointManager;
class GenericScanManager;

namespace Collections {

class CollectionLocation;
class DatabaseCollectionLocationFactory;

/** The DatabaseCollection is intended to be a base class for all database backed primary collections.
 *  Primary collection implies that the basis for the collection is a file system.
 *  It also means that there is a scan manager that scans the file system and
 *  a mount point manager for the dynamic collection feature (we can blend out
 *  whole parts of the collection in case a drive is just unmounted).
 */
class DatabaseCollection : public Collections::Collection
{
    Q_OBJECT

    /** This property is important. CollectionSetup is using the property to
        determine the folders covered by this collection (and also setting them) */
    Q_PROPERTY( QStringList collectionFolders
                READ collectionFolders
                WRITE setCollectionFolders
                SCRIPTABLE false
                DESIGNABLE false )

    public:
        /** Creates a new DatabaseCollection.
         */
        DatabaseCollection();
        virtual ~DatabaseCollection();

        QString collectionId() const override;
        QString prettyName() const override;
        QIcon icon() const override;

        virtual GenericScanManager *scanManager() const;
        virtual MountPointManager *mountPointManager() const;
        /** This set's the mount point manager which is actually only useful for testing.
         *  Note: The old MountPointManager is not deleted when the new one is set.
         */
        virtual void setMountPointManager( MountPointManager *mpm );

        virtual QStringList collectionFolders() const;
        virtual void setCollectionFolders( const QStringList &folders );

        /**
         * Call this function to prevent the collection updated signal emitted.
         * This function can be called in preparation of larger updates.
         */
        void blockUpdatedSignal();

        /**
         * Unblocks one blockUpdatedSignal call. If collectionUpdated() has been called,
         * when the update was blocked, update() will be emitted here.
         */
        void unblockUpdatedSignal();

        /**
         * Emit updated if the signal. If it is blocked by blockUpdatedSignal(), it will
         * be postponed until unblockUpdatedSignal() is called, but never discarded.
         */
        void collectionUpdated();

        bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override;
        Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) override;

    public Q_SLOTS:
        /** Dumps the complete database content.
         *  The content of all Amarok tables is dumped in a couple of files
         *  in the users homedirectory.
         */
        virtual void dumpDatabaseContent() = 0;

    protected Q_SLOTS:
        virtual void slotDeviceAdded( int id ) { Q_UNUSED( id ); }
        virtual void slotDeviceRemoved( int id ) { Q_UNUSED( id ); }

    protected:
        MountPointManager *m_mpm;
        GenericScanManager *m_scanManager;

        int m_blockUpdatedSignalCount;
        bool m_updatedSignalRequested;
        QMutex m_mutex;
};

class DatabaseCollectionScanCapability : public Capabilities::CollectionScanCapability
{
    Q_OBJECT
    public:

        explicit DatabaseCollectionScanCapability( DatabaseCollection* collection );
        virtual ~DatabaseCollectionScanCapability();

        void startFullScan() override;
        void startIncrementalScan( const QString &directory = QString() ) override;
        void stopScan() override;

    private:
        DatabaseCollection* m_collection;
};

class DatabaseCollectionImportCapability : public Capabilities::CollectionImportCapability
{
    Q_OBJECT
    public:

        explicit DatabaseCollectionImportCapability( DatabaseCollection* collection );
        virtual ~DatabaseCollectionImportCapability();

        void import( QIODevice *input, QObject *listener ) override;

    private:
        DatabaseCollection* m_collection;
};


}

Q_DECLARE_METATYPE( TrackUrls )
Q_DECLARE_METATYPE( ChangedTrackUrls )

#endif /* AMAROK_COLLECTION_DATABASECOLLECTION_H */
