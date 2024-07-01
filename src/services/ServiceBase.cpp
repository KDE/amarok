/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "ServiceBase.h"

#include "browsers/CollectionTreeItem.h"
#include "browsers/CollectionTreeItemModelBase.h"
#include "browsers/InfoProxy.h"
#include "core/collections/Collection.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "widgets/SearchWidget.h"
#include "widgets/BoxWidget.h"

#include <QLayout>
#include <QMenuBar>


ServiceFactory::ServiceFactory()
    : Plugins::PluginFactory()
{
    CollectionManager::instance()->addTrackProvider( this );
    connect( this, &ServiceFactory::newService, this, &ServiceFactory::slotNewService );
    connect( this, &ServiceFactory::removeService, this, &ServiceFactory::slotRemoveService );
}

ServiceFactory::~ServiceFactory()
{
    CollectionManager::instance()->removeTrackProvider( this );
}


Meta::TrackPtr
ServiceFactory::trackForUrl( const QUrl &url )
{
    if ( m_activeServices.isEmpty() ) {
        debug() << "our service (" << name() << ") is needed for a url, so init it!";
        init();
    }

    for( ServiceBase *service : m_activeServices )
    {
        if( !service->serviceReady() )
        {
            debug() << "our service is not ready! queuing track and returning proxy";
            MetaProxy::Track *ptrack = new MetaProxy::Track( url, MetaProxy::Track::ManualLookup );
            MetaProxy::TrackPtr trackptr( ptrack );
            m_tracksToLocate.enqueue( trackptr );
            return Meta::TrackPtr::staticCast( trackptr );
        }
        else if( service->collection() )
        {
            debug() << "Service Ready. Collection is: " << service->collection();
            return service->collection()->trackForUrl( url );
        }
        else
            warning() << __PRETTY_FUNCTION__ << "service is ready, but service->collection() is null!";
    }
    return Meta::TrackPtr();
}

void ServiceFactory::clearActiveServices()
{
    m_activeServices.clear();
}

void ServiceFactory::slotServiceReady()
{
    while( !m_tracksToLocate.isEmpty() )
    {
        MetaProxy::TrackPtr track = m_tracksToLocate.dequeue();
        if( !track )
            continue;

        track->lookupTrack( this );
    }
}

void
ServiceFactory::slotNewService( ServiceBase *newService )
{
    Q_ASSERT( newService );
    connect( newService, &ServiceBase::ready, this, &ServiceFactory::slotServiceReady );
    m_activeServices << newService;
}

void
ServiceFactory::slotRemoveService( ServiceBase *service )
{
    Q_ASSERT( service );
    m_activeServices.remove( service );
    service->deleteLater();
}

ServiceBase *ServiceBase::s_instance = nullptr;

ServiceBase::ServiceBase( const QString &name, ServiceFactory *parent, bool useCollectionTreeView, const QString &prettyName )
    : BrowserCategory( name, nullptr )
    , m_contentView ( nullptr )
    , m_parentFactory( parent )
    , m_polished( false )
    , m_useCollectionTreeView( useCollectionTreeView )
    , m_infoParser( nullptr )
    , m_serviceready( false )
    , m_model( nullptr )
    , m_filterModel( nullptr )
{
    DEBUG_BLOCK

    if ( !prettyName.isEmpty() )
    {
        setPrettyName( prettyName );
    }
    else
        setPrettyName( name );

    layout()->setSpacing( 1 );

    m_topPanel = new BoxWidget( true, this );

    if( useCollectionTreeView )
    {
        m_contentView = new ServiceCollectionTreeView( this );
        m_contentView->setFrameShape( QFrame::NoFrame );
        m_contentView->setSortingEnabled( true );
        m_contentView->sortByColumn ( 0, Qt::AscendingOrder );
        m_contentView->setDragEnabled ( true );
        m_contentView->setDragDropMode ( QAbstractItemView::DragOnly );
        connect( static_cast<ServiceCollectionTreeView*>( m_contentView ), &ServiceCollectionTreeView::itemSelected,
                 this, &ServiceBase::itemSelected );
    }

    m_bottomPanel = new BoxWidget( true, this );

    m_bottomPanel->setFrameStyle( QFrame::NoFrame );
    m_bottomPanel->setLineWidth(2);
    m_bottomPanel->layout()->setSpacing( 2 );
    m_bottomPanel->layout()->setContentsMargins( 2, 2, 2, 2 );

    m_filterModel = new QSortFilterProxyModel( this );
    m_filterModel->setSortCaseSensitivity( Qt::CaseInsensitive );
    m_filterModel->setFilterCaseSensitivity( Qt::CaseInsensitive );

    m_menubar = new QMenuBar( m_topPanel );
    // Make sure we do not expose this menubar outside to ensure it does not
    // replace the main menubar when Amarok is used with Plasma Menubar
    m_menubar->setNativeMenuBar( false );
    m_filterMenu = m_menubar->addMenu( i18n( "Group By" ) );

    m_menubar->hide();

    m_searchWidget = new SearchWidget( m_topPanel );
    if( m_contentView )
        connect( m_searchWidget, &SearchWidget::filterChanged,
                 static_cast<ServiceCollectionTreeView*>( m_contentView ), &ServiceCollectionTreeView::slotSetFilter );
}

ServiceBase::~ServiceBase()
{
    delete m_infoParser;
}

ServiceFactory*
ServiceBase::parent() const
{
    return m_parentFactory;
}

void
ServiceBase::itemActivated ( const QModelIndex & index )
{
    Q_UNUSED( index );
}


void
ServiceBase::setModel( QAbstractItemModel * model )
{
    if( m_contentView )
        m_contentView->setModel( model );
    m_model = model;
}

QAbstractItemModel *
ServiceBase::model()
{
    return m_model;
}

QTreeView *
ServiceBase::view()
{
    return m_contentView;
}

void
ServiceBase::setView( QTreeView * view )
{
    if( !view)
        return;
    m_contentView = view;
    if( m_model )
        m_contentView->setModel( m_model );
}

bool
ServiceBase::serviceReady() const
{
    return m_serviceready;
}

void
ServiceBase::setServiceReady( bool newReady )
{
    if( newReady == m_serviceready )
        return; // nothing to do

    m_serviceready = newReady;
    if( m_serviceready )
        Q_EMIT ready();
}

void
ServiceBase::infoChanged( const QString &infoHtml )
{
    QVariantMap map;
    map[QStringLiteral("service_name")] = prettyName();
    map[QStringLiteral("main_info")] = infoHtml;
    The::infoProxy()->setInfo( map );
}

void
ServiceBase::itemSelected( CollectionTreeItem * item )
{

    Meta::DataPtr ptr = item->data();
    if ( ( ptr.data() == nullptr ) || ( m_infoParser == nullptr )) return; 

    debug() << "selected item: " << ptr->name();

    ServiceDisplayInfoProvider * infoProvider = dynamic_cast<ServiceDisplayInfoProvider *>( ptr.data() );
    if (infoProvider == nullptr ) return; 

    infoProvider->processInfoOf( m_infoParser );
}

void
ServiceBase::generateWidgetInfo( const QString &html ) const
{
    QVariantMap map;
    map[QStringLiteral("service_name")] = prettyName();
    map[QStringLiteral("main_info")] = html;
    The::infoProxy()->setInfo( map );
}

void
ServiceBase::setPlayableTracks(bool playable)
{
    if( m_useCollectionTreeView ) {
        if( ServiceCollectionTreeView* view = dynamic_cast<ServiceCollectionTreeView*>(m_contentView) )
            view->setPlayableTracks( playable );
    }
}

void
ServiceBase::sortByArtist()
{
    setLevels( QList<CategoryId::CatMenuId>() << CategoryId::Artist );
}

void
ServiceBase::sortByArtistAlbum()
{
    setLevels( QList<CategoryId::CatMenuId>() << CategoryId::Artist << CategoryId::Album );
}

void
ServiceBase::sortByAlbum()
{
    setLevels( QList<CategoryId::CatMenuId>() << CategoryId::Album );
}

void
ServiceBase::sortByGenreArtist()
{
    setLevels( QList<CategoryId::CatMenuId>() << CategoryId::Genre << CategoryId::Artist );
}

void
ServiceBase::sortByGenreArtistAlbum()
{
    if( m_useCollectionTreeView ) {
        if( ServiceCollectionTreeView* view = dynamic_cast<ServiceCollectionTreeView*>(m_contentView) )
            view->setLevels( QList<CategoryId::CatMenuId>() << CategoryId::Genre << CategoryId::Artist << CategoryId::Album );
    }
}

void
ServiceBase::setFilter(const QString & filter)
{
    polish();
    m_searchWidget->setSearchString( filter );
}

void
ServiceBase::setInfoParser(InfoParserBase * infoParser)
{
    m_infoParser = infoParser;

    connect ( m_infoParser, &InfoParserBase::info, this, &ServiceBase::infoChanged );
}

InfoParserBase *
ServiceBase::infoParser()
{
    return m_infoParser;
}

QString
ServiceBase::messages()
{
    return i18n( "This service does not accept any messages" );
}

QString
ServiceBase::sendMessage( const QString & message )
{
    Q_UNUSED( message );
    return i18n( "ERROR: unknown message" );
}

QString
ServiceBase::filter() const
{
    return m_searchWidget->currentText();
}

QList<CategoryId::CatMenuId>
ServiceBase::levels() const
{
    CollectionTreeView *contentView = qobject_cast<CollectionTreeView*>(m_contentView);
    if( contentView )
        return contentView->levels();
    return QList<CategoryId::CatMenuId>();
}

void ServiceBase::setLevels( const QList<CategoryId::CatMenuId> &levels )
{
    if( m_useCollectionTreeView ) {
        if( ServiceCollectionTreeView* view = dynamic_cast<ServiceCollectionTreeView*>(m_contentView) )
            view->setLevels( levels );
    }
}



