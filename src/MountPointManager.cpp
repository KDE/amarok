/****************************************************************************************
 * Copyright (c) 2006-2007 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
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

#define DEBUG_PREFIX "MountPointManager"

#include "MountPointManager.h"

#include "Amarok.h"
#include "CollectionManager.h"
#include "Debug.h"
#include "PluginManager.h"
#include "statusbar/StatusBar.h"
#include "SqlStorage.h"

//solid stuff
#include <solid/predicate.h>
#include <solid/device.h>
#include <solid/deviceinterface.h>
#include <solid/devicenotifier.h>
#include <solid/storagevolume.h>

#include <threadweaver/Job.h>
#include <threadweaver/ThreadWeaver.h>

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QList>
#include <QStringList>
#include <QTimer>


MountPointManager* MountPointManager::s_instance = 0;

MountPointManager*
MountPointManager::instance()
{
    if( !s_instance )
        s_instance = new MountPointManager();
    return s_instance;
}

void
MountPointManager::destroy()
{
    delete s_instance;
    s_instance = 0;
}


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

//     SqlStorage *collDB = CollectionManager::instance()->sqlStorage();

    //FIXME: Port 2.0
//     if ( collDB->adminValue( "Database Stats Version" ).toInt() >= 9 && /* make sure that deviceid actually exists*/
//          collDB->query( "SELECT COUNT(url) FROM statistics WHERE deviceid = -2;" ).first().toInt() != 0 )
//     {
//         connect( this, SIGNAL( mediumConnected( int ) ), SLOT( migrateStatistics() ) );
//         QTimer::singleShot( 0, this, SLOT( migrateStatistics() ) );
//     }
    updateStatisticsURLs();
}


MountPointManager::~MountPointManager()
{
    DEBUG_BLOCK

    m_handlerMapMutex.lock();
    foreach( DeviceHandler *dh, m_handlerMap )
        delete dh;
    
    while( !m_mediumFactories.isEmpty() )
        delete m_mediumFactories.takeFirst();
    while( !m_remoteFactories.isEmpty() )
        delete m_remoteFactories.takeFirst();
    m_handlerMapMutex.unlock();
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
        else
            debug() << "Plugin could not be loaded";

        Solid::Predicate predicate = Solid::Predicate( Solid::DeviceInterface::StorageVolume );
        QList<Solid::Device> devices = Solid::Device::listFromQuery( predicate );
        foreach( const Solid::Device &device, devices )
            createHandlerFromDevice( device, device.udi() );
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

bool
MountPointManager::isMounted( const int deviceId ) const
{
    m_handlerMapMutex.lock();
    const bool result = m_handlerMap.contains( deviceId );
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
        mountPoint = '/';
    return mountPoint;
}

void
MountPointManager::getAbsolutePath( const int deviceId, const KUrl& relativePath, KUrl& absolutePath) const
{
    //debug() << "id is " << deviceId << ", relative path is " << relativePath.path();
    if ( deviceId == -1 )
    {
#ifdef Q_OS_WIN32
        absolutePath.setPath( relativePath.toLocalFile() );
#else
        absolutePath.setPath( "/" );
        absolutePath.addPath( relativePath.path() );
#endif
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
        const QStringList lastMountPoint = CollectionManager::instance()->sqlStorage()->query(
                                                 QString( "SELECT lastmountpoint FROM devices WHERE id = %1" )
                                                 .arg( deviceId ) );
        if ( lastMountPoint.count() == 0 )
        {
            //hmm, no device with that id in the DB...serious problem
            getAbsolutePath( -1, relativePath, absolutePath );
            warning() << "Device " << deviceId << " not in database, this should never happen! Returning " << absolutePath.path();
        }
        else
        {
            absolutePath.setPath( lastMountPoint.first() );
            absolutePath.addPath( relativePath.path() );
            absolutePath.cleanPath();
            debug() << "Device " << deviceId << " not mounted, using last mount point and returning " << absolutePath.path();
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
    #ifdef Q_OS_WIN32
        return url.toLocalFile();
    #else
        return url.path();
    #endif
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
#ifdef Q_OS_WIN32
        QString rpath = absolutePath.toLocalFile();
#else
        QString rpath = KUrl::relativePath( "/", absolutePath.path() );
#endif
        relativePath.setPath( rpath );
    }
}

QString
MountPointManager::getRelativePath( const int deviceId, const QString& absolutePath ) const
{
    KUrl url;
    getRelativePath( deviceId, KUrl( absolutePath ), url );
    return url.toLocalFile();
}

// void
// MountPointManager::mediumChanged( const Medium *m )
// {
//     DEBUG_BLOCK
//     if ( !m ) return;
//     if ( m->isMounted() )
//     {
//         foreach( DeviceHandlerFactory *factory, m_mediumFactories )
//         {
//             if ( factory->canHandle ( m ) )
//             {
//                 debug() << "found handler for " << m->id();
//                 DeviceHandler *handler = factory->createHandler( m );
//                 if( !handler )
//                 {
//                     debug() << "Factory " << factory->type() << "could not create device handler";
//                     break;
//                 }
//                 int key = handler->getDeviceID();
//                 m_handlerMapMutex.lock();
//                 if ( m_handlerMap.contains( key ) )
//                 {
//                     debug() << "Key " << key << " already exists in handlerMap, replacing";
//                     delete m_handlerMap[key];
//                     m_handlerMap.remove( key );
//                 }
//                 m_handlerMap.insert( key, handler );
//                 m_handlerMapMutex.unlock();
//                 debug() << "added device " << key << " with mount point " << m->mountPoint();
//                 emit mediumConnected( key );
//                 break;  //we found the added medium and don't have to check the other device handlers
//             }
//         }
//     }
//     else
//     {
//         m_handlerMapMutex.lock();
//         foreach( DeviceHandler *dh, m_handlerMap )
//         {
//             if ( dh->deviceIsMedium( m ) )
//             {
//                 int key = m_handlerMap.key( dh );
//                 m_handlerMap.remove( key );
//                 delete dh;
//                 debug() << "removed device " << key;
//                 m_handlerMapMutex.unlock();
//                 emit mediumRemoved( key );
//                 //we found the medium which was removed, so we can abort the loop
//                 return;
//             }
//         }
//         m_handlerMapMutex.unlock();
//     }
// }
//

IdList
MountPointManager::getMountedDeviceIds() const
{
    m_handlerMapMutex.lock();
    IdList list( m_handlerMap.keys() );
    m_handlerMapMutex.unlock();
    list.append( -1 );
    return list;
}

QStringList
MountPointManager::collectionFolders()
{
    DEBUG_BLOCK

    //TODO max: cache data
    QStringList result;
    KConfigGroup folders = Amarok::config( "Collection Folders" );
    IdList ids = getMountedDeviceIds();
    foreach( int id, ids )
    {
        const QStringList rpaths = folders.readEntry( QString::number( id ), QStringList() );
        foreach( const QString &strIt, rpaths )
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
    if( result.isEmpty() )
    {
        const QString musicDir = QDesktopServices::storageLocation( QDesktopServices::MusicLocation );
        debug() << "QDesktopServices::MusicLocation: " << musicDir; 

        if( !musicDir.isEmpty() )
        {
            const QDir dir( musicDir );
            if( dir != QDir::home() && dir.exists() )
            {
                result << musicDir;
            }
        }
    }
    return result;
}

void
MountPointManager::setCollectionFolders( const QStringList &folders )
{
    if( folders.size() == 1 )
    {
        if( folders[0] == QDesktopServices::storageLocation( QDesktopServices::MusicLocation ) )
        {
            return;
        }
    }
    typedef QMap<int, QStringList> FolderMap;
    KConfigGroup folderConf = Amarok::config( "Collection Folders" );
    FolderMap folderMap;
    
    foreach( const QString &folder, folders )
    {
        int id = getIdForUrl( folder );
        const QString rpath = getRelativePath( id, folder );
        if( folderMap.contains( id ) ) {
            if( !folderMap[id].contains( rpath ) )
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
    QStringList urls = CollectionManager::instance()->sqlStorage()->query( "SELECT url FROM statistics WHERE deviceid = -2;" );
    foreach( const QString &url, urls )
    {
        if ( QFile::exists( url ) )
        {
            int deviceid = getIdForUrl( url );
            SqlStorage *db = CollectionManager::instance()->sqlStorage();
            QString rpath = getRelativePath( deviceid, url );
            QString update = QString( "UPDATE statistics SET deviceid = %1, url = '%2'" )
                                      .arg( deviceid )
                                      .arg( db->escape( rpath ) );
            update += QString( " WHERE url = '%1' AND deviceid = -2;" )
                               .arg( db->escape( url ) );
            db->query( update );
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
    AMAROK_NOTIMPLEMENTED
    //ThreadWeaver::Weaver::instance()->enqueue( new UrlUpdateJob( this ) );
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
    DEBUG_BLOCK
    Solid::Predicate predicate = Solid::Predicate( Solid::DeviceInterface::StorageVolume, "udi", udi );
    QList<Solid::Device> devices = Solid::Device::listFromQuery( predicate );
    //there'll be maximum one device because we are using the udi in the predicate
    if( !devices.isEmpty() )
    {
        Solid::Device device = devices[0];
        createHandlerFromDevice( device, udi );
    }
}

void
MountPointManager::deviceRemoved( const QString &udi )
{
    DEBUG_BLOCK
    m_handlerMapMutex.lock();
    foreach( DeviceHandler *dh, m_handlerMap )
    {
        if( dh->deviceMatchesUdi( udi ) )
        {
            int key = m_handlerMap.key( dh );
            m_handlerMap.remove( key );
            delete dh;
            debug() << "removed device " << key;
            m_handlerMapMutex.unlock();
            //we found the medium which was removed, so we can abort the loop
            emit deviceRemoved( key );
            return;
        }
    }
    m_handlerMapMutex.unlock();
}

void MountPointManager::createHandlerFromDevice( const Solid::Device& device, const QString &udi )
{
    if ( device.isValid() )
    {
        debug() << "Device added and mounted, checking handlers";
        foreach( DeviceHandlerFactory *factory, m_mediumFactories )
        {
            if( factory->canHandle( device ) )
            {
                debug() << "found handler for " << udi;
                DeviceHandler *handler = factory->createHandler( device, udi );
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
//                 debug() << "added device " << key << " with mount point " << volumeAccess->mountPoint();
                emit deviceAdded( key );
                break;  //we found the added medium and don't have to check the other device handlers
            }
        }
    }
}


//UrlUpdateJob

UrlUpdateJob::UrlUpdateJob( QObject *dependent )
    : ThreadWeaver::Job( dependent )
{
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( deleteLater() ) );
}

void
UrlUpdateJob::run()
{
    DEBUG_BLOCK
    updateStatistics();
    updateLabels();
}

void UrlUpdateJob::updateStatistics( )
{
    AMAROK_NOTIMPLEMENTED
#if 0
    SqlStorage *db = CollectionManager::instance()->sqlStorage();
    MountPointManager *mpm = MountPointManager::instance();
    QStringList urls = db->query( "SELECT s.deviceid,s.url "
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

            int statCount = db->query(
                            QString( "SELECT COUNT( url ) FROM statistics WHERE deviceid = %1 AND url = '%2';" )
                                        .arg( newDeviceid )
                                        .arg( db->escape( newRpath ) ) ).first().toInt();
            if( statCount )
                continue;       //statistics row with new URL/deviceid values already exists

            QString sql = QString( "UPDATE statistics SET deviceid = %1, url = '%2'" )
                                .arg( newDeviceid ).arg( db->escape( newRpath ) );
            sql += QString( " WHERE deviceid = %1 AND url = '%2';" )
                                .arg( deviceid ).arg( db->escape( rpath ) );
            db->query( sql );
        }
    }
#endif
}

void UrlUpdateJob::updateLabels( )
{
    AMAROK_NOTIMPLEMENTED

#if 0
    SqlStorage *db = CollectionManager::instance()->sqlStorage();
    MountPointManager *mpm = MountPointManager::instance();
    QStringList labels = db->query( "SELECT l.deviceid,l.url "
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
            QStringList labelids = db->query(
                                        QString( "SELECT labelid FROM tags_labels WHERE deviceid = %1 AND url = '%2';" )
                                                 .arg( QString::number( newDeviceid ), db->escape( newRpath ) ) );
            QString existingLabelids;
            if( !labelids.isEmpty() )
            {
                existingLabelids = " AND labelid NOT IN (";
                oldForeach( labelids )
                {
                    if( it != labelids.constBegin() )
                        existingLabelids += ',';
                    existingLabelids += *it;
                }
                existingLabelids += ')';
            }
            QString sql = QString( "UPDATE tags_labels SET deviceid = %1, url = '%2' "
                                    "WHERE deviceid = %3 AND url = '%4'%5;" )
                                    .arg( newDeviceid )
                                    .arg( db->escape( newRpath ),
                                          QString::number( deviceid ),
                                          db->escape( rpath ),
                                          existingLabelids );
            db->query( sql );
        }
    }
#endif
}

#include "MountPointManager.moc"
