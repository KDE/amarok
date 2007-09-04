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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/


#include "debug.h"
#include "servicebrowser.h"
#include "context/ContextView.h"
//#include "scriptableservicemanager.h"

#include <KIconLoader>

ServiceBrowser::ServiceBrowser(QWidget * parent, const QString& name )
    : KVBox( parent )
    , m_currentService( 0 )
    , m_usingContextView( false )
    , m_serviceListModel( new ServiceListModel() )
{
    setObjectName( name );

    debug() << "ServiceBrowser starting...";
    m_serviceSelectionList = new QListWidget( this );
    m_serviceSelectionList->setIconSize ( QSize(32, 32 ) );
    m_serviceSelectionList->setSpacing ( 4 );
    connect(m_serviceSelectionList, SIGNAL( itemDoubleClicked  ( QListWidgetItem *) ), this, SLOT( serviceSelected( QListWidgetItem *) ) );

    m_scriptableServiceManager = 0;
}

//TODO: Thsi should be moved to the ScriptableServiceManager instead
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

    m_serviceListModel->addService( service );

    connect( service, SIGNAL( home() ), this, SLOT( home() ) );
}


void ServiceBrowser::serviceSelected( QListWidgetItem * item )
{
    debug() << "Show service: " <<  item->text();
    showService(  item->text() );
}


void ServiceBrowser::showService( const QString &name )
{
    ServiceBase * service = 0;
    if ( m_services.contains( name ) )
       service = m_services.value( name );

    if ( service != 0 ) {

        m_serviceSelectionList->setParent ( 0 );
        m_serviceSelectionList->move ( QPoint( 0,0 ) );
        service->setParent ( this );
        service->move( QPoint( 0,0 ) );
        service->show();
        service->polish();
        m_usingContextView = service->updateContextView();
        m_currentService = service;
    }
}


void ServiceBrowser::home()
{
    if ( m_currentService != 0 ) {
        m_currentService->setParent( 0 );
        m_serviceSelectionList->setParent( this );
        m_serviceSelectionList->move( 0, 0 );
        m_serviceSelectionList->show();
        m_currentService = 0;
        // remove any context stuff we might have added

        // NOTE why does thic clear the CV?
        if ( m_usingContextView )
            Context::ContextView::self()->clear();
    }
}


#include "servicebrowser.moc"

