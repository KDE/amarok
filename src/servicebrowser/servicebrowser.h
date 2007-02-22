/*
  Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#ifndef AMAROKSERVICEBROWSER_H
#define AMAROKSERVICEBROWSER_H

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
    /**
     * Destructor
     */
    ~ServiceBrowser() { }

    /**
     * Retrieves the class instance (Singleton pattern)
     * @return pointer to the class instance
     */
    static ServiceBrowser *instance() {
        if(!s_instance)  s_instance = new ServiceBrowser("ServiceBrowser");
        return s_instance;
    }

private:

    static ServiceBrowser *s_instance;


    KListWidget * m_serviceSelectionList;

    ServiceBrowser( const char *name );

    //QMap<ServiceBase> m_services;
    
   
};


#endif
