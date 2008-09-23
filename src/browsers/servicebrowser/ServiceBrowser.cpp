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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "ServiceBrowser.h"

#include "context/ContextView.h"
#include "Debug.h"
#include "ServiceListDelegate.h"

ServiceBrowser * ServiceBrowser::s_instance = 0;

ServiceBrowser * ServiceBrowser::instance()
{
    if ( s_instance == 0 )
        s_instance = new ServiceBrowser( 0, "Internet Content" );

    return s_instance;
}


ServiceBrowser::ServiceBrowser( QWidget * parent, const QString& name )
    : KVBox( parent )
    , m_currentService( 0 )
    , m_usingContextView( false )
    , m_serviceListModel( new ServiceListModel() )
{
    setObjectName( name );
    debug() << "ServiceBrowser starting...";
    m_serviceListView = new QListView( this );

    //make background transparent
    QPalette p = m_serviceListView->palette();
    QColor c = p.color( QPalette::Base );
    c.setAlpha( 0 );
    p.setColor( QPalette::Base, c );
    m_serviceListView->setPalette( p );

    m_serviceListView->setFrameShape( QFrame::NoFrame );

    m_delegate = new ServiceListDelegate( m_serviceListView );
    m_serviceListView->setItemDelegate( m_delegate );
    m_serviceListView->setModel( m_serviceListModel );
    connect(m_serviceListView, SIGNAL( clicked ( const QModelIndex & ) ), this, SLOT( serviceActivated( const QModelIndex & ) ) );
    m_scriptableServiceManager = 0;

    setFrameShape( QFrame::StyledPanel );
    setFrameShadow( QFrame::Sunken );
}


ServiceBrowser::~ServiceBrowser()
{
    DEBUG_BLOCK
    qDeleteAll( m_services.values() );
    delete m_serviceListView;
    delete m_serviceListModel;
    delete m_delegate;
}

//TODO: This should be moved to the ScriptableServiceManager instead
void
ServiceBrowser::setScriptableServiceManager( ScriptableServiceManager * scriptableServiceManager )
{
    m_scriptableServiceManager = scriptableServiceManager;
    m_scriptableServiceManager->setParent( this );
    connect ( m_scriptableServiceManager, SIGNAL( addService ( ServiceBase * ) ), this, SLOT( addService (  ServiceBase * ) ) );
}

void
ServiceBrowser::addService( ServiceBase * service )
{
    //insert service into service map
    m_services[service->name()] = service;
    m_serviceListModel->addService( service );
    connect( service, SIGNAL( home() ), this, SLOT( home() ) );
}

void
ServiceBrowser::serviceActivated( const QModelIndex & index )
{
    DEBUG_BLOCK
    ServiceBase * service = 0;

    if ( index.data( ServiceRole ).canConvert<ServiceBase *>() )
        service = index.data( ServiceRole ).value<ServiceBase *>();
    else
        return;

    if ( service )
    {
        debug() << "Show service: " <<  service->name();
        showService( service->name() );
    }
}

void
ServiceBrowser::showService( const QString &name )
{
    ServiceBase * service = 0;
    if ( m_services.contains( name ) )
       service = m_services.value( name );

    if ( service != 0 && service != m_currentService ) {
        //if a service is already shown, make damn sure to deactivate that one first...

        if ( m_currentService )
            m_currentService->setParent( 0 );

        m_serviceListView->setParent( 0 );
        service->setParent ( this );
        service->move( QPoint( 0, 0 ) );
        service->show();
        service->polish();
        m_usingContextView = service->updateContextView();
        m_currentService = service;
    }
}

void
ServiceBrowser::home()
{
    if ( m_currentService != 0 )
    {
        m_currentService->setParent( 0 );
        m_serviceListView->setParent( this );
        m_currentService = 0;
        // remove any context stuff we might have added

        // NOTE why does thic clear the CV?
        if ( m_usingContextView )
            Context::ContextView::self()->clear();
    }
}

void
ServiceBrowser::paletteChange( const QPalette & oldPalette )
{
    DEBUG_BLOCK
    Q_UNUSED( oldPalette );
    m_delegate->paletteChange();
    m_serviceListView->reset();
}


QMap< QString, ServiceBase * >
ServiceBrowser::services()
{
    return m_services;
}

void
ServiceBrowser::removeService( const QString &name )
{
    DEBUG_BLOCK
    debug() << "removing service: " << name;
    ServiceBase * service = m_services.take( name );
    if ( m_currentService == service )
        home();

    if( service )
        m_serviceListModel->removeService( service );
    delete service;
    m_serviceListView->reset();
}

void
ServiceBrowser::resetService( const QString &name )
{
    //What in the world is this for...

    //Currently unused, but needed, in the future, for resetting a service based on config changes
    //or the user choosing to reset the state of the service somehow.
    Q_UNUSED( name );
}

#include "ServiceBrowser.moc"

