/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#include "Amarok.h"

#include "Debug.h"

#include "Collection.h"
#include "SearchWidget.h"
#include "browsers/InfoProxy.h"

#include <khbox.h>
#include <KLineEdit>
#include <KMenuBar>

#include <QFrame>
#include <QLabel>


ServiceFactory::ServiceFactory() : m_initialized( false )
{
    CollectionManager::instance()->addTrackProvider( this );
}

ServiceFactory::~ServiceFactory()
{
    CollectionManager::instance()->removeTrackProvider( this );
}


Meta::TrackPtr
ServiceFactory::trackForUrl(const KUrl & url)
{
    if ( m_activeServices.size() == 0 ) {
        debug() << "our service (" << name() << ") is needed for a url, so init it!";
        init();
    }

    /*Meta::ServiceTrack * serviceTrack = new Meta::ServiceTrack();
    serviceTrack->setUidUrl( url.url() );
    Meta::TrackPtr track( serviceTrack );*/

    Meta::TrackPtr track;
    foreach( ServiceBase * service, m_activeServices )
    {
        if( !service->serviceReady() ){
            debug() << "our service is not ready! queuing track and returning proxy";
            MetaProxy::Track* ptrack = new MetaProxy::Track( url.url(), true );
            MetaProxy::TrackPtr trackptr( ptrack );
            m_tracksToLocate.enqueue( trackptr );
            return Meta::TrackPtr::staticCast( trackptr );
        } else if (  service->collection() ) {
            debug() << "Service Ready. Collection is: " << service->collection();
            track = service->collection()->trackForUrl( url );
        }

        if ( track )
            return track;
    }
    return track;
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
        if( track ) 
            track->lookupTrack( this );
    }
}

ServiceBase *ServiceBase::s_instance = 0;

ServiceBase::ServiceBase( const QString &name, ServiceFactory *parent, bool useCollectionTreeView, const QString &prettyName )
    : BrowserCategory( name, 0 )
    , m_contentView ( 0 )
    , m_parentFactory( parent )
    , m_polished( false )
    , m_serviceready( false )
    , m_useCollectionTreeView( useCollectionTreeView )
    , m_infoParser( 0 )
    , m_model( 0 )
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
        connect( m_contentView, SIGNAL( itemSelected ( CollectionTreeItem * )  ), this, SLOT( itemSelected( CollectionTreeItem * ) ) );
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
    m_filterMenu = m_menubar->addMenu( i18n( "Group By" ) );

    m_menubar->hide();

    m_searchWidget = new SearchWidget( m_topPanel );
    if ( m_contentView )
        m_searchWidget->setup( m_contentView );

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
    //m_filterModel->setSourceModel( model );
    //m_contentView->setModel( m_filterModel );
    m_contentView->setModel( model );
    m_model  = model;
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
    setLevels( QList<int>() << CategoryId::Artist );
}

void
ServiceBase::sortByArtistAlbum()
{
    setLevels( QList<int>() << CategoryId::Artist << CategoryId::Album );
}

void
ServiceBase::sortByAlbum()
{
    setLevels( QList<int>() << CategoryId::Album );
}

void
ServiceBase::sortByGenreArtist()
{
    setLevels( QList<int>() << CategoryId::Genre << CategoryId::Artist );
}

void
ServiceBase::sortByGenreArtistAlbum()
{
    if( m_useCollectionTreeView ) {
        if( ServiceCollectionTreeView* view = dynamic_cast<ServiceCollectionTreeView*>(m_contentView) )
            view->setLevels( QList<int>() << CategoryId::Genre << CategoryId::Artist << CategoryId::Album );
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

    connect ( m_infoParser, SIGNAL( info( QString) ), this, SLOT( infoChanged( QString ) ) );
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
    return m_searchWidget->lineEdit()->text();
}

QList<int>
ServiceBase::levels() const
{
    CollectionTreeView *contentView = qobject_cast<CollectionTreeView*>(m_contentView);
    if( contentView )
        return contentView->levels();
    return QList<int>();
}

void ServiceBase::setLevels( const QList<int> &levels )
{
    if( m_useCollectionTreeView ) {
        if( ServiceCollectionTreeView* view = dynamic_cast<ServiceCollectionTreeView*>(m_contentView) )
            view->setLevels( levels );
    }
}


#include "ServiceBase.moc"



