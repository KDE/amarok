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

#include "ServiceBase.h"
#include "ServiceBrowser.h"

#include <QObject>

/**
A class to keep track of available service plugins and load them as needed

    @author
*/
class ServicePluginManager : public QObject
{

    Q_OBJECT

public:
    static ServicePluginManager * instance();

    ~ServicePluginManager();

    void setBrowser( ServiceBrowser * browser );

    /**
     * Collects the factories of all services that can be loaded
     */
    void collect();

    /**
     * Load any services that are configured to be loaded
     */
    void init();

    /**
     * The service settings has been changed... add, remove or reset any affected services
     */
    void settingsChanged();

    void settingsChanged( const QString &pluginName );

    void checkEnabledStates();

    QMap< QString, ServiceFactory* > factories();

public slots:

    QStringList loadedServices();
    QStringList loadedServiceNames();
    QString serviceDescription( const QString &service );
    QString serviceMessages( const QString &service );
    QString sendMessage( const QString &service, const QString &message );

private:
    ServicePluginManager();

    static ServicePluginManager * m_instance;
    ServiceBrowser * m_serviceBrowser;
    QMap< QString, ServiceFactory* > m_factories;
    QStringList m_loadedServices;

private slots:
    void slotNewService( ServiceBase *newService);
    void slotRemoveService( ServiceBase *removedService );
};


#endif //AMAROK_SERVICEPLUGINMANAGER_H

