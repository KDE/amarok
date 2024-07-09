/****************************************************************************************
 * Copyright (c) 2004-2013 Mark Kretschmann <kretschmann@kde.org>                       *
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

#include <core/support/Amarok.h>
#include <core/support/Components.h>
#include <core/support/Debug.h>
#include <core-impl/collections/support/CollectionManager.h>
#include <core-impl/storage/StorageManager.h>
#include <services/ServiceBase.h>
#include <services/ServicePluginManager.h>
#include <statsyncing/Controller.h>
#include <statsyncing/ProviderFactory.h>
#include <storage/StorageFactory.h>

#include <KLocalizedString>
#include <KMessageBox>

#include <QGuiApplication>
#include <QPluginLoader>


/** Defines the used plugin version number.
 *
 *  This must match the desktop files.
 */
const int Plugins::PluginManager::s_pluginFrameworkVersion = 77;
Plugins::PluginManager* Plugins::PluginManager::s_instance = nullptr;

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
        s_instance = nullptr;
    }
}

Plugins::PluginManager::PluginManager( QObject *parent )
    : QObject( parent )
{
    DEBUG_BLOCK
    setObjectName( QStringLiteral("PluginManager") );
    s_instance = this;

    PERF_LOG( "Initialising Plugin Manager" )
    init();
    PERF_LOG( "Initialised Plugin Manager" )
}

Plugins::PluginManager::~PluginManager()
{
    // tell the managers to get rid of their current factories
    QList<QSharedPointer<Plugins::PluginFactory> > emptyFactories;

    StatSyncing::Controller *controller = Amarok::Components::statSyncingController();
    if( controller )
        controller->setFactories( emptyFactories );
    ServicePluginManager::instance()->setFactories( emptyFactories );
    CollectionManager::instance()->setFactories( emptyFactories );
    StorageManager::instance()->setFactories( emptyFactories );
}

void
Plugins::PluginManager::init()
{
    checkPluginEnabledStates();
}

QVector<KPluginMetaData>
Plugins::PluginManager::plugins( Type type ) const
{
    QVector<KPluginMetaData> infos;

    for( const auto &pluginInfo : m_pluginsByType.value( type ) )
    {
        infos << pluginInfo;
    }

    return infos;
}

QVector<KPluginMetaData>
Plugins::PluginManager::enabledPlugins(Plugins::PluginManager::Type type) const
{
    QVector<KPluginMetaData> enabledList;

    for( const auto &plugin : m_pluginsByType.value( type ) )
    {
        if( isPluginEnabled( plugin ) )
            enabledList << plugin;
    }

    return enabledList;
}

QList<QSharedPointer<Plugins::PluginFactory> >
Plugins::PluginManager::factories( Type type ) const
{
    return m_factoriesByType.value( type );
}

void
Plugins::PluginManager::checkPluginEnabledStates()
{
    DEBUG_BLOCK

    // re-create all the member infos.
    m_plugins.clear();
    m_pluginsByType.clear();
    m_factoriesByType.clear();

    m_plugins = findPlugins(); // reload all the plugins plus their enabled state

    if( m_plugins.isEmpty() ) // exit if no plugins are found
    {
        if( qobject_cast<QGuiApplication*>( qApp ) )
        {
            KMessageBox::error( nullptr, i18n( "Amarok could not find any plugins. This indicates an installation problem." ) );
        }
        else
        {
            warning() << "Amarok could not find any plugins. Bailing out.";
        }
        // don't use QApplication::exit, as the eventloop may not have started yet
        std::exit( EXIT_SUCCESS );
    }

    // sort the plugin infos by type
    for( const auto &pluginInfo : m_plugins )
    {
        // create the factories and sort them by type
        auto factory = createFactory( pluginInfo );

        if( factory )
        {
            Type type;

            if( qobject_cast<StorageFactory*>( factory ) )
                type = Storage;
            else if( qobject_cast<Collections::CollectionFactory*>( factory ) )
                type = Collection;
            else if( qobject_cast<ServiceFactory*>( factory ) )
                type = Service;
            else if( qobject_cast<StatSyncing::ProviderFactory*>( factory ) )
                type = Importer;
            else
            {
                warning() << pluginInfo.name() << "has unknown category";
                warning() << pluginInfo.rawData().keys();
                continue;
            }

            m_pluginsByType[ type ] << pluginInfo;

            if( isPluginEnabled( pluginInfo ) )
                m_factoriesByType[ type ] << factory;
        }
        else
            warning() << pluginInfo.name() << "could not create factory";
    }

    // the setFactories functions should:
    // - filter out factories not useful (e.g. services when setting collections)
    // - handle the new list of factories, disabling old ones and enabling new ones.


    PERF_LOG( "Loading storage plugins" )
    StorageManager::instance()->setFactories( m_factoriesByType.value( Storage ) );
    PERF_LOG( "Loaded storage plugins" )

    PERF_LOG( "Loading collection plugins" )
    CollectionManager::instance()->setFactories( m_factoriesByType.value( Collection ) );
    PERF_LOG( "Loaded collection plugins" )

    PERF_LOG( "Loading service plugins" )
    ServicePluginManager::instance()->setFactories( m_factoriesByType.value( Service ) );
    PERF_LOG( "Loaded service plugins" )

    PERF_LOG( "Loading importer plugins" )
    StatSyncing::Controller *controller = Amarok::Components::statSyncingController();
    if( controller )
        controller->setFactories( m_factoriesByType.value( Importer ) );
    PERF_LOG( "Loaded importer plugins" )

    // init all new factories
    // do this after they were added to the sub-manager so that they
    // have a chance to connect to signals
    //
    // we need to init by type and the storages need to go first
    for( const auto &factory : m_factoriesByType[ Storage ] )
        factory->init();
    for( const auto &factory : m_factoriesByType[ Collection ] )
        factory->init();
    for( const auto &factory : m_factoriesByType[ Service ] )
        factory->init();
    for( const auto &factory : m_factoriesByType[ Importer ] )
        factory->init();
}


bool
Plugins::PluginManager::isPluginEnabled( const KPluginMetaData &plugin ) const
{
    // mysql storage and collection are vital. They need to be loaded always

    auto raw = plugin.rawData();
    int version = raw.value( QStringLiteral("X-KDE-Amarok-framework-version") ).toInt();
    int rank = raw.value( QStringLiteral("X-KDE-Amarok-rank") ).toInt();

    if( version != s_pluginFrameworkVersion )
    {
        warning() << "Plugin" << plugin.pluginId() << "has frameworks version" << version
                  << ". Version" << s_pluginFrameworkVersion << "is required";
        return false;
    }

    if( rank == 0 )
    {
        warning() << "Plugin" << plugin.pluginId() << "has rank 0";
        return false;
    }

    auto vital = raw.value( QStringLiteral( "X-KDE-Amarok-vital" ) );

    if( !vital.isUndefined())
    {
        if( vital.toBool() || vital.toString().toLower() == QStringLiteral("true") )
        {
            debug() << "Plugin" << plugin.pluginId() << "is vital";
            return true;
        }
    }

    return plugin.isEnabled( Amarok::config( QStringLiteral("Plugins") ) );
}


QSharedPointer<Plugins::PluginFactory>
Plugins::PluginManager::createFactory( const KPluginMetaData &pluginInfo )
{
    debug() << "Creating factory for plugin:" << pluginInfo.pluginId();

    // check if we already created this factory
    // note: old factories are not deleted.
    //   We can't very well just destroy a factory being
    //   currently used.
    const QString name = pluginInfo.pluginId();

    if( m_factoryCreated.contains( name ) )
        return m_factoryCreated.value( name );

    QPluginLoader loader( pluginInfo.fileName() );
    auto pointer = qobject_cast<PluginFactory*>( loader.instance() );
    auto pluginFactory = QSharedPointer<Plugins::PluginFactory>( pointer );

    if( !pluginFactory )
    {
        warning() << QStringLiteral( "Failed to get factory '%1' from QPluginLoader: %2" )
                     .arg( name, loader.errorString() );
        return QSharedPointer<Plugins::PluginFactory>();
    }

    m_factoryCreated[ name ] = pluginFactory;
    return pluginFactory;
}


QVector<KPluginMetaData>
Plugins::PluginManager::findPlugins()
{
    QVector<KPluginMetaData> plugins;
    for( const auto &location : QCoreApplication::libraryPaths() )
        plugins << KPluginMetaData::findPlugins( location, [] ( const KPluginMetaData &metadata )
            { return metadata.serviceTypes().contains( QStringLiteral( "Amarok/Plugin" ) ); } );

    for( const auto &plugin : plugins )
    {
        bool enabled = isPluginEnabled( plugin );
        debug() << "found plugin:" << plugin.pluginId()
                << "enabled:" << enabled;
    }
    debug() << plugins.count() << "plugins in total";

    return plugins;
}

int
Plugins::PluginManager::pluginFrameworkVersion()
{
    return s_pluginFrameworkVersion;
}

