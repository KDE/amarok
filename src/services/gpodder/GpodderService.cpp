/****************************************************************************************
 * Copyright (c) 2010 - 2011 Stefan Derkits <stefan@derkits.at>                         *
 * Copyright (c) 2010 - 2011 Christian Wagner <christian.wagner86@gmx.at>               *
 * Copyright (c) 2010 - 2011 Felix Winter <ixos01@gmail.com>                            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "GpodderService"

#include "GpodderService.h"

#include "core/podcasts/PodcastProvider.h"
#include "core/support/Debug.h"
#include "GpodderPodcastTreeItem.h"
#include "GpodderServiceConfig.h"
#include "GpodderServiceModel.h"
#include "GpodderServiceView.h"
#include "GpodderSortFilterProxyModel.h"
#include <mygpo-qt5/ApiRequest.h>
#include <mygpo-qt5/Podcast.h>
#include "playlistmanager/PlaylistManager.h"
#include "widgets/SearchWidget.h"

#include <QHostInfo>
#include <QStandardPaths>
#include <QUrl>


GpodderServiceFactory::GpodderServiceFactory()
    : ServiceFactory()
{}

GpodderServiceFactory::~GpodderServiceFactory()
{}

void
GpodderServiceFactory::init()
{
    if( m_initialized )
        return;

    ServiceBase *service = createGpodderService();
    if( service )
    {
        m_initialized = true;
        Q_EMIT newService( service );
    }
}

QString
GpodderServiceFactory::name()
{
    return QStringLiteral("gpodder.net");
}

KConfigGroup
GpodderServiceFactory::config()
{
    return Amarok::config( QLatin1String(GpodderServiceConfig::configSectionName()) );
}

void
GpodderServiceFactory::slotCreateGpodderService()
{
    //Until we can remove a service when networking gets disabled, only create it the first time.
    if( !m_initialized )
    {
        ServiceBase *service = createGpodderService();
        if( service )
        {
            m_initialized = true;
            Q_EMIT newService( service );
        }
    }
}

void
GpodderServiceFactory::slotRemoveGpodderService()
{
    if( activeServices().isEmpty() )
        return;

    m_initialized = false;
    Q_EMIT removeService( activeServices().first() );
}

ServiceBase *
GpodderServiceFactory::createGpodderService()
{
    ServiceBase *service = new GpodderService( this, QLatin1String( "gpodder" ) );
    return service;
}

GpodderService::GpodderService( GpodderServiceFactory *parent, const QString &name )
    : ServiceBase( name, parent, false )
    , m_inited( false )
    , m_apiRequest( nullptr )
    , m_podcastProvider( nullptr )
    , m_proxyModel( nullptr )
    , m_subscribeButton( nullptr )
    , m_selectionModel( nullptr )
{
    DEBUG_BLOCK

    setShortDescription( i18n( "gpodder.net: Podcast Directory Service" ) );
    setIcon( QIcon::fromTheme( QStringLiteral("view-services-gpodder-amarok") ) );
    setLongDescription(
                i18n( "gpodder.net is an online Podcast Directory & Synchonisation Service." ) );
    setImagePath( QStandardPaths::locate( QStandardPaths::GenericDataLocation, QStringLiteral("amarok/images/mygpo.png") ) );

    init();
}

GpodderService::~GpodderService()
{
    DEBUG_BLOCK

    if( m_podcastProvider )
    {
        //Remove the provider
        The::playlistManager()->removeProvider( m_podcastProvider );
        delete m_podcastProvider;
    }

    if ( m_apiRequest )
        delete m_apiRequest;
}

//This Method should only contain the most necessary things for initilazing the Service
void
GpodderService::init()
{
    DEBUG_BLOCK

    GpodderServiceConfig config;

    const QString &username = config.username();
    const QString &password = config.password();

    if ( m_apiRequest )
        delete m_apiRequest;

    //We have to check this here too, since KWallet::openWallet() doesn't
    //guarantee that it will always return a wallet.
    //Notice that LastFm service does the same verification.
    if ( !config.isDataLoaded() )
    {
        debug() << "Failed to read gpodder credentials.";
        m_apiRequest = new mygpo::ApiRequest( The::networkAccessManager() );
    }
    else
    {
        if( config.enableProvider() )
        {
            m_apiRequest = new mygpo::ApiRequest( username,
                                                  password,
                                                  The::networkAccessManager() );
            if( m_podcastProvider )
                delete m_podcastProvider;

            enableGpodderProvider( username );
        }
        else
            m_apiRequest = new mygpo::ApiRequest( The::networkAccessManager() );
    }

    setServiceReady( true );
    m_inited = true;
}

//This Method should contain the rest of the Service Initialization (not soo necessary things, that
//can be done after the Object was created)
void
GpodderService::polish()
{
    DEBUG_BLOCK

    generateWidgetInfo();

    if( m_polished )
        return;

    //do not allow this content to get added to the playlist. At least not for now
    setPlayableTracks( false );

    GpodderServiceView *view = new GpodderServiceView( this );
    view->setHeaderHidden( true );
    view->setFrameShape( QFrame::NoFrame );

    // Was set true in OpmlDirectoryService, but I think we won't need this on true
    view->setDragEnabled( false );
    view->setItemsExpandable( true );

    view->setSortingEnabled( false );
    view->setEditTriggers( QAbstractItemView::NoEditTriggers );
    view->setDragDropMode( QAbstractItemView::NoDragDrop );

    setView( view );

    GpodderServiceModel *sourceModel = new GpodderServiceModel( m_apiRequest, this );

    m_proxyModel = new GpodderSortFilterProxyModel( this );
    m_proxyModel->setDynamicSortFilter( true );
    m_proxyModel->setFilterCaseSensitivity( Qt::CaseInsensitive );

    m_proxyModel->setSourceModel( sourceModel );

    setModel( m_proxyModel );

    m_selectionModel = view->selectionModel();

    m_subscribeButton = new QPushButton();
    m_subscribeButton->setParent( m_bottomPanel );
    m_subscribeButton->setText( i18n( "Subscribe" ) );
    m_subscribeButton->setObjectName( QStringLiteral("subscribeButton") );
    m_subscribeButton->setIcon( QIcon::fromTheme( QStringLiteral("get-hot-new-stuff-amarok") ) );

    m_subscribeButton->setEnabled( true );

    connect( m_subscribeButton, &QPushButton::clicked, this, &GpodderService::subscribe );

    connect( m_searchWidget, &SearchWidget::filterChanged,
             m_proxyModel, &QSortFilterProxyModel::setFilterWildcard );

    m_polished = true;
}

void
GpodderService::itemSelected( CollectionTreeItem * selectedItem )
{
    Q_UNUSED( selectedItem )
    DEBUG_BLOCK
    return;
}

void
GpodderService::subscribe()
{
    QModelIndex index = m_proxyModel->mapToSource( m_selectionModel->currentIndex() );
    GpodderTreeItem *treeItem = static_cast<GpodderTreeItem*>( index.internalPointer() );

    if( GpodderPodcastTreeItem *podcastTreeItem = qobject_cast<GpodderPodcastTreeItem*>( treeItem ) )
    {
        Podcasts::PodcastProvider *podcastProvider = The::playlistManager()->defaultPodcasts();
        QUrl kUrl( podcastTreeItem->podcast()->url() );
        podcastProvider->addPodcast( kUrl );
    }
}

void
GpodderService::enableGpodderProvider( const QString &username )
{
    DEBUG_BLOCK

    QString deviceName = QLatin1String( "amarok-" ) % QHostInfo::localHostName();

    debug() << QStringLiteral( "Enabling GpodderProvider( Username: %1 - Device: %1 )" )
                        .arg( username )
                        .arg( deviceName );

    m_podcastProvider = new Podcasts::GpodderProvider( username, deviceName, m_apiRequest );

    //Add the gpodder's provider to the playlist manager
    The::playlistManager()->addProvider( m_podcastProvider, PlaylistManager::PodcastChannel );

}
