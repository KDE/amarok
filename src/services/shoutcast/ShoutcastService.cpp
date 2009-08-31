/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2007 Adam Pigg <adam@piggz.co.uk>                                      *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "ShoutcastService.h"

#include "Debug.h"
#include "Amarok.h"
#include "statusbar/StatusBar.h"

using namespace Meta;

AMAROK_EXPORT_PLUGIN( ShoutcastServiceFactory )


void ShoutcastServiceFactory::init()
{
    ServiceBase* service = new ShoutcastService( this, "Shoutcast.com", i18n( "Shoutcast Directory" ) );
    m_activeServices << service;
    m_initialized = true;
    connect( service, SIGNAL( ready() ), this, SLOT( slotServiceReady() ) );
    emit newService( service );
}

QString ShoutcastServiceFactory::name()
{
    return "Shoutcast.com";
}

KPluginInfo ShoutcastServiceFactory::info()
{
    KPluginInfo pluginInfo( "amarok_service_shoutcast.desktop", "services" );
    pluginInfo.setConfig( config() );
    return pluginInfo;
}

KConfigGroup ShoutcastServiceFactory::config()
{
    return Amarok::config( "Service_Shoutcast" );
}

ShoutcastService::ShoutcastService( ShoutcastServiceFactory* parent, const QString &name, const QString &prettyName )
    : ServiceBase( "Shoutcast Directory", parent, true, prettyName )
    , m_collection( 0 )
{
    setObjectName( name );
    setShortDescription( i18n( "The biggest list of online radio stations on the Internet" ) );
    setIcon( KIcon( "network-wireless" ) );
    m_serviceready = true;
    emit( ready() );
}

ShoutcastService::~ShoutcastService()
{}

void ShoutcastService::polish()
{
    DEBUG_BLOCK

    if ( m_polished )
        return;

    bottomPanelLayout = new KHBox;
    bottomPanelLayout->setParent( m_bottomPanel );

    m_top500ListButton = new QPushButton;
    m_top500ListButton->setParent( bottomPanelLayout );
    m_top500ListButton->setText( i18nc( "Fetch the 500 most popular stations", "View Top 500 Stations" ) );
    m_top500ListButton->setObjectName( "top500Button" );
    m_top500ListButton->setIcon( KIcon( "get-hot-new-stuff-amarok" ) );

    m_allListButton = new QPushButton;
    m_allListButton->setParent( bottomPanelLayout );
    m_allListButton->setText( i18nc( "Fetch list of all stations by genre", "View All Stations" ) );
    m_allListButton->setObjectName( "allButton" );
    m_allListButton->setIcon( KIcon( "get-hot-new-stuff-amarok" ) );

    connect( m_top500ListButton, SIGNAL( clicked() ), this, SLOT( top500ButtonClicked() ) );
    connect( m_allListButton, SIGNAL( clicked() ), this, SLOT( allButtonClicked() ) );
    m_polished = true;

    // Show complete list by default
    QTimer::singleShot( 0, m_allListButton, SLOT( click() ) );
}

void ShoutcastService::top500ButtonClicked()
{
    m_top500ListButton->setEnabled( false );
    m_allListButton->setEnabled( true );

    delete m_collection;

    m_collection = new ShoutcastServiceCollection(true); // Shoutcast service collection specifying top500 query
    QList<int> levels;
    setModel( new SingleCollectionTreeItemModel( m_collection, levels ) );
    view()->sortByColumn( 0, Qt::DescendingOrder );
}

void ShoutcastService::allButtonClicked()
{
    m_allListButton->setEnabled( false );
    m_top500ListButton->setEnabled( true );

    delete m_collection;

    m_collection = new ShoutcastServiceCollection();
    QList<int> levels;
    levels << CategoryId::Genre;
    setModel( new SingleCollectionTreeItemModel( m_collection, levels ) );
    view()->sortByColumn( 0, Qt::AscendingOrder );
}

#include "ShoutcastService.moc"

