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

#include "core/support/Amarok.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "core-impl/collections/db/sql/SqlCollection.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "services/ServicePluginManager.h"
#include "statsyncing/Controller.h"

#include <KLocalizedString>
#include <KMessageBox>
#include <KProcess>
#include <KServiceTypeTrader>
#include <KStandardDirs>

#include <QApplication>
#include <QFile>
#include <QMetaEnum>

#include <cstdlib>
#include <unistd.h>


const int Plugins::PluginManager::s_pluginFrameworkVersion = 71;
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
    if( !findPlugins() )
        handleNoPluginsFound();

    QString key;

    PERF_LOG( "Loading collection plugins" )
    key = QLatin1String( "Collection" );
    m_factories[ key ] = createFactories( key );
    CollectionManager::instance()->handleNewFactories( m_factories.value( key ) );
    PERF_LOG( "Loaded collection plugins" )

    PERF_LOG( "Loading service plugins" )
    key = QLatin1String( "Service" );
    m_servicePluginManager = new ServicePluginManager( this );
    m_factories[ key ] = createFactories( key );
    m_servicePluginManager->init( m_factories.value( key ) );
    PERF_LOG( "Loaded service plugins" )

    PERF_LOG( "Loading importer plugins" )
    key = QLatin1String( "Importer" );
    m_factories[ key ] = createFactories( key );
    StatSyncing::Controller *controller = Amarok::Components::statSyncingController();
    if( controller )
        controller->handleNewFactories( m_factories.value( key ) );
    PERF_LOG( "Loaded importer plugins" )
}

QList<Plugins::PluginFactory*>
Plugins::PluginManager::factories( const QString &category ) const
{
    return m_factories.value( category );
}

ServicePluginManager *
Plugins::PluginManager::servicePluginManager()
{
    return m_servicePluginManager;
}

void
Plugins::PluginManager::checkPluginEnabledStates()
{
    QList<PluginFactory*> newFactories;
    foreach( const KPluginInfo::List &plugins, m_pluginInfos )
    {
        foreach( KPluginInfo plugin, plugins )
        {
            plugin.load( Amarok::config(QLatin1String("Plugins")) ); // load enabled state of plugin
            QString name = plugin.pluginName();
            bool enabled = plugin.isPluginEnabled();
            if( !enabled || m_factoryCreated.value(name) )
                continue;

            PluginFactory *factory = createFactory( plugin );
            if( factory )
            {
                m_factories[ plugin.category() ] << factory;
                newFactories << factory;
            }
        }
    }
    m_servicePluginManager->checkEnabledStates( m_factories.value(QLatin1String("Service")) );
    CollectionManager::instance()->handleNewFactories( newFactories );
}

KPluginInfo::List
Plugins::PluginManager::plugins( const QString &category ) const
{
    return m_pluginInfos.value( category );
}

Plugins::PluginFactory*
Plugins::PluginManager::createFactory( const KPluginInfo &plugin )
{
    const QString name = plugin.pluginName();
    const bool useMySqlServer = Amarok::config( "MySQL" ).readEntry( "UseServer", false );
    bool enabled = false;
    if( name == QLatin1String("amarok_collection-mysqlservercollection") )
    {
        if( useMySqlServer )
            enabled = true;
    }
    else if( name == QLatin1String("amarok_collection-mysqlecollection") )
    {
        if( !useMySqlServer )
            enabled = true;
    }
    else
    {
        const bool enabledByDefault = plugin.isPluginEnabledByDefault();
        enabled = Amarok::config( "Plugins" ).readEntry( name + "Enabled", enabledByDefault );
    }
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

    PluginFactory *factory = 0;
    if( !(factory = pluginFactory->create<PluginFactory>( this )) )
    {
        warning() << "Failed to create plugin" << name << loader.errorString();
        return 0;
    }

    if( factory->pluginType() == PluginFactory::Unknown )
    {
        warning() << "factory has unknown type!";
        factory->deleteLater();
        return 0;
    }

    const QMetaObject *mo = factory->metaObject();
    QString type = mo->enumerator( mo->indexOfEnumerator("Type") ).valueToKey( factory->pluginType() );
    debug() << "created factory for plugin" << name << "type:" << type;
    m_factoryCreated[ name ] = true;
    return factory;
}

QList<Plugins::PluginFactory*>
Plugins::PluginManager::createFactories( const QString &category )
{
    QList<PluginFactory*> factories;
    foreach( const KPluginInfo &plugin, plugins( category ) )
    {
        PluginFactory *factory = createFactory( plugin );
        if( factory )
            factories << factory;
    }
    return factories;
}

int
Plugins::PluginManager::findPlugins()
{
    QString query = QString::fromLatin1( "[X-KDE-Amarok-framework-version] == %1"
                                         " and [X-KDE-Amarok-rank] > 0" )
                    .arg( s_pluginFrameworkVersion );

    KConfigGroup pluginsConfig = Amarok::config(QLatin1String("Plugins"));
    KService::List services = KServiceTypeTrader::self()->query( "Amarok/Plugin", query );
    KPluginInfo::List plugins = KPluginInfo::fromServices( services, pluginsConfig );
    qSort( plugins ); // sort list by category

    foreach( KPluginInfo info, plugins )
    {
        info.load( pluginsConfig ); // load the enabled state of plugin
        QString name = info.pluginName();
        m_pluginInfos[ info.category() ] << info;
        debug() << "found plugin:" << name << "enabled:" << info.isPluginEnabled();
    }
    debug() << plugins.count() << "plugins in total";

    return plugins.count();
}

void
Plugins::PluginManager::handleNoPluginsFound()
{
    DEBUG_BLOCK

    debug() << "No Amarok plugins found, running kbuildsycoca4.";

    // Run kbuildsycoca4 in a blocking fashion
    KProcess::execute( KStandardDirs::findExe( "kbuildsycoca4" ), QStringList( "--noincremental" ) );

    // Wait a bit until ksycoca has fully updated
    for( int i = 0; i < 3; i++ ) {
        sleep( 1 );
        QApplication::processEvents();
    }

    debug() << "Second attempt at finding plugins";

    if( !findPlugins() )
    {
        if( QApplication::type() != QApplication::Tty )
        {
            KMessageBox::error( 0, i18n( "Amarok could not find any plugins. This indicates an installation problem." ) );
        }
        else
        {
            warning() << "Amarok could not find any plugins. Bailing out.";
        }
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
