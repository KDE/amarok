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
#include "collection/Collection.h"
#include "collection/CollectionManager.h"
#include "DatabaseUpdater.h"
#include "SqlRegistry.h"
#include "collection/SqlStorage.h"

#include <QPointer>

#include <KIcon>

typedef QHash<QString, QString> TrackUrls;
typedef QHash<QString, QPair<QString, QString> > ChangedTrackUrls;

class CollectionCapabilityDelegate;
class CollectionLocation;
class SqlMountPointManager;
class SqlCollectionLocationFactory;
class SqlQueryMakerFactory;
class XesamCollectionBuilder;
class ScanManager;

class AMAROK_SQLCOLLECTION_EXPORT SqlCollection : public Amarok::Collection
{
    Q_OBJECT

    Q_PROPERTY( SqlStorage *sqlStorage
                READ sqlStorage
                SCRIPTABLE false
                DESIGNABLE false )

    public:
        SqlCollection( const QString &id, const QString &prettyName );
        virtual ~SqlCollection();

        virtual void startFullScan();
        virtual void startIncrementalScan();
        virtual void stopScan();
        virtual QueryMaker* queryMaker();

        virtual QString uidUrlProtocol() const;
        virtual QString collectionId() const;
        virtual QString prettyName() const;
        virtual KIcon icon() const { return KIcon("drive-harddisk"); }

        SqlRegistry* registry() const;
        DatabaseUpdater* dbUpdater() const;
        ScanManager* scanManager() const;
        SqlStorage* sqlStorage() const;
        SqlMountPointManager* mountPointManager() const;
        
        void removeCollection();    //testing, remove later

        virtual bool isDirInCollection( QString path );
        virtual bool isFileInCollection( const QString &url );
        virtual bool possiblyContainsTrack( const KUrl &url ) const;
        virtual Meta::TrackPtr trackForUrl( const KUrl &url );

        virtual CollectionLocation* location() const;
        virtual bool isWritable() const;
        virtual bool isOrganizable() const;

        //sqlcollection internal methods
        void sendChangedSignal();

        virtual bool hasCapabilityInterface( Meta::Capability::Type type ) const;
        virtual Meta::Capability* createCapabilityInterface( Meta::Capability::Type type );

        void setSqlStorage( SqlStorage *storage ) { m_sqlStorage = storage; }
        void setRegistry( SqlRegistry *registry ) { m_registry = registry; }
        void setUpdater( DatabaseUpdater *updater ) { m_updater = updater; }
        void setCapabilityDelegate( CollectionCapabilityDelegate *delegate ) { m_capabilityDelegate = delegate; }
        void setCollectionLocationFactory( SqlCollectionLocationFactory *factory ) { m_collectionLocationFactory = factory; }
        void setQueryMakerFactory( SqlQueryMakerFactory *factory ) { m_queryMakerFactory = factory; }
        void setScanManager( ScanManager *scanMgr );
        void setMountPointManager( SqlMountPointManager *mpm );
        //this method MUST be called before using the collection
        void init();

    public slots:
        void updateTrackUrlsUids( const ChangedTrackUrls &changedUrls, const TrackUrls & ); //they're not actually track urls
        void deleteTracksSlot( Meta::TrackList tracklist );

        void dumpDatabaseContent();

    signals:
        void scanFinished();

    private slots:
        void initXesam();
        void slotDeviceAdded( int id );
        void slotDeviceRemoved( int id );

    private:
        SqlRegistry* m_registry;
        DatabaseUpdater * m_updater;
        CollectionCapabilityDelegate * m_capabilityDelegate;
        SqlStorage * m_sqlStorage;
        SqlCollectionLocationFactory *m_collectionLocationFactory;
        SqlQueryMakerFactory *m_queryMakerFactory;
        QPointer<ScanManager> m_scanManager;
        SqlMountPointManager *m_mpm;

        QString m_collectionId;
        QString m_prettyName;

        XesamCollectionBuilder *m_xesamBuilder;
};

typedef QList<int> IdList;

class SqlMountPointManager : public QObject
{
    Q_OBJECT
public:
    virtual int getIdForUrl( const KUrl &url ) = 0;
    virtual QString getAbsolutePath ( const int deviceId, const QString& relativePath ) const = 0;
    virtual QString getRelativePath( const int deviceId, const QString& absolutePath ) const = 0;
    virtual IdList getMountedDeviceIds() const = 0;
    virtual QStringList collectionFolders() = 0;

signals:
        void deviceAdded( int id );
        void deviceRemoved( int id );
};

Q_DECLARE_METATYPE( TrackUrls )
Q_DECLARE_METATYPE( ChangedTrackUrls )

#endif /* AMAROK_COLLECTION_SQLCOLLECTION_H */

