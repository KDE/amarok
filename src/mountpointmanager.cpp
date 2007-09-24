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

#include "debug.h"

#include "amarok.h"
#include "amarokconfig.h"   //used in init()
#include "collectiondb.h"
#include "devicemanager.h"
#include "mountpointmanager.h"
#include "pluginmanager.h"
#include "statusbar.h"

#include <kglobal.h>        //used in init()
#include <ktrader.h>

#include <qfile.h>
#include <qstringlist.h>
#include <qtimer.h>
#include <qvaluelist.h>

typedef Medium::List MediumList;

MountPointManager::MountPointManager()
    : QObject( 0, "MountPointManager" )
    , m_noDeviceManager( false )
{

    if ( !Amarok::config( "Collection" )->readBoolEntry( "DynamicCollection", true ) )
    {
        debug() << "Dynamic Collection deactivated in amarokrc, not loading plugins, not connecting signals" << endl;
        return;
    }
    //we are only interested in the mounting or unmounting of mediums
    //therefore it is enough to listen to DeviceManager's mediumChanged signal
    if (DeviceManager::instance()->isValid() )
    {
        connect( DeviceManager::instance(), SIGNAL( mediumAdded( const Medium*, QString ) ), SLOT( mediumAdded( const Medium* ) ) );
        connect( DeviceManager::instance(), SIGNAL( mediumChanged( const Medium*, QString ) ), SLOT( mediumChanged( const Medium* ) ) );
        connect( DeviceManager::instance(), SIGNAL( mediumRemoved( const Medium*, QString ) ), SLOT( mediumRemoved( const Medium* ) ) );
    }
    else
    {
        handleMissingMediaManager();
    }

    m_mediumFactories.setAutoDelete( true );
    m_remoteFactories.setAutoDelete( true );
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
    foreachType( HandlerMap, m_handlerMap )
    {
        delete it.data();
    }
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
    KTrader::OfferList plugins = PluginManager::query( "[X-KDE-Amarok-plugintype] == 'device'" );
    debug() << "Received [" << QString::number( plugins.count() ) << "] device plugin offers" << endl;
    foreachType( KTrader::OfferList, plugins )
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
                debug() << "Unknown DeviceHandlerFactory" << endl;
        }
        else debug() << "Plugin could not be loaded" << endl;
    }
    //we need access to the unfiltered data
    MediumList list = DeviceManager::instance()->getDeviceList();
    foreachType ( MediumList, list )
    {
        mediumChanged( &(*it) );
    }
    if( !KGlobal::config()->hasGroup( "Collection Folders" ) )
    {
        QStringList folders = AmarokConfig::collectionFolders();
        if( !folders.isEmpty() )
            setCollectionFolders( folders );
    }
}

int
MountPointManager::getIdForUrl( KURL url )
{
    uint mountPointLength = 0;
    int id = -1;
    m_handlerMapMutex.lock();
    foreachType( HandlerMap, m_handlerMap )
    {
        if ( url.path().startsWith( it.data()->getDevicePath() ) && mountPointLength < it.data()->getDevicePath().length() )
        {
            id = it.key();
            mountPointLength = it.data()->getDevicePath().length();
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
        //treat -1 as mount point / in al other methods
        return -1;
    }
}

int
MountPointManager::getIdForUrl( const QString &url )
{
    return getIdForUrl( KURL::fromPathOrURL( url ) );
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
MountPointManager::getAbsolutePath( const int deviceId, const KURL& relativePath, KURL& absolutePath) const
{
    //debug() << "id is " << deviceId << ", relative path is " << relativePath.path() << endl;
    if ( deviceId == -1 )
    {
        absolutePath.setPath( "/" );
        absolutePath.addPath( relativePath.path() );
        absolutePath.cleanPath();
        //debug() << "Deviceid is -1, using relative Path as absolute Path, returning " << absolutePath.path() << endl;
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
//             debug() << "Device " << deviceId << " not mounted, using last mount point and returning " << absolutePath.path() << endl;
        }
    }
}

QString
MountPointManager::getAbsolutePath( const int deviceId, const QString& relativePath ) const
{
    KURL rpath;
    rpath.setProtocol("file");
    rpath.setPath( relativePath );
    KURL url;
    getAbsolutePath( deviceId, rpath, url );
    return url.path();
}

void
MountPointManager::getRelativePath( const int deviceId, const KURL& absolutePath, KURL& relativePath ) const
{
    m_handlerMapMutex.lock();
    if ( deviceId != -1 && m_handlerMap.contains( deviceId ) )
    {
        //FIXME max: returns garbage if the absolute path is actually not under the device's mount point
        QString rpath = KURL::relativePath( m_handlerMap[deviceId]->getDevicePath(), absolutePath.path() );
        m_handlerMapMutex.unlock();
        relativePath.setPath( rpath );
    }
    else
    {
        m_handlerMapMutex.unlock();
        //TODO: better error handling
        QString rpath = KURL::relativePath( "/", absolutePath.path() );
        relativePath.setPath( rpath );
    }
}

QString
MountPointManager::getRelativePath( const int deviceId, const QString& absolutePath ) const
{
    KURL url;
    getRelativePath( deviceId, KURL::fromPathOrURL( absolutePath ), url );
    return url.path();
}

void
MountPointManager::mediumChanged( const Medium *m )
{
    DEBUG_BLOCK
    if ( !m ) return;
    if ( m->isMounted() )
    {
        foreachType( FactoryList, m_mediumFactories )
        {
            if ( (*it)->canHandle ( m ) )
            {
                debug() << "found handler for " << m->id() << endl;
                DeviceHandler *handler = (*it)->createHandler( m );
                if( !handler )
                {
                    debug() << "Factory " << (*it)->type() << "could not create device handler" << endl;
                    break;
                }
                int key = handler->getDeviceID();
                m_handlerMapMutex.lock();
                if ( m_handlerMap.contains( key ) )
                {
                    debug() << "Key " << key << " already exists in handlerMap, replacing" << endl;
                    delete m_handlerMap[key];
                    m_handlerMap.erase( key );
                }
                m_handlerMap.insert( key, handler );
                m_handlerMapMutex.unlock();
                debug() << "added device " << key << " with mount point " << m->mountPoint() << endl;
                emit mediumConnected( key );
                break;  //we found the added medium and don't have to check the other device handlers
            }
        }
    }
    else
    {
        m_handlerMapMutex.lock();
        foreachType( HandlerMap, m_handlerMap )
        {
            if ( it.data()->deviceIsMedium( m ) )
            {
                delete it.data();
                int key = it.key();
                m_handlerMap.erase( key );
                debug() << "removed device " << key << endl;
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
        foreachType( HandlerMap, m_handlerMap )
        {
            if ( it.data()->deviceIsMedium( m ) )
            {
                delete it.data();
                int key = it.key();
                m_handlerMap.erase( key );
                debug() << "removed device " << key << endl;
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
        debug() << "Device added and mounted, checking handlers" << endl;
        foreachType( FactoryList, m_mediumFactories )
        {
            if ( (*it)->canHandle ( m ) )
            {
                debug() << "found handler for " << m->id() << endl;
                DeviceHandler *handler = (*it)->createHandler( m );
                if( !handler )
                {
                    debug() << "Factory " << (*it)->type() << "could not create device handler" << endl;
                    break;
                }
                int key = handler->getDeviceID();
                m_handlerMapMutex.lock();
                if ( m_handlerMap.contains( key ) )
                {
                    debug() << "Key " << key << " already exists in handlerMap, replacing" << endl;
                    delete m_handlerMap[key];
                    m_handlerMap.erase( key );
                }
                m_handlerMap.insert( key, handler );
                m_handlerMapMutex.unlock();
                debug() << "added device " << key << " with mount point " << m->mountPoint() << endl;
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
    KConfig* const folders = Amarok::config( "Collection Folders" );
    IdList ids = getMountedDeviceIds();
    foreachType( IdList, ids )
    {
        QStringList rpaths = folders->readListEntry( QString::number( *it ) );
        for( QStringList::ConstIterator strIt = rpaths.begin(), end = rpaths.end(); strIt != end; ++strIt )
        {
            QString absPath;
            if ( *strIt == "./" )
            {
                absPath = getMountPointForId( *it );
            }
            else
            {
                absPath = getAbsolutePath( *it, *strIt );
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
    KConfig* const folderConf = Amarok::config( "Collection Folders" );
    FolderMap folderMap;
    foreach( folders )
    {
        int id = getIdForUrl( *it );
        QString rpath = getRelativePath( id, *it );
        if ( folderMap.contains( id ) ) {
            if ( !folderMap[id].contains( rpath ) )
                folderMap[id].append( rpath );
        }
        else
            folderMap[id] = QStringList( rpath );
    }
    //make sure that collection folders on devices which are not in foldermap are deleted
    IdList ids = getMountedDeviceIds();
    foreachType( IdList, ids )
    {
        if( !folderMap.contains( *it ) )
        {
            folderConf->deleteEntry( QString::number( *it ) );
        }
    }
    foreachType( FolderMap, folderMap )
    {
        folderConf->writeEntry( QString::number( it.key() ), it.data() );
    }
}

void
MountPointManager::migrateStatistics()
{
    QStringList urls = CollectionDB::instance()->query( "SELECT url FROM statistics WHERE deviceid = -2;" );
    foreach( urls )
    {
        if ( QFile::exists( *it) )
        {
            int deviceid = getIdForUrl( *it );
            QString rpath = getRelativePath( deviceid, *it );
            QString update = QString( "UPDATE statistics SET deviceid = %1, url = '%2'" )
                                      .arg( deviceid )
                                      .arg( CollectionDB::instance()->escapeString( rpath ) );
            update += QString( " WHERE url = '%1' AND deviceid = -2;" )
                               .arg( CollectionDB::instance()->escapeString( *it ) );
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
MountPointManager::handleMissingMediaManager()
{
    //TODO this method should activate a fallback mode which simply shows all songs and uses the
    //device's last mount point to build the absolute path
    m_noDeviceManager = true;
    //Amarok::StatusBar::instance()->longMessage( i18n( "BlaBla" ), KDE::StatusBar::Warning );
}

void
MountPointManager::checkDeviceAvailability()
{
    //code to actively scan for devices which are not supported by KDE mediamanager should go here
    //method is not actually called yet
}

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
    debug() << "Trying to update " << urls.count() / 2 << " statistics rows" << endl;
    foreach( urls )
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
    debug() << "Trying to update " << labels.count() / 2 << " tags_labels rows" << endl;
    foreach( labels )
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
                foreach( labelids )
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
