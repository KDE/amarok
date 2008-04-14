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


#include "scriptableservice/ScriptableServiceManager.h"
#include "ServiceBase.h"
#include "ServiceListModel.h"

#include <kvbox.h>
#include <QListView>
#include <QMap>


class ServiceListDelegate;

/**
A browser for selecting and displaying a service in the style of the first imbedded Magnatune store from a list of available services. Allows many services to be shown as a single tab.
Implemented as a singleton

@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class ServiceBrowser : public KVBox
{
    Q_OBJECT

public:

    static ServiceBrowser * instance();
    
    /**
     * Destructor
     */
    ~ServiceBrowser();

    QMap<QString, ServiceBase *> services();
    void removeService( const QString &name );
    void resetService( const QString &name );

    void showService( const QString &name );

public slots:

    void addService( ServiceBase * service );
    void setScriptableServiceManager( ScriptableServiceManager * scriptableServiceManager );

protected:

    virtual void paletteChange( const QPalette & oldPalette );

private:

    ServiceBrowser(QWidget * parent, const QString& name );

    static ServiceBrowser * s_instance;
            
    QListView * m_serviceListView;

    QMap<QString, ServiceBase *> m_services;
    ServiceBase * m_currentService;

    ScriptableServiceManager * m_scriptableServiceManager;
    bool m_usingContextView;
    ServiceListModel * m_serviceListModel;
    ServiceListDelegate * m_delegate;


private slots:

    void serviceActivated( const QModelIndex & index );
    void home();
};


#endif
