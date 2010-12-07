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

#ifndef AMAROK_PLUGINMANAGER_H
#define AMAROK_PLUGINMANAGER_H

#include <KService>

#include "shared/amarok_export.h"

class ServicePluginManager;
class ServiceFactory;

namespace Plugins {

class AMAROK_EXPORT PluginManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY( int pluginFrameworkVersion READ pluginFrameworkVersion )

    public:
        ~PluginManager();

        AMAROK_EXPORT static PluginManager *instance();
        AMAROK_EXPORT static void destroy();

        template<typename T>
            QList<T*> findPlugins( const QString &type );

        /**
         * Load any services that are configured to be loaded
         */
        void init();

        void checkPluginEnabledStates();

        QList<ServiceFactory*> serviceFactories() const;

        ServicePluginManager *servicePluginManager();

        /**
         * It will return a list of services that match your
         * specifications.  The only required parameter is the service
         * type.  This is something like 'text/plain' or 'text/html'.  The
         * constraint parameter is used to limit the possible choices
         * returned based on the constraints you give it.
         *
         * The @p constraint language is rather full.  The most common
         * keywords are AND, OR, NOT, IN, and EXIST, all used in an
         * almost spoken-word form.  An example is:
         * \code
         * (Type == 'Service') and (('KParts/ReadOnlyPart' in ServiceTypes) or (exist Exec))
         * \endcode
         *
         * The keys used in the query (Type, ServiceType, Exec) are all
         * fields found in the .desktop files.
         *
         * @param constraint  A constraint to limit the choices returned, QString::null to
         *                    get all services of the given @p servicetype
         *
         * @return            A list of services that satisfy the query
         * @see               http://developer.kde.org/documentation/library/kdeqt/tradersyntax.html
         */
        AMAROK_EXPORT static KService::List query( const QString& constraint = QString() );

        AMAROK_EXPORT static int pluginFrameworkVersion();

        /**
         * Dump properties from a service to stdout for debugging
         * @param service     Pointer to KService
         */
        AMAROK_EXPORT static void dump( const KService::Ptr service );

       /**
         * Show modal info dialog about plugin
         * @param constraint  A constraint to limit the choices returned
         */
        AMAROK_EXPORT static void showAbout( const QString& constraint );

    private:
        ServicePluginManager *m_servicePluginManager;
        QList<ServiceFactory*> m_serviceFactories;

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
