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

#include <core/collections/Collection.h>
#include <core-impl/collections/db/amarok_databasecollection_export.h>
#include <core-impl/collections/support/CollectionManager.h>
#include <core-impl/collections/db/ScanResultProcessor.h>
#include <core-impl/collections/db/sql/MountPointManager.h>

#include <QMutex>

#include <KIcon>

typedef QHash<QString, QString> TrackUrls;
typedef QHash<QString, QPair<QString, QString> > ChangedTrackUrls;

namespace Capabilities {
    class CollectionCapabilityDelegate;
    class AlbumCapabilityDelegate;
    class ArtistCapabilityDelegate;
    class TrackCapabilityDelegate;
}

class ScanManager;
class XesamCollectionBuilder;

namespace Collections {

class CollectionLocation;
class DatabaseCollectionLocationFactory;

class AMAROK_DATABASECOLLECTION_EXPORT DatabaseCollection : public Collections::Collection
{
    Q_OBJECT

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

        //this method MUST be called before using the collection
        virtual void init() = 0;

        virtual void startFullScan() = 0;
        virtual void startIncrementalScan( const QString &directory = QString() ) = 0;
        virtual void stopScan() = 0;
        virtual QueryMaker* queryMaker() = 0;

        virtual QString uidUrlProtocol() const = 0;
        virtual QString collectionId() const = 0;
        virtual QString prettyName() const = 0;
        virtual KIcon icon() const { return KIcon("drive-harddisk"); }

        virtual ScanManager* scanManager() const = 0;
        virtual MountPointManager* mountPointManager() const = 0;

        virtual void removeCollection() = 0;    //testing, remove later

        virtual QStringList getDatabaseDirectories( QList<int> idList ) const = 0;
        virtual QStringList collectionFolders() const = 0;
        virtual void setCollectionFolders( const QStringList &folders ) = 0;

        /** Returns true if the directory is already known in the database. */
        virtual bool isDirInCollection( const QString &path ) = 0;

        virtual int getDirectory( const QString &path, uint mtime ) = 0;

        /** Every collection has this function. */
        virtual bool possiblyContainsTrack( const KUrl &url ) const = 0;

        virtual Meta::TrackPtr trackForUrl( const KUrl &url ) = 0;

        virtual CollectionLocation* location() const = 0;
        virtual bool isWritable() const = 0;
        virtual bool isOrganizable() const = 0;

        /** Call this function to prevent the collection updated signal emitted.
         *  This function can be called in preparation of larger updates.
         */
        virtual void blockUpdatedSignal() = 0;

        /** Unblocks one blockUpdatedSignal call. */
        virtual void unblockUpdatedSignal() = 0;

        virtual ScanResultProcessor* getNewScanResultProcessor( ScanResultProcessor::ScanType type ) = 0;

    public slots:
        /** Emit updated if the signal is not blocked by blockUpdatedSignal */
        virtual void collectionUpdated() = 0;

        /** Dumps the complete database content.
         *  The content of all Amarok tables is dumped in a couple of files
         *  in the users homedirectory.
         */
        virtual void dumpDatabaseContent() = 0;

    private:
        ScanManager *m_scanManager;

        QString m_collectionId;
        QString m_prettyName;

        XesamCollectionBuilder *m_xesamBuilder;

        int m_blockUpdatedSignalCount;
        bool m_updatedSignalRequested;
        QMutex m_mutex;
};

}

Q_DECLARE_METATYPE( TrackUrls )
Q_DECLARE_METATYPE( ChangedTrackUrls )

#endif /* AMAROK_COLLECTION_DATABASECOLLECTION_H */

