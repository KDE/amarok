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
#include "core/support/Debug.h"

#include "GpodderService.h"

#include "GpodderServiceView.h"
#include "GpodderServiceModel.h"
#include "GpodderPodcastTreeItem.h"
#include "GpodderSortFilterProxyModel.h"

#include "core/podcasts/PodcastProvider.h"
#include "playlistmanager/PlaylistManager.h"
#include "widgets/SearchWidget.h"

#include <KLocale>
#include <KPasswordDialog>
#include <KStandardDirs>
#include <KUrl>

#include <mygpo-qt/ApiRequest.h>
#include <mygpo-qt/Podcast.h>

AMAROK_EXPORT_SERVICE_PLUGIN( gpodder, GpodderServiceFactory )

GpodderServiceFactory::GpodderServiceFactory( QObject *parent, const QVariantList &args )
    : ServiceFactory( parent, args )
{
    KPluginInfo pluginInfo(  "amarok_service_gpodder.desktop", "services" );
    pluginInfo.setConfig( config() );
    m_info = pluginInfo;
}

void GpodderServiceFactory::init()
{
    ServiceBase *service = createGpodderService();
    if( service )
    {
        m_activeServices << service;
        m_initialized = true;
        emit newService( service );
    }
}

QString GpodderServiceFactory::name()
{
    return "gpodder.net";
}

KPluginInfo GpodderServiceFactory::info()
{
    KPluginInfo pluginInfo( "amarok_service_gpodder.desktop", "services" );
    pluginInfo.setConfig( config() );
    return pluginInfo;
}

KConfigGroup GpodderServiceFactory::config()
{
    return Amarok::config( "Service_gpodder" );
}

void GpodderServiceFactory::slotCreateGpodderService()
{
    if( !m_initialized )  // Until we can remove a service when networking gets disabled, only create it the first time.
    {
        ServiceBase *service = createGpodderService();
        if( service )
        {
            m_activeServices << service;
            m_initialized = true;
            emit newService( service );
        }
    }
}

void GpodderServiceFactory::slotRemoveGpodderService()
{
    if( m_activeServices.size() == 0 )
        return;

    m_initialized = false;
    emit removeService( m_activeServices.first() );
    m_activeServices.clear();
}

ServiceBase* GpodderServiceFactory::createGpodderService()
{
    ServiceBase* service = new GpodderService( this, QLatin1String( "gpodder" ) );
    return service;
}

GpodderService::GpodderService( GpodderServiceFactory* parent, const QString& name )
    : ServiceBase( name, parent, false ), m_inited( false )
{
    DEBUG_BLOCK
    setShortDescription( i18n( "gpodder.net: Podcast Directory Service" ) );
    setIcon( KIcon( "view-services-gpodder-amarok" ) );
    setLongDescription( i18n( "gpodder.net is an online Podcast Directory & Synchonisation Service." ) );
    setImagePath( KStandardDirs::locate( "data", "amarok/images/mygpo.png" ) );

    init();
}

GpodderService::~GpodderService()
{
    DEBUG_BLOCK

}

//This Method should only contain the most necessary things for initilazing the Service
void GpodderService::init()
{

    m_serviceready = true;

    m_inited = true;
}

//This Method should contain the rest of the Service Initialization (not soo necessary things, that can be done after the Object was created)
void GpodderService::polish()
{

    DEBUG_BLOCK

    generateWidgetInfo();
    if( m_polished )
        return;

    //do not allow this content to get added to the playlist. At least not for now
    setPlayableTracks( false );

    GpodderServiceView* view = new GpodderServiceView( this );
    view->setHeaderHidden( true );
    view->setFrameShape( QFrame::NoFrame );

    // Was set true in OpmlDirectoryService, but I think we won't need this on true
    view->setDragEnabled( false );
    view->setItemsExpandable( true );

    view->setSortingEnabled( false );
    view->setEditTriggers( QAbstractItemView::NoEditTriggers );
    view->setDragDropMode( QAbstractItemView::NoDragDrop );

    setView( view );

    GpodderServiceModel *sourceModel = new GpodderServiceModel( this );

    m_proxyModel = new GpodderSortFilterProxyModel( this );
    m_proxyModel->setDynamicSortFilter( true );

    m_proxyModel->setSourceModel( sourceModel );


    setModel( m_proxyModel );

    m_selectionModel = view->selectionModel();

    m_subscribeButton = new QPushButton();
    m_subscribeButton->setParent( m_bottomPanel );
    m_subscribeButton->setText( i18n( "Subscribe" ) );
    m_subscribeButton->setObjectName( "subscribeButton" );
    m_subscribeButton->setIcon( KIcon( "get-hot-new-stuff-amarok" ) );

    m_subscribeButton->setEnabled( true );

    connect( m_subscribeButton, SIGNAL( clicked() ), this, SLOT( subscribe() ) );




    m_searchWidget->setup( this );
    m_polished = true;
}

void GpodderService::itemSelected( CollectionTreeItem * selectedItem )
{
    Q_UNUSED( selectedItem )
    DEBUG_BLOCK
    return;
}

// Filter slots
void GpodderService::slotSetFilterTimeout()
{

    m_proxyModel->setFilterWildcard( m_searchWidget->currentText() );
}

void GpodderService::slotFilterNow()
{

}

void GpodderService::setFocus()
{

}

void GpodderService::subscribe()
{
    QModelIndex index = m_proxyModel->mapToSource( m_selectionModel->currentIndex() );
    GpodderTreeItem *treeItem = static_cast<GpodderTreeItem*>( index.internalPointer() );

    if( GpodderPodcastTreeItem *podcastTreeItem = qobject_cast<GpodderPodcastTreeItem*>( treeItem ) )
    {
        Podcasts::PodcastProvider *podcastProvider = The::playlistManager()->defaultPodcasts();
        KUrl kUrl( podcastTreeItem->podcast()->url() );
        podcastProvider->addPodcast( kUrl );
    }
}
