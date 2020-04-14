/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
 
#ifndef AMAROK_SERVICEPLUGINMANAGER_H
#define AMAROK_SERVICEPLUGINMANAGER_H

#include <QObject>
#include <QStringList>
#include <QSet>

class ServiceBase;
namespace Plugins {
    class PluginFactory;
}

/** A class to keep track of available service plugins and load them as needed
 *
 *  @author
 */
class ServicePluginManager : public QObject
{
    Q_OBJECT

public:

    static ServicePluginManager *instance();
    static void destroy();

    /**
     * Load any services that are configured to be loaded.
     * Unload any services that have been switched off.
     */
    void setFactories( const QList<QSharedPointer<Plugins::PluginFactory> > &factories );

public Q_SLOTS:
    QStringList loadedServices() const;
    QStringList loadedServiceNames() const;
    QString serviceDescription( const QString &service );
    QString serviceMessages( const QString &service );
    QString sendMessage( const QString &service, const QString &message );

private:
    static ServicePluginManager* s_instance;
    ServicePluginManager();
    ~ServicePluginManager() override;

    Q_DISABLE_COPY( ServicePluginManager )

    /** The list of currently set factories.
     *  Note: the PluginManager owns the pointers.
     */
    QList<QSharedPointer<Plugins::PluginFactory> > m_factories;

private Q_SLOTS:
    void slotNewService( ServiceBase *newService);
    void slotRemoveService( ServiceBase *removedService );
};


#endif //AMAROK_SERVICEPLUGINMANAGER_H

