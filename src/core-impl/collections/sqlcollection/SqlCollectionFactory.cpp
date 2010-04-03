/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#include "SqlCollectionFactory.h"

#include "CapabilityDelegateImpl.h"
#include "DatabaseUpdater.h"
#include "DefaultSqlQueryMakerFactory.h"
#include "ScanManager.h"
#include "SqlCollection.h"
#include "SqlCollectionLocation.h"
#include "SqlQueryMaker.h"
#include "SqlRegistry.h"
#include "MountPointManager.h"

#include "dialogs/OrganizeCollectionDialog.h"
#include "MainWindow.h"

#include <KLocale>

class SqlMountPointManagerImpl : public SqlMountPointManager
{
public:
    SqlMountPointManagerImpl( MountPointManager *mpm )
        : SqlMountPointManager()
        , m_mpm( mpm )
    {
        connect( mpm, SIGNAL( deviceAdded(int) ), SIGNAL( deviceAdded(int) ) );
        connect( mpm, SIGNAL( deviceRemoved(int) ), SIGNAL( deviceRemoved(int) ) );
    }

    int getIdForUrl( const KUrl &url )
    {
        return m_mpm->getIdForUrl( url );
    }

    QString getAbsolutePath ( const int deviceId, const QString& relativePath ) const
    {
        return m_mpm->getAbsolutePath( deviceId, relativePath );
    }

    QString getRelativePath( const int deviceId, const QString& absolutePath ) const
    {
        return m_mpm->getRelativePath( deviceId, absolutePath );
    }

    IdList getMountedDeviceIds() const
    {
        return m_mpm->getMountedDeviceIds();
    }

    QStringList collectionFolders() const
    {
        return m_mpm->collectionFolders();
    }

    void setCollectionFolders( const QStringList &folders )
    {
        m_mpm->setCollectionFolders( folders );
    }

    MountPointManager *m_mpm;


};

class DelegateSqlRegistry : public SqlRegistry
{
public:
    DelegateSqlRegistry( Collections::SqlCollection *coll ) : SqlRegistry( coll ) {}
protected:
    Capabilities::AlbumCapabilityDelegate *createAlbumDelegate() const { return new Capabilities::AlbumCapabilityDelegateImpl(); }
    Capabilities::ArtistCapabilityDelegate *createArtistDelegate() const { return new Capabilities::ArtistCapabilityDelegateImpl(); }
    Capabilities::TrackCapabilityDelegate *createTrackDelegate() const { return new Capabilities::TrackCapabilityDelegateImpl(); }
};

class OrganizeCollectionDelegateImpl : public OrganizeCollectionDelegate
{
public:
    OrganizeCollectionDelegateImpl() : OrganizeCollectionDelegate(), m_dialog( 0 ) {}
    virtual ~ OrganizeCollectionDelegateImpl() { delete m_dialog; }

    virtual void setTracks( const Meta::TrackList &tracks ) { m_tracks = tracks; }
    virtual void setFolders( const QStringList &folders ) { m_folders = folders; }

    virtual void show()
    {
        m_dialog = new OrganizeCollectionDialog( m_tracks,
                    m_folders,
                    The::mainWindow(), //parent
                    "", //name is unused
                    true, //modal
                    i18n( "Organize Files" ) //caption
                );

        connect( m_dialog, SIGNAL( accepted() ), SIGNAL( accepted() ) );
        connect( m_dialog, SIGNAL( rejected() ), SIGNAL( rejected() ) );
        m_dialog->show();
    }

    virtual bool overwriteDestinations() const { return m_dialog->overwriteDestinations(); }
    virtual QMap<Meta::TrackPtr, QString> destinations() const { return m_dialog->getDestinations(); }

private:
    Meta::TrackList m_tracks;
    QStringList m_folders;
    OrganizeCollectionDialog *m_dialog;
};

class OrganizeCollectionDelegateFactoryImpl : public OrganizeCollectionDelegateFactory
{
public:
    virtual OrganizeCollectionDelegate* createDelegate() { return new OrganizeCollectionDelegateImpl(); }
};

namespace Collections {

class SqlCollectionLocationFactoryImpl : public SqlCollectionLocationFactory
{
public:
    SqlCollectionLocationFactoryImpl()
        : SqlCollectionLocationFactory()
        , m_collection( 0 ) {}

    SqlCollectionLocation *createSqlCollectionLocation() const
    {
        Q_ASSERT( m_collection );
        SqlCollectionLocation *loc = new SqlCollectionLocation( m_collection );
        loc->setOrganizeCollectionDelegateFactory( new OrganizeCollectionDelegateFactoryImpl() );
        return loc;
    }

    Collections::SqlCollection *m_collection;
};

SqlCollectionFactory::SqlCollectionFactory()
{
}

SqlCollection*
SqlCollectionFactory::createSqlCollection( const QString &id, const QString &prettyName, SqlStorage *storage ) const
{
    SqlCollection *coll = new SqlCollection( id, prettyName );
    coll->setCapabilityDelegate( new Capabilities::CollectionCapabilityDelegateImpl() );

    MountPointManager *mpm = new MountPointManager( coll, storage );

    coll->setMountPointManager( new SqlMountPointManagerImpl( mpm ) );
    DatabaseUpdater *updater = new DatabaseUpdater();
    updater->setStorage( storage );
    updater->setCollection( coll );
    coll->setUpdater( updater );
    ScanManager *scanMgr = new ScanManager( coll );
    scanMgr->setCollection( coll );
    scanMgr->setStorage( storage );
    coll->setScanManager( scanMgr );
    coll->setSqlStorage( storage );
    SqlRegistry *registry = new DelegateSqlRegistry( coll );
    registry->setStorage( storage );
    coll->setRegistry( registry );

    SqlCollectionLocationFactoryImpl *clFactory = new SqlCollectionLocationFactoryImpl();
    clFactory->m_collection = coll;
    coll->setCollectionLocationFactory( clFactory );
    coll->setQueryMakerFactory( new DefaultSqlQueryMakerFactory( coll ) );

    //everything has been set up
    coll->init();
    return coll;
}

} //namespace Collections
