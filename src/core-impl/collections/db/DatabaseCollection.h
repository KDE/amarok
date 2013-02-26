/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Casey Link <unnamedrambler@gmail.com>                             *
 * Copyright (c) 2010 Jeff Mitchell <mitchell@kde.org>                                  *
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
#include "core-impl/collections/support/CollectionManager.h"
#include "core-impl/collections/db/ScanResultProcessor.h"
#include "MountPointManager.h"

#include <KIcon>

#include <QMutex>

typedef QHash<QString, QString> TrackUrls;
typedef QHash<QString, QPair<QString, QString> > ChangedTrackUrls;

namespace Capabilities {
    class CollectionCapabilityDelegate;
    class AlbumCapabilityDelegate;
    class ArtistCapabilityDelegate;
    class TrackCapabilityDelegate;
}

class ScanManager;
class ScannerJob;

namespace Collections {

class CollectionLocation;
class DatabaseCollectionLocationFactory;

/** The DatabaseCollection is intended to be a base class for all database backed primary collections.
 *  Primary collection means that the basis for the collection is a file system.
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
         *  @param storage The storage this collection should work on. It will be freed by the collection.
         */
        DatabaseCollection( const QString &id, const QString &prettyName );
        virtual ~DatabaseCollection();

        virtual QString collectionId() const;
        virtual QString prettyName() const;

        virtual ScanManager *scanManager() const;
        virtual MountPointManager *mountPointManager() const;
        /** This set's the mount point manager which is actually only useful for testing.
         *  Note: The old MountPointManager is not deleted when the new one is set.
         */
        virtual void setMountPointManager( MountPointManager *mpm );

        virtual QStringList getDatabaseDirectories( QList<int> idList ) const = 0;

        virtual QStringList collectionFolders() const;
        virtual void setCollectionFolders( const QStringList &folders );

        /** Call this function to prevent the collection updated signal emitted.
         *  This function can be called in preparation of larger updates.
         */
        virtual void blockUpdatedSignal();

        /** Unblocks one blockUpdatedSignal call. */
        virtual void unblockUpdatedSignal();

        /** Returns a new ScanResultProcessor for this collection.
            caller has to free the processor. */
        virtual ScanResultProcessor *getNewScanResultProcessor() = 0;

    public slots:
        /** Emit updated if the signal is not blocked by blockUpdatedSignal */
        virtual void collectionUpdated();

        /** Dumps the complete database content.
         *  The content of all Amarok tables is dumped in a couple of files
         *  in the users homedirectory.
         */
        virtual void dumpDatabaseContent() = 0;

        virtual void slotScanStarted( ScannerJob *job ) = 0;

    protected slots:
        virtual void slotDeviceAdded( int id ) { Q_UNUSED( id ); };
        virtual void slotDeviceRemoved( int id ) { Q_UNUSED( id ); };

    protected:
        MountPointManager *m_mpm;
        ScanManager *m_scanManager;

        int m_blockUpdatedSignalCount;
        bool m_updatedSignalRequested;
        QMutex m_mutex;

        QString m_collectionId;
        QString m_prettyName;
};

}

Q_DECLARE_METATYPE( TrackUrls )
Q_DECLARE_METATYPE( ChangedTrackUrls )

#endif /* AMAROK_COLLECTION_DATABASECOLLECTION_H */
