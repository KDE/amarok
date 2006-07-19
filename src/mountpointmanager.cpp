//
// C++ Implementation: mountpointmanager
//
// Description: 
//
//
// Author: Maximilian Kossick <maximilian.kossick@googlemail.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//

#define DEBUG_PREFIX "MountPointManager"

#include "debug.h"

#include "amarok.h"
#include "collectiondb.h"
#include "devicemanager.h"
#include "mountpointmanager.h"
#include "pluginmanager.h"
#include "statusbar.h"

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
    //we are only interested in the mounting or unmounting of mediums
    //therefore it is enough to listen to DeviceManager's mediumChanged signal
    if (DeviceManager::instance()->isValid() )
    {
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
}


MountPointManager::~MountPointManager()
{
    foreachType( HandlerMap, m_handlerMap )
    {
        delete it.data();
    }
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
        amaroK::Plugin *plugin = PluginManager::createFromService( *it );
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
}

int
MountPointManager::getIdForUrl( KURL url )
{
    uint mountPointLength = 0;
    int id = -1;
    foreachType( HandlerMap, m_handlerMap )
    {
        if ( url.path().startsWith( it.data()->getDevicePath() ) && mountPointLength < it.data()->getDevicePath().length() )
        {
            id = it.key();
            mountPointLength = it.data()->getDevicePath().length();
        }
    }
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
MountPointManager::getIdForUrl( QString url )
{
    return getIdForUrl( KURL::fromPathOrURL( url ) );
}

void
MountPointManager::getMountPointForId( const int& id, KURL& url ) const
{
    if ( isMounted( id ) )
        url = KURL( m_handlerMap[id]->getDevicePath() );
    else
        //TODO better error handling
        url = KURL::fromPathOrURL( "/" );
}

void
MountPointManager::getAbsolutePath( const int& deviceId, const KURL& relativePath, KURL& absolutePath) const
{
    //debug() << "id is " << deviceId << ", relative path is " << relativePath.path() << endl;
    if ( deviceId == -1 )
    {
        absolutePath.setPath( "/" );
        absolutePath.addPath( relativePath.path() );
        absolutePath.cleanPath();
        debug() << "Deviceid is -1, using relative Path as absolute Path, returning " << absolutePath.path() << endl;
        return;
    }
    if ( m_handlerMap.contains( deviceId ) )
    {
        m_handlerMap[deviceId]->getURL( absolutePath, relativePath );
    }
    else
    {
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
            debug() << "Device " << deviceId << " not mounted, using last mount point and returning " << absolutePath.path() << endl;
        }
    }
}

QString
MountPointManager::getAbsolutePath( const int& deviceId, const QString& relativePath ) const
{
    KURL rpath;
    rpath.setPath( relativePath );
    KURL url;
    getAbsolutePath( deviceId, rpath, url );
    return url.path();
}

void
MountPointManager::getRelativePath( const int& deviceId, const KURL& absolutePath, KURL& relativePath ) const
{
    if ( deviceId != -1 && m_handlerMap.contains( deviceId ) )
    {
        //FIXME max: returns garbage if the absolute path is actually not under the device's mount point
        QString rpath = KURL::relativePath( m_handlerMap[deviceId]->getDevicePath(), absolutePath.path() );
        relativePath.setPath( rpath );
    }
    else
    {
        //TODO: better error handling
        QString rpath = KURL::relativePath( "/", absolutePath.path() );
        relativePath.setPath( rpath );
    }
}

QString
MountPointManager::getRelativePath( const int& deviceId, const QString& absolutePath ) const
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
                int key = handler->getDeviceID();
                m_handlerMap.insert( key, handler );
                debug() << "added device " << key << " with mount point " << m->mountPoint() << endl;
                emit mediumConnected( key );
                break;  //we found the added medium and dont have to check the other device handlers
            }
        }
    }
    else
    {
        foreachType( HandlerMap, m_handlerMap )
        {
            if ( it.data()->deviceIsMedium( m ) )
            {
                delete it.data();
                int key = it.key();
                m_handlerMap.erase( key );
                debug() << "removed device " << key << endl;
                emit mediumRemoved( key );
                //we found the medium which was removed, so we can abort the loop
                break;
            }
        }
    }
}

void
MountPointManager::mediumRemoved( const Medium* m )
{
}

IdList
MountPointManager::getMountedDeviceIds() const {
    IdList list( m_handlerMap.keys() );
    list.append( -1 );
    return list;
}

QStringList
MountPointManager::collectionFolders( )
{
    //TODO max: cache data
    QStringList result;
    KConfig* const folders = amaroK::config( "Collection Folders" );
    IdList ids = getMountedDeviceIds();
    foreachType( IdList, ids )
    {
        QStringList rpaths = folders->readListEntry( QString::number( *it ) );
        for( QStringList::ConstIterator strIt = rpaths.begin(), end = rpaths.end(); strIt != end; ++strIt )
        {
            result.append( getAbsolutePath( *it, *strIt ) );
        }
    }
    return result;
}

void
MountPointManager::setCollectionFolders( QStringList folders )
{
    //TODO max: cache data
    typedef QMap<int, QStringList> FolderMap;
    KConfig* const folderConf = amaroK::config( "Collection Folders" );
    FolderMap folderMap;
    foreach( folders )
    {
        int id = getIdForUrl( *it );
        QString rpath = getRelativePath( id, *it );
        if ( folderMap.contains( id ) )
            folderMap[id].append( rpath );
        else
            folderMap[id] = QStringList( rpath );
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
MountPointManager::handleMissingMediaManager()
{
    m_noDeviceManager = true;
    amaroK::StatusBar::instance()->longMessage( i18n( "BlaBla" ), KDE::StatusBar::Warning );
}

void
MountPointManager::checkDeviceAvailability()
{
    
}

#include "mountpointmanager.moc"
