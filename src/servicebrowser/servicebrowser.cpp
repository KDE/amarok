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
#include "magnatunestore/magnatunebrowser.h"

#include <kiconloader.h> 

ServiceBrowser::ServiceBrowser( const char *name )
        : KVBox( 0)
{
    

    debug() << "ServiceBrowser starting..." << endl;
    m_serviceSelectionList = new QListWidget( this );
    m_serviceSelectionList->setIconSize ( QSize(32, 32 ) );
    m_serviceSelectionList->setSpacing ( 4 );
    connect(m_serviceSelectionList, SIGNAL( itemDoubleClicked  ( QListWidgetItem *) ), this, SLOT( serviceSelected( QListWidgetItem *) ) );
    

    m_currentService = 0;
    m_scriptableServiceManager = 0;
}

void ServiceBrowser::setScriptableServiceManager( ScriptableServiceManager * scriptableServiceManager ) {
    m_scriptableServiceManager = scriptableServiceManager;
    m_scriptableServiceManager->setParent( this );
    connect ( m_scriptableServiceManager, SIGNAL( addService (  ServiceBase * ) ), this, SLOT( addService (  ServiceBase * ) ) );
}

void ServiceBrowser::addService( ServiceBase * service ) {

    //insert service into service map
    m_services[service->getName()] = service;

    //insert service name and image service selection list
    QListWidgetItem * serviceListItem = new QListWidgetItem( service->getName() , m_serviceSelectionList );
    //serviceListItem->setTextAlignment( Qt::AlignHCenter );
    serviceListItem->setIcon( service->getIcon() );
    

   

    connect( service, SIGNAL( home() ), this, SLOT( home() ) );
    

}


void ServiceBrowser::serviceSelected( QListWidgetItem * item ) {
    
    debug() << "Show service: " <<  item->text() << endl;
    showService(  item->text() );
}

void ServiceBrowser::showService( const QString &name ) {

    
    ServiceBase * service = 0;
    if ( m_services.contains( name ) )
       service = m_services.value( name );

    if ( service != 0 ) {

        m_serviceSelectionList->reparent ( 0,  QPoint( 0,0 ) );
        service->reparent ( this,  QPoint( 0,0 ), true );
        m_currentService = service;

    }


}

void ServiceBrowser::home()
{

    if ( m_currentService != 0 ) {
        m_currentService->reparent( 0,  QPoint( 0, 0 ) );
        m_serviceSelectionList->reparent( this,  QPoint( 0,0 ), true );
        m_currentService = 0;
    }

}




#include "servicebrowser.moc"
