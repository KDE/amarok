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

#ifndef AMAROKSERVICEBROWSER_H
#define AMAROKSERVICEBROWSER_H

#include "browsers/BrowserCategoryList.h"
#include "services/scriptable/ScriptableServiceManager.h"
#include "services/ServiceBase.h"

#include <QTimer>
#include <QVBoxLayout>


/**
 *  A browser for selecting and displaying a service in the style of the first
 *  embedded Magnatune store from a list of available services. Allows
 *  many services to be shown as a single category.
 *  Implemented as a singleton.
 *
 *  @author Nikolaj Hald Nielsen <nhn@kde.org>
 */
class ServiceBrowser : public BrowserCategoryList
{
    Q_OBJECT

    public:
        /**
         * Get the ServiceBrowser instance. Create it if it does not exist yet. ( Singleton pattern ).
         * @return The ServiceBrowser instance.
         */
        static ServiceBrowser *instance();

        /**
         * Destructor.
         */
        ~ServiceBrowser() override;

        /**
         * Reset a service and make it reload configuration. Not fully implemented..
         * @param name The name of the service to reset.
         */
        void resetService( const QString &name );

    public Q_SLOTS:

        /**
         * Set a scriptable service manager to handle scripted services.
         * @param scriptableServiceManager The scriptable service manager to set.
         */
        void setScriptableServiceManager( ScriptableServiceManager *scriptableServiceManager );

        void addService ( ServiceBase * );

    private:
        /**
         * Private constructor ( Singleton pattern )
         * @param parent The parent widget.
         * @param name The name of this widget.
         */
        ServiceBrowser( const QString& name, QWidget *parent = nullptr );

        static ServiceBrowser    *s_instance;

        ScriptableServiceManager *m_scriptableServiceManager;

        QTimer m_filterTimer;

        QString m_currentFilter;

};


#endif
