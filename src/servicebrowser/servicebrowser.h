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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#ifndef AMAROKSERVICEBROWSER_H
#define AMAROKSERVICEBROWSER_H


//#include "scriptableservice/scriptableservicemanager.h"
#include "servicebase.h"

#include <klistwidget.h>
#include <kvbox.h>
#include <QMap>




/**
A browser for selecting and displaying a service in the style of the first imbedded Magnatune store from a list of available services. Allows many services to be shown as a single tab.
Implemented as a singleton

@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class ServiceBrowser : public KVBox
{
    Q_OBJECT

public:

     ServiceBrowser(QWidget * parent, const char *name );
    /**
     * Destructor
     */
    ~ServiceBrowser() { }


public slots:

    void addService( ServiceBase * service );
   // void setScriptableServiceManager( ScriptableServiceManager * scriptableServiceManager ); 

private:

    QListWidget * m_serviceSelectionList;
    
    void showService( const QString &name );

    QMap<QString, ServiceBase *> m_services;
    ServiceBase * m_currentService;

    //ScriptableServiceManager * m_scriptableServiceManager;
    bool m_usingContextView;


private slots:

    void serviceSelected( QListWidgetItem * item );
    void home();

};


#endif
