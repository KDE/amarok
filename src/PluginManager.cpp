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
    m_collectionFactories = findPlugins<Collections::CollectionFactory>( QLatin1String("collection") );
    if( m_collectionFactories.isEmpty() )
    {
        debug() << "No Amarok collection plugins found, running kbuildsycoca4.";
        KBuildSycocaProgressDialog::rebuildKSycoca( 0 );

        debug() << "Second attempt at finding collection plugins";
        m_collectionFactories = findPlugins<Collections::CollectionFactory>( QLatin1String("collection") );

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

    PERF_LOG( "Loading collection plugins" )
    CollectionManager::instance()->init( m_collectionFactories );
    PERF_LOG( "Loaded collection plugins" )

    m_servicePluginManager = new ServicePluginManager( this );
    m_serviceFactories = findPlugins<ServiceFactory>( QLatin1String("service") );

    PERF_LOG( "Loading service plugins" )
    m_servicePluginManager->init( m_serviceFactories );
    PERF_LOG( "Loaded service plugins" )

    Collections::Collection *coll = CollectionManager::instance()->primaryCollection();
    m_mountPointManager = static_cast<Collections::SqlCollection*>( coll )->mountPointManager();
    m_deviceFactories = findPlugins<DeviceHandlerFactory>( QLatin1String("device") );

    PERF_LOG( "Loading device plugins" )
    m_mountPointManager->loadDevicePlugins( m_deviceFactories );
    PERF_LOG( "Loaded device plugins" )
}

KPluginInfo::List
Plugins::PluginManager::collectionPluginInfos() const
{
    KPluginInfo::List infos;
    foreach( Collections::CollectionFactory *factory, m_collectionFactories )
        infos.append( factory->info() );
    return infos;
}

QList<Collections::CollectionFactory*>
Plugins::PluginManager::collectionFactories() const
{
    return m_collectionFactories;
}

KPluginInfo::List
Plugins::PluginManager::servicePluginInfos() const
{
    KPluginInfo::List infos;
    foreach( ServiceFactory *factory, m_serviceFactories )
        infos.append( factory->info() );
    return infos;
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

KPluginInfo::List
Plugins::PluginManager::devicePluginInfos() const
{
    KPluginInfo::List infos;
    foreach( DeviceHandlerFactory *factory, m_deviceFactories )
        infos.append( factory->info() );
    return infos;
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
    m_servicePluginManager->checkEnabledStates( m_serviceFactories );
}

template<typename T>
QList<T*>
Plugins::PluginManager::findPlugins( const QString &type )
{
    DEBUG_BLOCK
    QString pluginsQuery = QString::fromLatin1( "[X-KDE-Amarok-plugintype] == '%1'" ).arg( type );
    KService::List plugins = Plugins::PluginManager::query( pluginsQuery );
    debug() << QString( "Received %1 plugin offers for %2 type" ).arg( plugins.count() ).arg( type );

    QList<T*> factories;
    foreach( const KService::Ptr &service, plugins )
    {
        const QString name( service->property( QLatin1String("X-KDE-Amarok-name") ).toString() );
        KPluginLoader loader( *( service.constData() ) );
        KPluginFactory *pluginFactory( loader.factory() );

        if( !pluginFactory )
        {
            warning() << QString( "Failed to get factory '%1' from KPluginLoader: %2" )
                            .arg( name, loader.errorString() );
            continue;
        }

        T *factory = 0;
        if( (factory = pluginFactory->create<T>( this )) )
            factories << factory;
        else
            debug() << "Plugin" << name << "has wrong factory class:" << loader.errorString();
    }
    return factories;
}

KService::List
Plugins::PluginManager::query( const QString &constraint )
{
    // Add versioning constraint
    QString str = QString::fromLatin1( "[X-KDE-Amarok-framework-version] == %1" )
            .arg( s_pluginFrameworkVersion );
    if( !constraint.trimmed().isEmpty() )
        str += QString::fromLatin1( " and %1" ).arg( constraint );
    str += QString::fromLatin1( " and [X-KDE-Amarok-rank] > 0" );

    debug() << "Plugin trader constraint:" << str;
    return KServiceTypeTrader::self()->query( "Amarok/Plugin", str );
}

void
Plugins::PluginManager::showAbout( const QString &constraint )
{
    KService::List offers = query( constraint );

    if ( offers.isEmpty() )
        return;

    KService::Ptr s = offers.front();

    const QString body = "<tr><td>%1</td><td>%2</td></tr>";

    QString str  = "<html><body><table width=\"100%\" border=\"1\">";

    str += body.arg( i18nc( "Title, as in: the title of this item", "Name" ),                s->name() );
    str += body.arg( i18n( "Library" ),             s->library() );
    str += body.arg( i18n( "Authors" ),             s->property( "X-KDE-Amarok-authors" ).toStringList().join( "\n" ) );
    str += body.arg( i18nc( "Property, belonging to the author of this item", "Email" ),               s->property( "X-KDE-Amarok-email" ).toStringList().join( "\n" ) );
    str += body.arg( i18n( "Version" ),             s->property( "X-KDE-Amarok-version" ).toString() );
    str += body.arg( i18n( "Framework Version" ),   s->property( "X-KDE-Amarok-framework-version" ).toString() );

    str += "</table></body></html>";

    KMessageBox::information( 0, str, i18n( "Plugin Information" ) );
}

void
Plugins::PluginManager::dump( const KService::Ptr service )
{
    #define ENDLI endl << Debug::indent()

    debug()
      << ENDLI
      << "PluginManager Service Info:" << ENDLI
      << "---------------------------" << ENDLI
      << "name                          :" << service->name() << ENDLI
      << "library                       :" << service->library() << ENDLI
      << "desktopEntryPath              :" << service->entryPath() << ENDLI
      << "X-KDE-Amarok-plugintype       :" << service->property( "X-KDE-Amarok-plugintype" ).toString() << ENDLI
      << "X-KDE-Amarok-name             :" << service->property( "X-KDE-Amarok-name" ).toString() << ENDLI
      << "X-KDE-Amarok-authors          :" << service->property( "X-KDE-Amarok-authors" ).toStringList() << ENDLI
      << "X-KDE-Amarok-rank             :" << service->property( "X-KDE-Amarok-rank" ).toString() << ENDLI
      << "X-KDE-Amarok-version          :" << service->property( "X-KDE-Amarok-version" ).toString() << ENDLI
      << "X-KDE-Amarok-framework-version:" << service->property( "X-KDE-Amarok-framework-version" ).toString()
      << endl
     ;

    #undef ENDLI
}

int
Plugins::PluginManager::pluginFrameworkVersion()
{
    return s_pluginFrameworkVersion;
}

#include "PluginManager.moc"
