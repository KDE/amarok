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

#ifndef AMAROK_PLUGINMANAGER_H
#define AMAROK_PLUGINMANAGER_H

#include "amarok_export.h"
#include "core/support/PluginFactory.h"

class ServicePluginManager;

namespace Plugins {

class AMAROK_EXPORT PluginManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY( int pluginFrameworkVersion READ pluginFrameworkVersion )

    public:
        ~PluginManager();

        static PluginManager *instance();

        /** Destroys the instance of the PluginManager.
         *
         *  The order of the destruction is somewhat important.
         *  The PluginManager needs to be destroyed after all collections
         *  have been removed and before the CollectionManager,
         *  the ServicePluginManager and the StatSyncing::Controller are destroyed.
         */
        static void destroy();
        static int pluginFrameworkVersion();

        /**
         * Load any services that are configured to be loaded
         */
        void init();

        /** Returns plugin factories for the given plugin type */
        KPluginInfo::List plugins( PluginFactory::Type type ) const;

        /** Returns enabled plugin factories for the given plugin type.
         *
         *  This function will only return enable factories.
         *
         *  Owner of the PluginFactory pointers is the PluginManager
         *  and the pointers will only be valid while the PluginManager exists.
         */
        QList<PluginFactory*> factories( PluginFactory::Type type ) const;

        /** Check if any services were disabled and needs to be removed, or any
         *  that are hidden needs to be enabled
         *
         * This function will call the sub plugin managers (like CollectionManager)
         * setFactories function.
         */
        void checkPluginEnabledStates();

    private:
        /** Tries finding Amarok plugins */
        KPluginInfo::List findPlugins();

        /** Does additional effort to find plugins.
         *
         *  Starts kbuildsycoca4 thingie
         *  Stops Amarok if it still can't find plugins
         */
        void handleNoPluginsFound();

        /** Returns true if the plugin is enabled.
         *  This function will theck the default enabled state,
         *  the Amarok configuration state and the primary collection.
         *
         *  @returns true if the plugin is enabled.
         */
        bool isPluginEnabled( const KPluginInfo &factory ) const;

        /** Creates factories for all infos */
        QList<PluginFactory*> createFactories( const KPluginInfo::List& infos );
        PluginFactory* createFactory( const KPluginInfo &plugin );

        /// contains the names of all KPluginInfos that have factories created
        QHash<QString, PluginFactory*> m_factoryCreated;
        QHash<PluginFactory::Type, QList<PluginFactory*> > m_factoriesByType;
        KPluginInfo::List m_pluginInfos;
        QHash<PluginFactory::Type, KPluginInfo::List > m_pluginInfosByType;

        static const int s_pluginFrameworkVersion;
        static PluginManager *s_instance;

        PluginManager( QObject *parent = 0 );
};

} // namespace Plugins

namespace The
{
    inline Plugins::PluginManager *pluginManager() { return Plugins::PluginManager::instance(); }
}

#endif /* AMAROK_PLUGINMANAGER_H */
