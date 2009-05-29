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

#include "Debug.h"
#include "ServiceListDelegate.h"
#include "context/ContextView.h"
#include "PaletteHandler.h"
#include "widgets/PrettyTreeView.h"
#include "widgets/SearchWidget.h"
#include "services/ServiceInfoProxy.h"

#include <KLineEdit>


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

    m_searchWidget = new SearchWidget( this, this, false );

    m_filterTimer.setSingleShot( true );
    connect( &m_filterTimer, SIGNAL( timeout() ), this, SLOT( slotFilterNow() ) );

    m_serviceListView = new Amarok::PrettyTreeView( this );
#ifdef Q_WS_MAC
    m_serviceListView->setVerticalScrollMode( QAbstractItemView::ScrollPerItem ); // for some bizarre reason w/ some styles on mac
    m_serviceListView->setHorizontalScrollMode( QAbstractItemView::ScrollPerItem ); // per-pixel scrolling is slower than per-item
#else
    m_serviceListView->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel ); // Scrolling per item is really not smooth and looks terrible
    m_serviceListView->setHorizontalScrollMode( QAbstractItemView::ScrollPerPixel ); // Scrolling per item is really not smooth and looks terrible
#endif
    
    m_serviceListView->setFrameShape( QFrame::NoFrame );

    m_proxyModel = new ServiceListSortFilterProxyModel( this );
    m_proxyModel->setSourceModel( m_serviceListModel );

    m_delegate = new ServiceListDelegate( m_serviceListView );
    m_serviceListView->setItemDelegate( m_delegate );
    m_serviceListView->setSelectionMode( QAbstractItemView::NoSelection );
    m_serviceListView->setHeaderHidden( true );
    m_serviceListView->setRootIsDecorated( false );
    m_serviceListView->setSortingEnabled( true );
    m_serviceListView->setAlternatingRowColors( true );
    m_serviceListView->setModel( m_proxyModel );
    connect( m_serviceListView, SIGNAL( activated( const QModelIndex & ) ), this, SLOT( serviceActivated( const QModelIndex & ) ) );
    m_scriptableServiceManager = 0;

    The::paletteHandler()->updateItemView( m_serviceListView );

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
    if( !service )
        return;

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

    if ( index.data( CustomServiceRoles::ServiceRole ).canConvert<ServiceBase *>() )
        service = index.data( CustomServiceRoles::ServiceRole ).value<ServiceBase *>();
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

    if ( service != 0 && service != m_currentService )
    {
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

    m_searchWidget->hide();
}

void
ServiceBrowser::home()
{
    if ( m_currentService != 0 )
    {
        m_currentService->setParent( 0 );
        m_serviceListView->setParent( this );
        m_currentService = 0; // remove any context stuff we might have added
        m_searchWidget->show();

        The::serviceInfoProxy()->loadHomePage();

        // Clear the search filter, preventing user confusion ("Where have my services gone?")
        if( !m_currentFilter.isEmpty() )
        {
            m_currentFilter.clear();
            slotFilterNow();
            m_searchWidget->lineEdit()->clear();
        }
    }
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

void ServiceBrowser::slotSetFilterTimeout()
{
    KLineEdit *lineEdit = dynamic_cast<KLineEdit*>( sender() );
    if( lineEdit )
    {
        m_currentFilter = lineEdit->text();
        m_filterTimer.stop();
        m_filterTimer.start( 500 );
    }
}

void ServiceBrowser::slotFilterNow()
{
    m_proxyModel->setFilterFixedString( m_currentFilter );
}

QString ServiceBrowser::activeServiceName()
{
    DEBUG_BLOCK
    if ( m_currentService )
        return m_currentService->name();
    return QString();
}

QString ServiceBrowser::activeServiceFilter()
{
    if ( m_currentService )
        return m_currentService->filter();
    return QString();
}

QList<int> ServiceBrowser::activeServiceLevels()
{
    if ( m_currentService )
        return m_currentService->levels();
    return QList<int>();
}

#include "ServiceBrowser.moc"



