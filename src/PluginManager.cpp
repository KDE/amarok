/****************************************************************************************
 * Copyright (c) 2004 Mark Kretschmann <kretschmann@kde.org>                            *
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

#define DEBUG_PREFIX "PluginManager"

#include "PluginManager.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core-impl/collections/db/sql/SqlCollection.h"
#include "MountPointManager.h"
#include "services/ServiceBase.h"
#include "services/ServicePluginManager.h"

#include <KBuildSycocaProgressDialog>
#include <KLocalizedString>
#include <KMessageBox>
#include <KServiceTypeTrader>

#include <QFile>

const int Plugins::PluginManager::s_pluginFrameworkVersion = 59;
Plugins::PluginManager* Plugins::PluginManager::s_instance = 0;

Plugins::PluginManager*
Plugins::PluginManager::instance()
{
    return s_instance ? s_instance : new PluginManager();
}

void
Plugins::PluginManager::destroy()
{
    if( s_instance )
    {
        delete s_instance;
        s_instance = 0;
    }
}

Plugins::PluginManager::PluginManager( QObject *parent )
    : QObject( parent )
{
    DEBUG_BLOCK
    setObjectName( "PluginManager" );
    s_instance = this;

    PERF_LOG( "Initialising Plugin Manager" )
    init();
    PERF_LOG( "Initialised Plugin Manager" )
}

Plugins::PluginManager::~PluginManager()
{
}

void
Plugins::PluginManager::init()
{
    findAllPlugins();

    PERF_LOG( "Loading collection plugins" )
    m_collectionFactories = createFactories<Collections::CollectionFactory>( QLatin1String("Collection") );
    handleEmptyCollectionFactories();
    CollectionManager::instance()->init( m_collectionFactories );
    PERF_LOG( "Loaded collection plugins" )

    PERF_LOG( "Loading service plugins" )
    m_servicePluginManager = new ServicePluginManager( this );
    m_serviceFactories = createFactories<ServiceFactory>( QLatin1String("Service") );
    m_servicePluginManager->init( m_serviceFactories );
    PERF_LOG( "Loaded service plugins" )

    PERF_LOG( "Loading device plugins" )
    Collections::Collection *coll = CollectionManager::instance()->primaryCollection();
    m_mountPointManager = static_cast<Collections::SqlCollection*>( coll )->mountPointManager();
    m_deviceFactories = createFactories<DeviceHandlerFactory>( QLatin1String("Device") );
    m_mountPointManager->loadDevicePlugins( m_deviceFactories );
    PERF_LOG( "Loaded device plugins" )
}

QList<Collections::CollectionFactory*>
Plugins::PluginManager::collectionFactories() const
{
    return m_collectionFactories;
}

QList<ServiceFactory*>
Plugins::PluginManager::serviceFactories() const
{
    return m_serviceFactories;
}

ServicePluginManager *
Plugins::PluginManager::servicePluginManager()
{
    return m_servicePluginManager;
}

QList<DeviceHandlerFactory*>
Plugins::PluginManager::deviceFactories() const
{
    return m_deviceFactories;
}

void
Plugins::PluginManager::checkPluginEnabledStates()
{
    DEBUG_BLOCK
    KPluginInfo::List newlyEnabledList;
    foreach( const KPluginInfo::List &plugins, m_pluginInfos )
    {
        foreach( const KPluginInfo &plugin, plugins )
        {
            QString name = plugin.pluginName();
            bool enabled = plugin.isPluginEnabled();
            if( !enabled || m_factoryCreated.value(name) )
                continue;

            QString cate = plugin.category();
            if( cate == QLatin1String("Collection") )
            {
                using namespace Collections;
                CollectionFactory *fac = createFactory<CollectionFactory>( plugin );
                if( fac )
                    m_collectionFactories << fac;
            }
            else if( cate == QLatin1String("Service") )
            {
                ServiceFactory *fac = createFactory<ServiceFactory>( plugin );
                if( fac )
                    m_serviceFactories << fac;
            }
            else if( cate == QLatin1String("Device") )
            {
                DeviceHandlerFactory *fac = createFactory<DeviceHandlerFactory>( plugin );
                if( fac )
                    m_deviceFactories << fac;
            }
        }
    }
    m_servicePluginManager->checkEnabledStates( m_serviceFactories );
}

KPluginInfo::List
Plugins::PluginManager::plugins( const QString &category )
{
    return m_pluginInfos.value( category );
}

template<typename T>
T*
Plugins::PluginManager::createFactory( const KPluginInfo &plugin )
{
    QString name = plugin.pluginName();
    bool enabledByDefault = plugin.isPluginEnabledByDefault();
    bool enabled = Amarok::config( "Plugins" ).readEntry( name + "Enabled", enabledByDefault );
    if( !enabled )
        return 0;

    KService::Ptr service = plugin.service();
    KPluginLoader loader( *( service.constData() ) );
    KPluginFactory *pluginFactory( loader.factory() );

    if( !pluginFactory )
    {
        warning() << QString( "Failed to get factory '%1' from KPluginLoader: %2" )
            .arg( name, loader.errorString() );
        return 0;
    }

    T *factory = 0;
    if( !(factory = pluginFactory->create<T>( this )) )
    {
        warning() << "Failed to create plugin" << name << loader.errorString();
        return 0;
    }

    debug() << "created factory for plugin" << name;
    m_factoryCreated[ name ] = true;
    return factory;
}

template<typename T>
QList<T*>
Plugins::PluginManager::createFactories( const QString &category )
{
    QList<T*> factories;
    foreach( const KPluginInfo &plugin, plugins( category ) )
    {
        T *factory = createFactory<T>( plugin );
        if( factory )
            factories << factory;
    }
    return factories;
}

void
Plugins::PluginManager::findAllPlugins()
{
    DEBUG_BLOCK
    QString query = QString::fromLatin1( "[X-KDE-Amarok-framework-version] == %1"
                                         "and [X-KDE-Amarok-rank] > 0" ).arg( s_pluginFrameworkVersion );

    KService::List services = KServiceTypeTrader::self()->query( "Amarok/Plugin", query );
    KPluginInfo::List plugins = KPluginInfo::fromServices( services );
    qSort( plugins ); // sort list by category

    foreach( const KPluginInfo &info, plugins )
    {
        QString name = info.pluginName();
        m_pluginInfos[ info.category() ] << info;
        debug() << "found plugin:" << name << "enabled:" << info.isPluginEnabled();
    }
    debug() << plugins.count() << "plugins in total";
}

void
Plugins::PluginManager::handleEmptyCollectionFactories()
{
    if( !m_collectionFactories.isEmpty() )
        return;

    debug() << "No Amarok collection plugins found, running kbuildsycoca4.";
    KBuildSycocaProgressDialog::rebuildKSycoca( 0 );

    debug() << "Second attempt at finding collection plugins";
    m_collectionFactories = createFactories<Collections::CollectionFactory>( QLatin1String("Collection") );

    if( m_collectionFactories.isEmpty() )
    {
        KMessageBox::error( 0, i18n(
                "<p>Amarok could not find any collection plugins. "
                "It is possible that Amarok is installed under the wrong prefix, please fix your installation using:<pre>"
                "$ cd /path/to/amarok/source-code/<br>"
                "$ su -c \"make uninstall\"<br>"
                "$ cmake -DCMAKE_INSTALL_PREFIX=`kde4-config --prefix` && su -c \"make install\"<br>"
                "$ kbuildsycoca4 --noincremental<br>"
                "$ amarok</pre>"
                "More information can be found in the README file. For further assistance join us at #amarok on irc.freenode.net.</p>" ) );
        // don't use QApplication::exit, as the eventloop may not have started yet
        std::exit( EXIT_SUCCESS );
    }
}

int
Plugins::PluginManager::pluginFrameworkVersion()
{
    return s_pluginFrameworkVersion;
}

#include "PluginManager.moc"
