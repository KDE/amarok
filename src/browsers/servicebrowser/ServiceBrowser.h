/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef AMAROKSERVICEBROWSER_H
#define AMAROKSERVICEBROWSER_H


#include "services/scriptable/ScriptableServiceManager.h"
#include "services/ServiceBase.h"
#include "ServiceListModel.h"

#include <KVBox>

#include <QTimer>
#include <QTreeView>
#include <QMap>
#include "ServiceListSortFilterProxyModel.h"

class ServiceListDelegate;
class SearchWidget;

/**
 *  A browser for selecting and displaying a service in the style of the first imbedded Magnatune store from a list of available services. Allows many services to be shown as a single tab.
 *  Implemented as a singleton.
 *
 *  @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
 */
class ServiceBrowser : public KVBox
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
        ~ServiceBrowser();

        /**
         * Get a map of the loaded services.
         * @return the map of services.
         */
        QMap<QString,ServiceBase*> services();

        /**
         * Remove a named service from the service browser.
         * @param name The name of the service to remove.
         */
        void removeService( const QString &name );

        /**
         * Reset a service and make it reload configuration. Not fully implemented..
         * @param name The name of the service to reset.
         */
        void resetService( const QString &name );
    
        /**
         * Make a service show. Hide any other active service if needed.
         * @param name the service to show.
         */
        void showService( const QString &name );

        QString activeServiceName();
        QString activeServiceFilter();
        QList<int> activeServiceLevels();

    public slots:
        /**
         * Add a service.
         * @param service The service to add.
         */
        void addService( ServiceBase *service );

        /**
         * Set a scriptable service manager to handle scripted services.
         * @param scriptableServiceManager The scriptable service manager to set.
         */
        void setScriptableServiceManager( ScriptableServiceManager *scriptableServiceManager );

        /**
         * Slot called when the active service should be hidden the service selection list shown again.
         */
        void home();

    private:
        /**
         * Private constructor ( Singleton pattern )
         * @param parent The parent widget.
         * @param name The name of this widget.
         */
        ServiceBrowser( QWidget *parent, const QString& name );

        static ServiceBrowser    *s_instance;

        SearchWidget             *m_searchWidget;

        QTreeView                *m_serviceListView;

        QMap<QString,ServiceBase*> m_services;
        ServiceBase              *m_currentService;

        ScriptableServiceManager *m_scriptableServiceManager;
        bool                      m_usingContextView;
        ServiceListModel         *m_serviceListModel;
        ServiceListSortFilterProxyModel* m_proxyModel;
        ServiceListDelegate      *m_delegate;

        QTimer m_filterTimer;

        QString m_currentFilter;

    private slots:
        /**
         * Slot called when an item in the service list has been activated and the corrosponding service should be shown.
         * @param index The index that was activated
         */
        void serviceActivated( const QModelIndex &index );

        void slotSetFilterTimeout();
        void slotFilterNow();

};


#endif
