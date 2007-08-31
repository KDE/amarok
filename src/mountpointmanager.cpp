/*
 *  Copyright (c) 2006-2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
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

#define DEBUG_PREFIX "MountPointManager"

#include "mountpointmanager.h"

#include "amarok.h"
#include "collectiondb.h"
#include "debug.h"
#include "pluginmanager.h"
#include "statusbar.h"

//solid stuff
#include <solid/predicate.h>
#include <solid/device.h>
#include <solid/deviceinterface.h>
#include <solid/devicenotifier.h>
#include <solid/storagevolume.h>

#include <QFile>
#include <QList>
#include <QStringList>
#include <QTimer>

typedef Medium::List MediumList;

MountPointManager::MountPointManager()
    : QObject( 0 )
{
    setObjectName( "MountPointManager" );

    if ( !Amarok::config( "Collection" ).readEntry( "DynamicCollection", true ) )
    {
        debug() << "Dynamic Collection deactivated in amarokrc, not loading plugins, not connecting signals";
        return;
    }

    connect( Solid::DeviceNotifier::instance(), SIGNAL( deviceAdded( QString ) ), SLOT( deviceAdded( QString ) ) );
    connect( Solid::DeviceNotifier::instance(), SIGNAL( deviceRemoved( QString ) ), SLOT( deviceRemoved( QString ) ) );

    init();

    CollectionDB *collDB = CollectionDB::instance();

    if ( collDB->adminValue( "Database Stats Version" ).toInt() >= 9 && /* make sure that deviceid actually exists*/
         collDB->query( "SELECT COUNT(url) FROM statistics WHERE deviceid = -2;" ).first().toInt() != 0 )
    {
        connect( this, SIGNAL( mediumConnected( int ) ), SLOT( migrateStatistics() ) );
        QTimer::singleShot( 0, this, SLOT( migrateStatistics() ) );
    }
    connect( this, SIGNAL( mediumConnected( int ) ), SLOT( updateStatisticsURLs() ) );
    updateStatisticsURLs();
}


MountPointManager::~MountPointManager()
{
    m_handlerMapMutex.lock();
    foreach( DeviceHandler *dh, m_handlerMap )
    {
        delete dh;
    }
    while( !m_mediumFactories.isEmpty() )
        delete m_mediumFactories.takeFirst();
    while( !m_remoteFactories.isEmpty() )
        delete m_remoteFactories.takeFirst();
    m_handlerMapMutex.unlock();
}

MountPointManager * MountPointManager::instance( )
{
    static MountPointManager instance;
    return &instance;
}

void
MountPointManager::init()
{
    DEBUG_BLOCK
    KService::List plugins = PluginManager::query( "[X-KDE-Amarok-plugintype] == 'device'" );
    debug() << "Received [" << QString::number( plugins.count() ) << "] device plugin offers";
    oldForeachType( KService::List, plugins )
    {
        Amarok::Plugin *plugin = PluginManager::createFromService( *it );
        if( plugin )
        {
            DeviceHandlerFactory *factory = static_cast<DeviceHandlerFactory*>( plugin );
            if ( factory->canCreateFromMedium() )
                m_mediumFactories.append( factory );
            else if (factory->canCreateFromConfig() )
                m_remoteFactories.append( factory );
            else
                //FIXME max: better error message
                debug() << "Unknown DeviceHandlerFactory";
        }
        else debug() << "Plugin could not be loaded";
    }
}

int
MountPointManager::getIdForUrl( const KUrl &url )
{
    int mountPointLength = 0;
    int id = -1;
    m_handlerMapMutex.lock();
    foreach( DeviceHandler *dh, m_handlerMap )
    {
        if ( url.path().startsWith( dh->getDevicePath() ) && mountPointLength < dh->getDevicePath().length() )
        {
            id = m_handlerMap.key( dh );
            mountPointLength = dh->getDevicePath().length();
        }
    }
    m_handlerMapMutex.unlock();
    if ( mountPointLength > 0 )
    {
        return id;
    }
    else
    {
        //default fallback if we could not identify the mount point.
        //treat -1 as mount point / in all other methods
        return -1;
    }
}

int
MountPointManager::getIdForUrl( const QString &url )
{
    return getIdForUrl( KUrl( url ) );
}

bool
MountPointManager::isMounted ( const int deviceId ) const {
    m_handlerMapMutex.lock();
    bool result = m_handlerMap.contains( deviceId );
    m_handlerMapMutex.unlock();
    return result;
}

QString
MountPointManager::getMountPointForId( const int id ) const
{
    QString mountPoint;
    if ( isMounted( id ) )
    {
        m_handlerMapMutex.lock();
        mountPoint = m_handlerMap[id]->getDevicePath();
        m_handlerMapMutex.unlock();
    }
    else
        //TODO better error handling
        mountPoint = "/";
    return mountPoint;
}

void
MountPointManager::getAbsolutePath( const int deviceId, const KUrl& relativePath, KUrl& absolutePath) const
{
    //debug() << "id is " << deviceId << ", relative path is " << relativePath.path();
    if ( deviceId == -1 )
    {
        absolutePath.setPath( "/" );
        absolutePath.addPath( relativePath.path() );
        absolutePath.cleanPath();
        //debug() << "Deviceid is -1, using relative Path as absolute Path, returning " << absolutePath.path();
        return;
    }
    m_handlerMapMutex.lock();
    if ( m_handlerMap.contains( deviceId ) )
    {
        m_handlerMap[deviceId]->getURL( absolutePath, relativePath );
        m_handlerMapMutex.unlock();
    }
    else
    {
        m_handlerMapMutex.unlock();
        QStringList lastMountPoint = CollectionDB::instance()->query(
                                                 QString( "SELECT lastmountpoint FROM devices WHERE id = %1" )
                                                 .arg( deviceId ) );
        if ( lastMountPoint.count() == 0 )
        {
            //hmm, no device with that id in the DB...serious problem
            absolutePath.setPath( "/" );
            absolutePath.addPath( relativePath.path() );
            absolutePath.cleanPath();
            warning() << "Device " << deviceId << " not in database, this should never happen! Returning " << absolutePath.path() << endl;
        }
        else
        {
            absolutePath.setPath( lastMountPoint.first() );
            absolutePath.addPath( relativePath.path() );
            absolutePath.cleanPath();
//             debug() << "Device " << deviceId << " not mounted, using last mount point and returning " << absolutePath.path();
        }
    }
}

QString
MountPointManager::getAbsolutePath( const int deviceId, const QString& relativePath ) const
{
    KUrl rpath;
    rpath.setPath( relativePath );
    KUrl url;
    getAbsolutePath( deviceId, rpath, url );
    return url.path();
}

void
MountPointManager::getRelativePath( const int deviceId, const KUrl& absolutePath, KUrl& relativePath ) const
{
    m_handlerMapMutex.lock();
    if ( deviceId != -1 && m_handlerMap.contains( deviceId ) )
    {
        //FIXME max: returns garbage if the absolute path is actually not under the device's mount point
        QString rpath = KUrl::relativePath( m_handlerMap[deviceId]->getDevicePath(), absolutePath.path() );
        m_handlerMapMutex.unlock();
        relativePath.setPath( rpath );
    }
    else
    {
        m_handlerMapMutex.unlock();
        //TODO: better error handling
        QString rpath = KUrl::relativePath( "/", absolutePath.path() );
        relativePath.setPath( rpath );
    }
}

QString
MountPointManager::getRelativePath( const int deviceId, const QString& absolutePath ) const
{
    KUrl url;
    getRelativePath( deviceId, KUrl( absolutePath ), url );
    return url.path();
}

void
MountPointManager::mediumChanged( const Medium *m )
{
    DEBUG_BLOCK
    if ( !m ) return;
    if ( m->isMounted() )
    {
        foreach( DeviceHandlerFactory *factory, m_mediumFactories )
        {
            if ( factory->canHandle ( m ) )
            {
                debug() << "found handler for " << m->id();
                DeviceHandler *handler = factory->createHandler( m );
                if( !handler )
                {
                    debug() << "Factory " << factory->type() << "could not create device handler";
                    break;
                }
                int key = handler->getDeviceID();
                m_handlerMapMutex.lock();
                if ( m_handlerMap.contains( key ) )
                {
                    debug() << "Key " << key << " already exists in handlerMap, replacing";
                    delete m_handlerMap[key];
                    m_handlerMap.remove( key );
                }
                m_handlerMap.insert( key, handler );
                m_handlerMapMutex.unlock();
                debug() << "added device " << key << " with mount point " << m->mountPoint();
                emit mediumConnected( key );
                break;  //we found the added medium and don't have to check the other device handlers
            }
        }
    }
    else
    {
        m_handlerMapMutex.lock();
        foreach( DeviceHandler *dh, m_handlerMap )
        {
            if ( dh->deviceIsMedium( m ) )
            {
                int key = m_handlerMap.key( dh );
                m_handlerMap.remove( key );
                delete dh;
                debug() << "removed device " << key;
                m_handlerMapMutex.unlock();
                emit mediumRemoved( key );
                //we found the medium which was removed, so we can abort the loop
                return;
            }
        }
        m_handlerMapMutex.unlock();
    }
}

void
MountPointManager::mediumRemoved( const Medium *m )
{
    DEBUG_BLOCK
    if ( !m )
    {
        //reinit?
    }
    else
    {
        //this works for USB devices, special cases might be required for other devices
        m_handlerMapMutex.lock();
        foreach( DeviceHandler *dh, m_handlerMap )
        {
            if( dh->deviceIsMedium( m ) )
            {
                int key = m_handlerMap.key( dh );
                m_handlerMap.remove( key );
                delete dh;
                debug() << "removed device " << key;
                m_handlerMapMutex.unlock();
                emit mediumRemoved( key );
                //we found the medium which was removed, so we can abort the loop
                return;
            }
        }
        m_handlerMapMutex.unlock();
    }
}

void
MountPointManager::mediumAdded( const Medium *m )
{
    DEBUG_BLOCK
    if ( !m ) return;
    if ( m->isMounted() )
    {
        debug() << "Device added and mounted, checking handlers";
        foreach( DeviceHandlerFactory *factory, m_mediumFactories )
        {
            if( factory->canHandle( m ) )
            {
                debug() << "found handler for " << m->id();
                DeviceHandler *handler = factory->createHandler( m );
                if( !handler )
                {
                    debug() << "Factory " << factory->type() << "could not create device handler";
                    break;
                }
                int key = handler->getDeviceID();
                m_handlerMapMutex.lock();
                if( m_handlerMap.contains( key ) )
                {
                    debug() << "Key " << key << " already exists in handlerMap, replacing";
                    delete m_handlerMap[key];
                    m_handlerMap.remove( key );
                }
                m_handlerMap.insert( key, handler );
                m_handlerMapMutex.unlock();
                debug() << "added device " << key << " with mount point " << m->mountPoint();
                emit mediumConnected( key );
                break;  //we found the added medium and don't have to check the other device handlers
            }
        }
    }
}

IdList
MountPointManager::getMountedDeviceIds() const {
    m_handlerMapMutex.lock();
    IdList list( m_handlerMap.keys() );
    m_handlerMapMutex.unlock();
    list.append( -1 );
    return list;
}

QStringList
MountPointManager::collectionFolders( )
{
    //TODO max: cache data
    QStringList result;
    KConfigGroup folders = Amarok::config( "Collection Folders" );
    IdList ids = getMountedDeviceIds();
    foreach( int id, ids )
    {
        QStringList rpaths = folders.readEntry( QString::number( id ), QStringList() );
        foreach( QString strIt, rpaths )
        {
            QString absPath;
            if ( strIt == "./" )
            {
                absPath = getMountPointForId( id );
            }
            else
            {
                absPath = getAbsolutePath( id, strIt );
            }
            if ( !result.contains( absPath ) )
                result.append( absPath );
        }
    }
    return result;
}

void
MountPointManager::setCollectionFolders( const QStringList &folders )
{
    //TODO max: cache data
    typedef QMap<int, QStringList> FolderMap;
    KConfigGroup folderConf = Amarok::config( "Collection Folders" );
    FolderMap folderMap;
    
    foreach( QString folder, folders )
    {
        int id = getIdForUrl( folder );
        QString rpath = getRelativePath( id, folder );
        if ( folderMap.contains( id ) ) {
            if ( !folderMap[id].contains( rpath ) )
                folderMap[id].append( rpath );
        }
        else
            folderMap[id] = QStringList( rpath );
    }
    //make sure that collection folders on devices which are not in foldermap are deleted
    IdList ids = getMountedDeviceIds();
    foreach( int deviceId, ids )
    {
        if( !folderMap.contains( deviceId ) )
        {
            folderConf.deleteEntry( QString::number( deviceId ) );
        }
    }
    QMapIterator<int, QStringList> i( folderMap );
    while( i.hasNext() )
    {
        i.next();
        folderConf.writeEntry( QString::number( i.key() ), i.value() );
    }
}

void
MountPointManager::migrateStatistics()
{
    QStringList urls = CollectionDB::instance()->query( "SELECT url FROM statistics WHERE deviceid = -2;" );
    foreach( QString url, urls )
    {
        if ( QFile::exists( url ) )
        {
            int deviceid = getIdForUrl( url );
            QString rpath = getRelativePath( deviceid, url );
            QString update = QString( "UPDATE statistics SET deviceid = %1, url = '%2'" )
                                      .arg( deviceid )
                                      .arg( CollectionDB::instance()->escapeString( rpath ) );
            update += QString( " WHERE url = '%1' AND deviceid = -2;" )
                               .arg( CollectionDB::instance()->escapeString( url ) );
            CollectionDB::instance()->query( update );
        }
    }
}

void
MountPointManager::updateStatisticsURLs( bool changed )
{
    if ( changed )
        QTimer::singleShot( 0, this, SLOT( startStatisticsUpdateJob() ) );
}

void
MountPointManager::startStatisticsUpdateJob()
{
    ThreadManager::instance()->queueJob( new UrlUpdateJob( this ) );
}

void
MountPointManager::checkDeviceAvailability()
{
    //code to actively scan for devices which are not supported by KDE mediamanager should go here
    //method is not actually called yet
}

void
MountPointManager::deviceAdded( const QString &udi )
{
    Solid::Predicate predicate = Solid::Predicate( Solid::DeviceInterface::StorageVolume, "udi", udi );
    QList<Solid::Device> devices = Solid::Device::listFromQuery( predicate );
    //there'll be maximum one device because we are using the udi in the predicate
    if( !devices.isEmpty() )
    {
        Solid::StorageVolume *volume = devices[0].as<Solid::StorageVolume>();
        Q_UNUSED( volume );
    }
}

void
MountPointManager::deviceRemoved( const QString &udi )
{
    Q_UNUSED( udi );
}

//UrlUpdateJob

bool UrlUpdateJob::doJob( )
{
    DEBUG_BLOCK
    updateStatistics();
    updateLabels();
    return true;
}

void UrlUpdateJob::updateStatistics( )
{
    CollectionDB *collDB = CollectionDB::instance();
    MountPointManager *mpm = MountPointManager::instance();
    QStringList urls = collDB->query( "SELECT s.deviceid,s.url "
                                      "FROM statistics AS s LEFT JOIN tags AS t ON s.deviceid = t.deviceid AND s.url = t.url "
                                      "WHERE t.url IS NULL AND s.deviceid != -2;" );
    debug() << "Trying to update " << urls.count() / 2 << " statistics rows";
    oldForeach( urls )
    {
        int deviceid = (*it).toInt();
        QString rpath = *++it;
        QString realURL = mpm->getAbsolutePath( deviceid, rpath );
        if( QFile::exists( realURL ) )
        {
            int newDeviceid = mpm->getIdForUrl( realURL );
            if( newDeviceid == deviceid )
                continue;
            QString newRpath = mpm->getRelativePath( newDeviceid, realURL );

            int statCount = collDB->query(
                            QString( "SELECT COUNT( url ) FROM statistics WHERE deviceid = %1 AND url = '%2';" )
                                        .arg( newDeviceid )
                                        .arg( collDB->escapeString( newRpath ) ) ).first().toInt();
            if( statCount )
                continue;       //statistics row with new URL/deviceid values already exists

            QString sql = QString( "UPDATE statistics SET deviceid = %1, url = '%2'" )
                                .arg( newDeviceid ).arg( collDB->escapeString( newRpath ) );
            sql += QString( " WHERE deviceid = %1 AND url = '%2';" )
                                .arg( deviceid ).arg( collDB->escapeString( rpath ) );
            collDB->query( sql );
        }
    }
}

void UrlUpdateJob::updateLabels( )
{
    CollectionDB *collDB = CollectionDB::instance();
    MountPointManager *mpm = MountPointManager::instance();
    QStringList labels = collDB->query( "SELECT l.deviceid,l.url "
                                        "FROM tags_labels AS l LEFT JOIN tags as t ON l.deviceid = t.deviceid AND l.url = t.url "
                                        "WHERE t.url IS NULL;" );
    debug() << "Trying to update " << labels.count() / 2 << " tags_labels rows";
    oldForeach( labels )
    {
        int deviceid = (*it).toInt();
        QString rpath = *++it;
        QString realUrl = mpm->getAbsolutePath( deviceid, rpath );
        if( QFile::exists( realUrl ) )
        {
            int newDeviceid = mpm->getIdForUrl( realUrl );
            if( newDeviceid == deviceid )
                continue;
            QString newRpath = mpm->getRelativePath( newDeviceid, realUrl );

            //only update rows if there is not already a row with the new deviceid/rpath and the same labelid
            QStringList labelids = collDB->query(
                                        QString( "SELECT labelid FROM tags_labels WHERE deviceid = %1 AND url = '%2';" )
                                                 .arg( QString::number( newDeviceid ), collDB->escapeString( newRpath ) ) );
            QString existingLabelids;
            if( !labelids.isEmpty() )
            {
                existingLabelids = " AND labelid NOT IN (";
                oldForeach( labelids )
                {
                    if( it != labelids.begin() )
                        existingLabelids += ',';
                    existingLabelids += *it;
                }
                existingLabelids += ')';
            }
            QString sql = QString( "UPDATE tags_labels SET deviceid = %1, url = '%2' "
                                    "WHERE deviceid = %3 AND url = '%4'%5;" )
                                    .arg( newDeviceid )
                                    .arg( collDB->escapeString( newRpath ),
                                          QString::number( deviceid ),
                                          collDB->escapeString( rpath ),
                                          existingLabelids );
            collDB->query( sql );
        }
    }
}

#include "mountpointmanager.moc"
