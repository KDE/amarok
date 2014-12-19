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

#include <KHBox>
#include <KMenuBar>

#include <QFrame>
#include <QLabel>


ServiceFactory::ServiceFactory( QObject *parent, const QVariantList &args )
    : Plugins::PluginFactory( parent, args )
{
    CollectionManager::instance()->addTrackProvider( this );
    connect( this, SIGNAL(newService(ServiceBase*)), SLOT(slotNewService(ServiceBase*)) );
    connect( this, SIGNAL(removeService(ServiceBase*)), SLOT(slotRemoveService(ServiceBase*)) );
}

ServiceFactory::~ServiceFactory()
{
    CollectionManager::instance()->removeTrackProvider( this );
}


Meta::TrackPtr
ServiceFactory::trackForUrl( const KUrl &url )
{
    if ( m_activeServices.size() == 0 ) {
        debug() << "our service (" << name() << ") is needed for a url, so init it!";
        init();
    }

    foreach( ServiceBase *service, m_activeServices )
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
    connect( newService, SIGNAL(ready()), this, SLOT(slotServiceReady()) );
    m_activeServices << newService;
}

void
ServiceFactory::slotRemoveService( ServiceBase *service )
{
    Q_ASSERT( service );
    m_activeServices.remove( service );
    service->deleteLater();
}

ServiceBase *ServiceBase::s_instance = 0;

ServiceBase::ServiceBase( const QString &name, ServiceFactory *parent, bool useCollectionTreeView, const QString &prettyName )
    : BrowserCategory( name, 0 )
    , m_contentView ( 0 )
    , m_parentFactory( parent )
    , m_polished( false )
    , m_useCollectionTreeView( useCollectionTreeView )
    , m_infoParser( 0 )
    , m_serviceready( false )
    , m_model( 0 )
    , m_filterModel( 0 )
{
    DEBUG_BLOCK

    if ( !prettyName.isEmpty() )
    {
        setPrettyName( prettyName );
    }
    else
        setPrettyName( name );

    setSpacing( 1 );

    m_topPanel = new KVBox( this );

    if( useCollectionTreeView )
    {
        m_contentView = new ServiceCollectionTreeView( this );
        m_contentView->setFrameShape( QFrame::NoFrame );
        m_contentView->setSortingEnabled( true );
        m_contentView->sortByColumn ( 0, Qt::AscendingOrder );
        m_contentView->setDragEnabled ( true );
        m_contentView->setDragDropMode ( QAbstractItemView::DragOnly );
        connect( m_contentView, SIGNAL(itemSelected(CollectionTreeItem*)), this, SLOT(itemSelected(CollectionTreeItem*)) );
    }

    m_bottomPanel = new KVBox( this );

    m_bottomPanel->setFrameStyle( QFrame::NoFrame );
    m_bottomPanel->setLineWidth(2);
    m_bottomPanel->setSpacing( 2 );
    m_bottomPanel->setMargin( 2 );

    m_filterModel = new QSortFilterProxyModel( this );
    m_filterModel->setSortCaseSensitivity( Qt::CaseInsensitive );
    m_filterModel->setFilterCaseSensitivity( Qt::CaseInsensitive );

    m_menubar = new KMenuBar( m_topPanel );
    // Make sure we do not expose this menubar outside to ensure it does not
    // replace the main menubar when Amarok is used with Plasma Menubar
    m_menubar->setNativeMenuBar( false );
    m_filterMenu = m_menubar->addMenu( i18n( "Group By" ) );

    m_menubar->hide();

    m_searchWidget = new SearchWidget( m_topPanel );
    if( m_contentView )
        connect( m_searchWidget, SIGNAL(filterChanged(QString)),
                 m_contentView, SLOT(slotSetFilter(QString)) );
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
        emit ready();
}

void
ServiceBase::infoChanged( const QString &infoHtml )
{
    QVariantMap map;
    map["service_name"] = prettyName();
    map["main_info"] = infoHtml;
    The::infoProxy()->setInfo( map );
}

void
ServiceBase::itemSelected( CollectionTreeItem * item )
{

    Meta::DataPtr ptr = item->data();
    if ( ( ptr.data() == 0 ) || ( m_infoParser == 0 )) return; 

    debug() << "selected item: " << ptr.data()->name();

    ServiceDisplayInfoProvider * infoProvider = dynamic_cast<ServiceDisplayInfoProvider *>( ptr.data() );
    if (infoProvider == 0 ) return; 

    infoProvider->processInfoOf( m_infoParser );
}

void
ServiceBase::generateWidgetInfo( const QString &html ) const
{
    QVariantMap map;
    map["service_name"] = prettyName();
    map["main_info"] = html;
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

    connect ( m_infoParser, SIGNAL(info(QString)), this, SLOT(infoChanged(QString)) );
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


#include "ServiceBase.moc"

