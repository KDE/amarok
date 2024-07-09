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

#include <KPluginMetaData>

#include <QVector>

namespace Plugins {

class PluginFactory;

class AMAROK_EXPORT PluginManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY( int pluginFrameworkVersion READ pluginFrameworkVersion )

    public:
        /** Type of the plugin.
         *
         *  Will be determined by the KPluginMetaData::category
         */
        enum Type
        {
            Collection = 1, ///< the plugin implements a CollectionFactory
            Service = 2,    ///< this is a service plugin
            Importer = 3,   ///< this plugin implements importer functionality
            Storage = 4,    ///< the plugin implements a StorageFactory
        };
        Q_ENUM( Type )

        ~PluginManager() override;

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

        /** Returns enabled plugin factories for the given plugin type.
         *
         *  This function will only return factories for enabled plugins.
         */
        QList<QSharedPointer<PluginFactory> > factories( Type type ) const;

        QVector<KPluginMetaData> plugins( Type type ) const;

        QVector<KPluginMetaData> enabledPlugins(Type type ) const;

        /** Check if any services were disabled and needs to be removed, or any
         *  that are hidden needs to be enabled
         *
         * This function will call the sub plugin managers (like CollectionManager)
         * setFactories function.
         */
        void checkPluginEnabledStates();

    private:
        /** Tries finding Amarok plugins */
        QVector<KPluginMetaData> findPlugins();

        /** Returns true if the plugin is enabled.
         *  This function will check the default enabled state,
         *  the Amarok configuration state and the primary collection.
         *
         *  @returns true if the plugin is enabled.
         */
        bool isPluginEnabled( const KPluginMetaData &plugin ) const;

        /** Creates a factories for a plugin */
        QSharedPointer<PluginFactory> createFactory( const KPluginMetaData &pluginInfo );

        /// contains the names of all KPluginMetaDatas that have factories created
        QVector<KPluginMetaData> m_plugins;
        QHash<Type, QList<KPluginMetaData> > m_pluginsByType;
        QHash<Type, QList<QSharedPointer<PluginFactory> > > m_factoriesByType;
        QHash<QString, QSharedPointer<PluginFactory>> m_factoryCreated;

        static const int s_pluginFrameworkVersion;
        static PluginManager *s_instance;

        explicit PluginManager( QObject *parent = nullptr );
};

} // namespace Plugins

namespace The
{
    inline Plugins::PluginManager *pluginManager() { return Plugins::PluginManager::instance(); }
}

#endif /* AMAROK_PLUGINMANAGER_H */
