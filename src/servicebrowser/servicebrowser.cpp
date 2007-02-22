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


#include "debug.h"
#include "servicebrowser.h"

ServiceBrowser *ServiceBrowser::s_instance = 0;

ServiceBrowser::ServiceBrowser( const char *name )
        : KVBox( 0)
{

    m_serviceSelectionList = new KListWidget(this);


    ServiceBase * testService1 = new ServiceBase( "Dummy service 1" );
    ServiceBase * testService2 = new ServiceBase( "Dummy service 2" );
    ServiceBase * testService3 = new ServiceBase( "Dummy service 3" );

    addService( testService1 );
    addService( testService2 );
    addService( testService3 );

    showService( "Dummy service 3" );
}



void ServiceBrowser::addService( ServiceBase * service ) {

    //insert service into service map
    m_services[service->getName()] = service;

    //insert service name and image service selection list
    new QListWidgetItem( service->getName(), m_serviceSelectionList );
    

}

void ServiceBrowser::showService( QString name ) {

    
    ServiceBase * service = 0;
    if ( m_services.contains( name ) )
       service = m_services.value( name );

    if ( service != 0 ) {

        m_serviceSelectionList->reparent ( 0,  QPoint( 0,0 ) );
        service->reparent ( this,  QPoint( 0,0 ), true );

    }


}


#include "servicebrowser.moc"
