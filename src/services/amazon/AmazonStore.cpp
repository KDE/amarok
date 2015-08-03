/****************************************************************************************
 * Copyright (c) 2011, 2012 Sven Krohlas <sven@asbest-online.de>                        *
 * The Amazon store in based upon the Magnatune store in Amarok,                        *
 * Copyright (c) 2006,2007 Nikolaj Hald Nielsen <nhn@kde.org>                           *
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

#include "AmazonStore.h"

#include "Amazon.h"
#include "AmazonConfig.h"
#include "AmazonMeta.h"
#include "AmazonParser.h"
#include "AmazonShoppingCart.h"
#include "AmazonShoppingCartDialog.h"
#include "AmazonUrlRunner.h"
#include "AmazonWantCountryWidget.h"

#include "amarokurls/AmarokUrlHandler.h"
#include "browsers/CollectionTreeItem.h"
#include "browsers/SingleCollectionTreeItemModel.h"
#include "core/interfaces/Logger.h"
#include "core/support/Components.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "playlist/PlaylistController.h"
#include "widgets/SearchWidget.h"

#include <QDesktopServices>
#include <QDomDocument>
#include <QTemporaryFile>
#include <QToolBar>

#include <QAction>
#include "kio/jobclasses.h"
#include <KCMultiDialog>
#include <KStandardDirs>
#include <KToolBar>
#include "klocalizedstring.h"

AMAROK_EXPORT_SERVICE_PLUGIN( amazonstore, AmazonServiceFactory )

////////////////////////////////////////////////////////////////////////////////////////
// class AmazonServiceFactory
////////////////////////////////////////////////////////////////////////////////////////

AmazonServiceFactory::AmazonServiceFactory( QObject *parent, const QVariantList &args )
    : ServiceFactory( parent, args )
{
    KPluginInfo pluginInfo( "amarok_service_amazonstore.desktop" );
    pluginInfo.setConfig( config() );
    m_info = pluginInfo;
}

void
AmazonServiceFactory::init()
{
    DEBUG_BLOCK
    AmazonStore* service = new AmazonStore( this, "MP3 Music Store" );
    m_initialized = true;
    emit newService( service );
}

QString
AmazonServiceFactory::name()
{
    return "Amazon";
}

KConfigGroup
AmazonServiceFactory::config()
{
    return Amarok::config( "Service_Amazon" );
}


////////////////////////////////////////////////////////////////////////////////////////
// class AmazonStore
////////////////////////////////////////////////////////////////////////////////////////

// TODO: force country selection before first search, advanced search (albums/tracks only, further search result pages)

AmazonStore::AmazonStore( AmazonServiceFactory* parent, const char *name )
    : ServiceBase( name, parent, false )
    , m_wantCountryWidget(0)
{
    DEBUG_BLOCK
    setObjectName( name );

    m_polished = false;
    m_isNavigation = false;

    setShortDescription( i18n( "Access the Amazon MP3 Store directly from Amarok" ) );
    setIcon( QIcon::fromTheme( "view-services-amazon-amarok" ) );

    // used in info applet
    setLongDescription( i18n( "This plugin allows searching and purchasing songs and albums from the Amazon MP3 store. Amarok gets a share of the profits made by this service." ) );

    setImagePath( KStandardDirs::locate( "data", "amarok/images/hover_info_amazon.png" ) );

    m_metaFactory = new AmazonMetaFactory( "amazon" );
    m_collection = new Collections::AmazonCollection( this, "amazon", "MP3 Music Store" );
    polish();
    setPlayableTracks( true );

    m_lastSearch.clear();

    // add the collection, exclude it from global queries
    CollectionManager::instance()->addTrackProvider( m_collection );

    connect( m_searchWidget, SIGNAL(filterChanged(QString)), this, SLOT(newSearchRequest(QString)) );

    setServiceReady( true );
    newSearchRequest( QLatin1String( "" ) ); // to get some default content
}

AmazonStore::~AmazonStore()
{
    CollectionManager::instance()->removeTrackProvider( m_collection );
    delete m_collection;
}

void
AmazonStore::polish()
{
    DEBUG_BLOCK;

    if( !m_polished ) {
        m_polished = true;

        initTopPanel();
        initBottomPanel();
        initView();

        connect( m_itemView, SIGNAL(itemSelected(QModelIndex)), this, SLOT(itemSelected(QModelIndex)) );
        connect( m_itemView, SIGNAL(itemDoubleClicked(QModelIndex)), this, SLOT(itemDoubleClicked(QModelIndex)) );
        connect( m_itemView, SIGNAL(searchForAlbum(QModelIndex)), this, SLOT(searchForAlbum(QModelIndex)) );

        m_amazonInfoParser = new AmazonInfoParser();
        setInfoParser( m_amazonInfoParser );
        m_amazonInfoParser->showFrontPage();

        AmazonUrlRunner *runner = new AmazonUrlRunner();
        connect( runner, SIGNAL(search(QString)), this, SLOT(newSearchRequest(QString)) );
        The::amarokUrlHandler()->registerRunner( runner, runner->command() );
    }
}


/* public slots */

void
AmazonStore::addToCart()
{
    QString asin, name, price;
    int id = m_itemModel->idForIndex( m_selectedIndex );;

    // get item from collection
    if( m_itemModel->isAlbum( m_selectedIndex ) ) // album
    {
        Meta::AmazonAlbum* album;

        album = dynamic_cast<Meta::AmazonAlbum*>( m_collection->albumById( id ).data() );

        if( !album )
            return;

        name = m_collection->artistById( album->artistId() )->name() + " - " + album->name();
        asin = album->asin();
        price = album->price();
    }
    else // track
    {
        Meta::AmazonTrack* track;
        track = dynamic_cast<Meta::AmazonTrack*>( m_collection->trackById( id ).data() );

        if( !track )
            return;

        name = m_collection->artistById( track->artistId() )->name() + " - " + track->name();
        asin = track->asin();
        price = track->price();
    }

    AmazonShoppingCart::instance()->add( asin, price, name );
    m_checkoutButton->setEnabled( true );
}

void
AmazonStore::viewCart()
{
    AmazonShoppingCartDialog cartDialog( this, this );
    cartDialog.exec();
}

void
AmazonStore::checkout()
{
    QUrl url = AmazonShoppingCart::instance()->checkoutUrl();
    debug() << url;

    if( QDesktopServices::openUrl( url ) )
    {
        m_checkoutButton->setEnabled( false );
        AmazonShoppingCart::instance()->clear();
    }

    Amarok::Components::logger()->longMessage( i18n( "<b>MP3 Music Store</b><br/><br/>You are now being redirected to Amazon for the checkout process.<br/>To simplify that process please click <a href=\"%1\">this link</a> to tell Amazon that you have a downloader application for their MP3s installed.", Amazon::createCookieUrl().toString() ) );
}

void
AmazonStore::directCheckout()
{
    if( !m_selectedIndex.isValid() )
        return;

    // get item ASIN from collection
    int id = m_itemModel->idForIndex( m_selectedIndex );
    QString asin;
    Meta::AmazonItem* item;

    if( m_itemModel->isAlbum( m_selectedIndex ) ) // album
        item = dynamic_cast<Meta::AmazonItem*>( m_collection->albumById( id ).data() );
    else // track
        item = dynamic_cast<Meta::AmazonItem*>( m_collection->trackById( id ).data() );

    if( !item )
        return;

    asin = item->asin();

    // create and open direct checkout url
    QUrl url( AmazonShoppingCart::instance()->checkoutUrl( asin ) );
    QDesktopServices::openUrl( url );
}

void
AmazonStore::itemDoubleClicked( QModelIndex index )
{
    // for albums: search for the album ASIN to get details about it
    // for tracks: add it to the playlist

    int id = m_itemModel->idForIndex( index );

    if( m_itemModel->isAlbum( index ) ) // album
    {
        Meta::AmazonAlbum* album;
        album = dynamic_cast<Meta::AmazonAlbum*>( m_collection->albumById( id ).data() );

        if( !album )
            return;

        m_searchWidget->setSearchString( "asin:" + album->asin() );
    }
    else // track
    {
        Meta::AmazonTrack* track;
        track = dynamic_cast<Meta::AmazonTrack*>( m_collection->trackById( id ).data() );

        if( !track )
            return;

        Meta::TrackPtr trackPtr( track );

        The::playlistController()->instance()->insertOptioned( trackPtr, Playlist::OnDoubleClickOnSelectedItems );
    }
}

void
AmazonStore::itemSelected( QModelIndex index )
{
    m_addToCartButton->setEnabled( true );
    m_selectedIndex = index;

    int id = m_itemModel->idForIndex( index );
    Meta::AlbumPtr album;

    if( m_itemModel->isAlbum( index ) )
        album = m_collection->albumById( id ).data();
    else // track
        album = m_collection->trackById( id ).data()->album();

    m_amazonInfoParser->getInfo( album );
}

void
AmazonStore::newSearchRequest( const QString request )
{
    DEBUG_BLOCK

    if( AmazonConfig::instance()->country() == QLatin1String( "none" ) || AmazonConfig::instance()->country().isEmpty() )
    {
        // user explicitly said we are not in a supported country or refused to supply one
        if( m_itemView->isVisible() ) // show the message on startup only if the service is visible
            Amarok::Components::logger()->longMessage( i18n( "<b>MP3 Music Store</b><br/><br/>Please select a valid country in the settings to make the store work." ) );

        return; // do nothing
    }

    if( m_lastSearch != request )
    {
        // only add the request to the stack if it's a new one
        if( !m_isNavigation )
            m_backStack.push( m_lastSearch );

        // we start by showing the first result page
        m_lastSearch = request;
        m_resultpageSpinBox->setValue( 1 );
    }

    m_isNavigation = false;

    // update actions status
    m_backwardAction->setEnabled( !m_backStack.isEmpty() );
    m_forwardAction->setEnabled( !m_forwardStack.isEmpty() );

    // create request fetcher thread
    debug() << "Amazon: newSearchRequest: " << request;
    QUrl requestUrl = createRequestUrl( request );

    QTemporaryFile tempFile;
    tempFile.setAutoRemove( false );  // file must be removed later -> AmazonParser does it

    if( !tempFile.open() )
    {
        Amarok::Components::logger()->longMessage( i18n( "<b>MP3 Music Store</b><br/><br/>Error: Unable to write temporary file. :-(" ) );
        return;
    }

    m_searchWidget->searchStarted();
    KIO::FileCopyJob *requestJob = KIO::file_copy( requestUrl, QUrl( tempFile.fileName() ), 0700 , KIO::HideProgressInfo | KIO::Overwrite );

    connect( requestJob, SIGNAL(result(KJob*)), this, SLOT(parseReply(KJob*)) );
    requestJob->start();
}

void
AmazonStore::newSpinBoxSearchRequest( int i )
{
    Q_UNUSED( i )
    newSearchRequest( m_searchWidget->currentText() );
}

void
AmazonStore::searchForAlbum( QModelIndex index )
{
    // only being called for tracks to search for the album

    if( !m_itemModel->isAlbum( index ) ) // track
    {
        Meta::AmazonTrack* track;
        int id = m_itemModel->idForIndex( index );

        track = dynamic_cast<Meta::AmazonTrack*>( m_collection->trackById( id ).data() );

        if( !track )
            return;

        Meta::AmazonAlbum* album;
        album = dynamic_cast<Meta::AmazonAlbum*>( m_collection->albumById( track->albumId() ).data() );

        if( !album )
            return;

        m_searchWidget->setSearchString( "asin:" + album->asin() );
    }
}

/* private */

QUrl
AmazonStore::createRequestUrl( QString request )
{
    DEBUG_BLOCK
    QString urlString;
    QString pageValue;

    urlString += MP3_MUSIC_STORE_HOST;
    urlString += "/?apikey=";
    urlString += MP3_MUSIC_STORE_KEY;
    urlString += "&Player=amarok&Location=";
    urlString += AmazonConfig::instance()->country();

    if( request.startsWith( "asin:" ) ) // we need to load album details
    {
        urlString += "&method=LoadAlbum";
        urlString += "&ASIN=" + request.remove( "asin:" );
    }
    else // normal search
    {
        pageValue.setNum( m_resultpageSpinBox->value() );

        urlString += "&method=Search";
        urlString += "&Text=";
        urlString += request.toUtf8().toBase64();
        urlString += "&Page=";
        urlString += pageValue;
    }

    debug() << urlString;
    return QUrl( urlString );
}

void
AmazonStore::initTopPanel()
{
    KHBox *topPanel = new KHBox( m_topPanel );
    delete m_searchWidget;

    KToolBar *navigationToolbar = new KToolBar( topPanel );
    navigationToolbar->setToolButtonStyle( Qt::ToolButtonIconOnly );
    navigationToolbar->setIconDimensions( 16 );

    m_backwardAction = KStandardAction::back( this, SLOT(back()), topPanel );
    m_forwardAction = KStandardAction::forward( this, SLOT(forward()), topPanel );
    m_backwardAction->setEnabled( false );
    m_forwardAction->setEnabled( false );

    m_searchWidget = new SearchWidget( topPanel, false );
    m_searchWidget->setTimeout( 1500 );
    m_searchWidget->showAdvancedButton( false );

    m_resultpageSpinBox = new QSpinBox;
    m_resultpageSpinBox->setMinimum( 1 );
    m_resultpageSpinBox->setToolTip( i18n( "Select results page to show" ) );

    navigationToolbar->addAction( m_backwardAction );
    navigationToolbar->addAction( m_forwardAction );
    m_searchWidget->toolBar()->addWidget( m_resultpageSpinBox );

    connect( m_resultpageSpinBox, SIGNAL(valueChanged(int)), this, SLOT(newSpinBoxSearchRequest(int)) );
}

void
AmazonStore::initBottomPanel()
{
    QString country(AmazonConfig::instance()->country());
    if( country.isEmpty() || country == QLatin1String( "none" ) )
    {
        m_wantCountryWidget = new AmazonWantCountryWidget(m_bottomPanel);
        connect(m_wantCountryWidget, SIGNAL(countrySelected()),
                SLOT(countryUpdated()));
    }
}

void
AmazonStore::initView()
{
    m_itemView = new AmazonItemTreeView( this );
    m_itemModel = new AmazonItemTreeModel( m_collection );
    m_itemView->setParent( this );
    m_itemView->setRootIsDecorated( false ); // items cannot be expanded
    m_itemView->setUniformRowHeights( true ); // for perf reasons
    m_itemView->setFrameStyle( QFrame::NoFrame ); // no frame around the view, especially when selecting items
    m_itemView->setModel( m_itemModel );

    KHBox* bottomPanelLayout = new KHBox;
    bottomPanelLayout->setParent( this );

    m_addToCartButton = new QPushButton;
    m_addToCartButton->setText( i18nc( "Add selected item to your shopping cart", "Add to Cart" ) );
    m_addToCartButton->setToolTip( i18n( "Add selected item to your shopping cart" ) );
    m_addToCartButton->setEnabled( false );
    m_addToCartButton->setObjectName( "addToCartButton" );
    m_addToCartButton->setParent( bottomPanelLayout );
    m_addToCartButton->setIcon( QIcon::fromTheme( "amarok_cart_add" ) );

    m_viewCartButton = new QPushButton;
    m_viewCartButton->setText( i18nc( "View your shopping cart contents", "View Cart" ) );
    m_viewCartButton->setToolTip( i18n( "View your shopping cart contents" ) );
    m_viewCartButton->setEnabled( true );
    m_viewCartButton->setObjectName( "viewCartButton" );
    m_viewCartButton->setParent( bottomPanelLayout );
    m_viewCartButton->setIcon( QIcon::fromTheme( "amarok_cart_view" ) );

    m_checkoutButton = new QPushButton;
    m_checkoutButton->setText( i18nc( "Checkout your shopping cart", "Checkout" ) );
    m_checkoutButton->setToolTip( i18n( "Checkout your shopping cart" ) );
    m_checkoutButton->setEnabled( false );
    m_checkoutButton->setObjectName( "checkoutButton" );
    m_checkoutButton->setParent( bottomPanelLayout );
    m_checkoutButton->setIcon( QIcon::fromTheme( "download-amarok" ) );

    connect( m_addToCartButton, SIGNAL(clicked()), this, SLOT(addToCart()) );
    connect( m_itemView, SIGNAL(addToCart()), this, SLOT(addToCart()) );
    connect( m_itemView, SIGNAL(directCheckout()), this, SLOT(directCheckout()) );
    connect( m_viewCartButton, SIGNAL(clicked()), this, SLOT(viewCart()) );
    connect( m_checkoutButton, SIGNAL(clicked()), this, SLOT(checkout()) );
}

QString AmazonStore::iso3166toAmazon( const QString& country )
{
    static QHash<QString, QString> table;
    if( table.isEmpty() )
    {
        table["at"] = "de";
        table["ch"] = "de";
        table["de"] = "de";
        table["es"] = "es";
        table["fr"] = "fr";
        table["it"] = "it";
        table["jp"] = "co.jp";
        table["gb"] = "co.uk";
        table["us"] = "com";
    }

    return table.value( country, "none" );
}

/* private slots */

void
AmazonStore::parseReply( KJob* requestJob )
{
    DEBUG_BLOCK
    if( requestJob->error() )
    {
        Amarok::Components::logger()->longMessage( i18n( "<b>MP3 Music Store</b><br/><br/>Error: Querying MP3 Music Store database failed. :-(" ) );
        debug() << requestJob->errorString();
        requestJob->deleteLater();
        m_searchWidget->searchEnded();
        return;
    }

    QString tempFileName;
    KIO::FileCopyJob *job = dynamic_cast<KIO::FileCopyJob*>( requestJob );

    if( job )
        tempFileName = job->destUrl().toLocalFile();

    // create parser thread
    AmazonParser *parser = new AmazonParser( tempFileName, m_collection, m_metaFactory );
    connect( parser, SIGNAL(done(ThreadWeaver::JobPointer)), this, SLOT(parsingDone(ThreadWeaver::JobPointer)) );
    connect( parser, SIGNAL(failed(ThreadWeaver::JobPointer)), this, SLOT(parsingFailed(ThreadWeaver::JobPointer)) );
    ThreadWeaver::Queue::instance()->enqueue( QSharedPointer<ThreadWeaver::Job>(parser) );

    requestJob->deleteLater();
}

void
AmazonStore::parsingDone( ThreadWeaver::JobPointer parserJob )
{
    Q_UNUSED( parserJob )
    // model has been reset now, we no longer have a valid selection
    m_addToCartButton->setEnabled( false );
    m_searchWidget->searchEnded();
}

void
AmazonStore::parsingFailed( ThreadWeaver::JobPointer parserJob )
{
    Q_UNUSED( parserJob )
    Amarok::Components::logger()->longMessage( i18n( "<b>MP3 Music Store</b><br/><br/>Error: Received an invalid reply. :-(" ) );
    m_searchWidget->searchEnded();
}

void
AmazonStore::back()
{
    if( m_backStack.isEmpty() )
        return;

    QString request = m_backStack.pop();
    m_forwardStack.push( m_lastSearch );
    m_isNavigation = true;
    m_searchWidget->setSearchString( request );
}

void
AmazonStore::forward()
{
    if( m_forwardStack.isEmpty() )
        return;

    QString request = m_forwardStack.pop();
    m_backStack.push( m_lastSearch );
    m_isNavigation = true;
    m_searchWidget->setSearchString( request );
}

void
AmazonStore::countryUpdated()
{
    QString country( AmazonConfig::instance()->country() );
    if( country.isEmpty() || country == QLatin1String( "none" ) )
        return;

    if( m_wantCountryWidget )
    {
        m_wantCountryWidget->setParent( 0 );
        m_wantCountryWidget->deleteLater();
        m_wantCountryWidget = 0;
    }
    newSearchRequest( QString() );
}
