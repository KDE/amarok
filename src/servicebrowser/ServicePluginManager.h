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
 
#ifndef SERVICEPLUGINMANAGER_H
#define SERVICEPLUGINMANAGER_H

#include "servicebase.h"
#include "servicebrowser.h"

#include <QObject>

/**
A class to keep track of available service plugins and load them as needed

	@author 
*/
class ServicePluginManager : public QObject {
    Q_OBJECT
public:
    ServicePluginManager( ServiceBrowser * browser );

    ~ServicePluginManager();

    void init();

private:

    ServiceBrowser * m_serviceBrowser;
    
private slots:
    void slotNewService( ServiceBase *newService);

};

#endif
