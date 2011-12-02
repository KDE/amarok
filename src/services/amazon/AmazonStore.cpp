/****************************************************************************************
 * Copyright (c) 2011 Sven Krohlas <sven@getamarok.com>                                 *
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
#include "AmazonCart.h"
#include "AmazonConfig.h"
#include "AmazonMeta.h"
#include "AmazonParser.h"
#include "AmazonShoppingCartDialog.h"
#include "AmazonUrlRunner.h"

#include "amarokurls/AmarokUrlHandler.h"
#include "browsers/CollectionTreeItem.h"
#include "browsers/SingleCollectionTreeItemModel.h"
#include "core/interfaces/Logger.h"
#include "playlist/PlaylistController.h"
#include "widgets/SearchWidget.h"

#include <QDesktopServices>
#include <QDomDocument>
#include <QTemporaryFile>
#include <QToolBar>

#include <KAction>
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
    KPluginInfo pluginInfo( "amarok_service_amazonstore.desktop", "services" );
    pluginInfo.setConfig( config() );
    m_info = pluginInfo;
}

void
AmazonServiceFactory::init()
{
    DEBUG_BLOCK
    AmazonStore* service = new AmazonStore( this, "MP3 Music Store" );
    m_activeServices << service;
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
{
    DEBUG_BLOCK
    setObjectName( name );

    m_polished = false;
    m_isNavigation = false;

    setShortDescription( i18n( "Access the Amazon MP3 Store directly from Amarok" ) );
    setIcon( KIcon( "view-services-amazon-amarok" ) );

    // used in info applet
    setLongDescription( i18n( "This plugin allows searching and purchasing songs and albums from the Amazon MP3 store. Amarok gets a share of the profits made by this service." ) );

    setImagePath( KStandardDirs::locate( "data", "amarok/images/hover_info_amazon.png" ) );

    m_metaFactory = new AmazonMetaFactory( "amazon", this );
    m_collection = new Collections::AmazonCollection( this, "amazon", "MP3 Music Store" );
    polish();
    setPlayableTracks( true );
    m_serviceready = true;

    m_lastSearch = QString();

    // add the collection, exclude it from global queries
    CollectionManager::instance()->addUnmanagedCollection( m_collection, CollectionManager::CollectionDisabled );

    connect( m_searchWidget, SIGNAL( filterChanged( const QString ) ), this, SLOT( newSearchRequest( const QString ) ) );

    emit( ready() );
    newSearchRequest( QLatin1String( "" ) ); // to get some default content
}

AmazonStore::~AmazonStore()
{
    CollectionManager::instance()->removeUnmanagedCollection( m_collection );
    delete m_collection;
}

void
AmazonStore::polish()
{
    DEBUG_BLOCK;

    if( !m_polished ) {
        m_polished = true;

        initTopPanel();
        initView();

        connect( m_itemView, SIGNAL( itemSelected( QModelIndex ) ), this, SLOT( itemSelected( QModelIndex ) ) );
        connect( m_itemView, SIGNAL( itemDoubleClicked( QModelIndex ) ), this, SLOT( itemDoubleClicked( QModelIndex ) ) );
        connect( m_itemView, SIGNAL( searchForAlbum( QModelIndex ) ), this, SLOT( searchForAlbum( QModelIndex ) ) );

        AmazonUrlRunner *runner = new AmazonUrlRunner();
        connect( runner, SIGNAL( search( const QString ) ), this, SLOT( newSearchRequest( QString ) ) );
        The::amarokUrlHandler()->registerRunner( runner, runner->command() );
    }
}


/* public slots */

void
AmazonStore::addToCart()
{
    QString asin, name, price;
    int id = 0;

    // get item from collection
    if( m_itemModel->isAlbum( m_selectedIndex ) ) // album
    {
        Meta::AmazonAlbum* album;
        id = m_itemModel->idForIndex( m_selectedIndex );

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
        id = m_itemModel->idForIndex( m_selectedIndex );
        track = dynamic_cast<Meta::AmazonTrack*>( m_collection->trackById( id ).data() );

        if( !track )
            return;

        name = m_collection->artistById( track->artistId() )->name() + " - " + track->name();
        asin = track->asin();
        price = track->price();
    }

    AmazonCart::instance()->add( asin, price, name );
    Amarok::Components::logger()->shortMessage( i18n( "<em>%1</em> has been added to your shopping cart.", name ) );
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
    QUrl url = AmazonCart::instance()->checkoutUrl();
    debug() << url;
    m_checkoutButton->setEnabled( false );

    QTemporaryFile tempFile;
    tempFile.setAutoRemove( false );  // file must be removed later -> parser does it

    if( !tempFile.open() )
    {
        Amarok::Components::logger()->shortMessage( i18n( "Error: Unable to write temporary file. :-(" ) );
        m_checkoutButton->setEnabled( true );
        return;
    }

    KIO::FileCopyJob *requestJob = KIO::file_copy( url, KUrl( tempFile.fileName() ), 0700 , KIO::HideProgressInfo | KIO::Overwrite );

    connect( requestJob, SIGNAL( result( KJob * ) ), this, SLOT( openCheckoutUrl( KJob * ) ) );
    requestJob->start();
}

void
AmazonStore::itemDoubleClicked( QModelIndex index )
{
    // for albums: search for the album name to get details about it
    // for tracks: add it to the playlist

    int id = 0;

    if( m_itemModel->isAlbum( index ) ) // album
    {
        Meta::AmazonAlbum* album;
        id = m_itemModel->idForIndex( index );
        album = dynamic_cast<Meta::AmazonAlbum*>( m_collection->albumById( id ).data() );

        if( !album )
            return;

        QString name = m_collection->artistById( album->artistId() )->name() + " - " + album->name();
        m_searchWidget->setSearchString( name );
    }
    else // track
    {
        Meta::AmazonTrack* track;
        id = m_itemModel->idForIndex( index );
        track = dynamic_cast<Meta::AmazonTrack*>( m_collection->trackById( id ).data() );

        if( !track )
            return;

        Meta::TrackPtr trackPtr( track );

        The::playlistController()->instance()->insertOptioned( trackPtr, Playlist::Append );
    }
}

void
AmazonStore::itemSelected( QModelIndex index )
{
    m_addToCartButton->setEnabled( true );
    m_selectedIndex = index;
}

void
AmazonStore::newSearchRequest( const QString request )
{
    DEBUG_BLOCK

    // make sure we know where to search
    if( AmazonConfig::instance()->country().isEmpty() )
    {
        KCMultiDialog KCM;

        KCM.setWindowTitle( i18n( "Select your Amazon locale - Amarok" ) );
        KCM.addModule( KCModuleInfo( QString( "amarok_service_amazonstore_config.desktop" ) ) );
        KCM.setButtons( KCMultiDialog::Ok | KCMultiDialog::Cancel );
        KCM.resize( 400, 200 );

        // if the user selects a country we continue our quest for search results
        if( !(KCM.exec() == QDialog::Accepted) )
            return;
    }
    else if( AmazonConfig::instance()->country() == QLatin1String( "none" ) )
    {
        // user explicitly said we are in a not supported country
        Amarok::Components::logger()->longMessage( i18n( "<b>MP3 Music Store</b><br/><br/>Please select a valid country in the settings to make the store work." ) );
        return;
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
        Amarok::Components::logger()->shortMessage( i18n( "Error: Unable to write temporary file. :-(" ) );
        return;
    }

    m_searchWidget->searchStarted();
    KIO::FileCopyJob *requestJob = KIO::file_copy( requestUrl, KUrl( tempFile.fileName() ), 0700 , KIO::HideProgressInfo | KIO::Overwrite );

    connect( requestJob, SIGNAL( result( KJob * ) ), this, SLOT( parseReply( KJob * ) ) );
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

        QString name;

        // don't add the artist name for compilations
        if( !m_collection->albumById( track->albumId() )->isCompilation() )
            name = m_collection->artistById( track->artistId() )->name() + " - ";

        name = name + m_collection->albumById( track->albumId() )->name();

        m_searchWidget->setSearchString( name );
    }
}

/* private */

QUrl
AmazonStore::createRequestUrl( QString request )
{
    DEBUG_BLOCK
    QString urlString;
    QString pageValue;

    pageValue.setNum( m_resultpageSpinBox->value() );
    urlString += MP3_MUSIC_STORE_HOST;
    urlString += "apikey=";
    urlString += MP3_MUSIC_STORE_KEY;
    urlString += "&method=Search&Player=amarok&Location=";
    urlString += AmazonConfig::instance()->country();
    urlString += "&Text=";
    urlString += request.toUtf8().toBase64();
    urlString += "&Page=";
    urlString += pageValue;
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

    m_backwardAction = KStandardAction::back( this, SLOT( back() ), topPanel );
    m_forwardAction = KStandardAction::forward( this, SLOT( forward() ), topPanel );
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

    connect( m_resultpageSpinBox, SIGNAL( valueChanged( int ) ), this, SLOT( newSpinBoxSearchRequest( int ) ) );
}

void
AmazonStore::initView()
{
    m_itemView = new AmazonItemTreeView( this );
    m_itemModel = new AmazonItemTreeModel( m_collection );
    m_itemView->setParent( this );
    m_itemView->setRootIsDecorated( false ); // items cannot be expanded
    m_itemView->setUniformRowHeights( true ); // for perf reasons
    m_itemView->setModel( m_itemModel );

    KHBox* bottomPanelLayout = new KHBox;
    bottomPanelLayout->setParent( this );

    m_addToCartButton = new QPushButton;
    m_addToCartButton->setText( i18nc( "Add selected item to your shopping cart", "Add to Cart" ) );
    m_addToCartButton->setToolTip( i18n( "Add selected item to your shopping cart" ) );
    m_addToCartButton->setEnabled( false );
    m_addToCartButton->setObjectName( "addToCartButton" );
    m_addToCartButton->setParent( bottomPanelLayout );
    //m_downloadAlbumButton->setIcon( KIcon( "addToCart-amarok" ) );

    m_viewCartButton = new QPushButton;
    m_viewCartButton->setText( i18nc( "View your shopping cart contents", "View Cart" ) );
    m_viewCartButton->setToolTip( i18n( "View your shopping cart contents" ) );
    m_viewCartButton->setEnabled( true );
    m_viewCartButton->setObjectName( "viewCartButton" );
    m_viewCartButton->setParent( bottomPanelLayout );
    //m_downloadAlbumButton->setIcon( KIcon( "viewCartButton-amarok" ) );

    m_checkoutButton = new QPushButton;
    m_checkoutButton->setText( i18nc( "Checkout your shopping cart", "Checkout" ) );
    m_checkoutButton->setToolTip( i18n( "Checkout your shopping cart" ) );
    m_checkoutButton->setEnabled( false );
    m_checkoutButton->setObjectName( "checkoutButton" );
    m_checkoutButton->setParent( bottomPanelLayout );
    //m_downloadAlbumButton->setIcon( KIcon( "checkoutButton-amarok" ) );

    connect( m_addToCartButton, SIGNAL( clicked() ), this, SLOT( addToCart() ) );
    connect( m_itemView, SIGNAL( addToCart() ), this, SLOT( addToCart() ) );
    connect( m_viewCartButton, SIGNAL( clicked() ), this, SLOT( viewCart() ) );
    connect( m_checkoutButton, SIGNAL( clicked() ), this, SLOT( checkout() ) );
}

/* private slots */

void
AmazonStore::parseReply( KJob* requestJob )
{
    DEBUG_BLOCK
    if( requestJob->error() )
    {
        Amarok::Components::logger()->shortMessage( i18n( "Error: Querying MP3 Music Store database failed. :-(" ) );
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
    connect( parser, SIGNAL( done( ThreadWeaver::Job* ) ), this, SLOT( parsingDone( ThreadWeaver::Job* ) ) );
    connect( parser, SIGNAL( failed( ThreadWeaver::Job* ) ), this, SLOT( parsingFailed( ThreadWeaver::Job* ) ) );
    ThreadWeaver::Weaver::instance()->enqueue( parser );

    requestJob->deleteLater();
}

void
AmazonStore::parsingDone( ThreadWeaver::Job* parserJob )
{
    Q_UNUSED( parserJob )
    // model has been reset now, we no longer have a valid selection
    m_addToCartButton->setEnabled( false );
    m_searchWidget->searchEnded();
}

void
AmazonStore::parsingFailed( ThreadWeaver::Job* parserJob )
{
    Q_UNUSED( parserJob )
    Amarok::Components::logger()->shortMessage( i18n( "Error: Received an invalid reply. :-(" ) );
    m_searchWidget->searchEnded();
}

void
AmazonStore::openCheckoutUrl( KJob* requestJob )
{
    // very short document, we can parse it in the main thread
    QDomDocument responseDocument;

    QString tempFileName;
    KIO::FileCopyJob *job = dynamic_cast<KIO::FileCopyJob*>( requestJob );

    if( job )
        tempFileName = job->destUrl().toLocalFile();

    QFile responseFile( tempFileName );

    if( !responseFile.open( QIODevice::ReadOnly ) )
    {
        Amarok::Components::logger()->shortMessage( i18n( "Error: Unable to open temporary file. :-(" ) );

        m_checkoutButton->setEnabled( true );
        requestJob->deleteLater();
        QFile::remove( tempFileName );
        return;
    }

    QString errorMsg;
    int errorLine;
    int errorColumn;

    // verify it is valid
    if( !responseDocument.setContent( &responseFile, false, &errorMsg, &errorLine, &errorColumn ) ) // parse error
    {
        debug() << responseDocument.toString();
        debug() << "Parse ERROR";
        debug() << "Message:" << errorMsg;
        debug() << "Line:" << errorLine;
        debug() << "Column:" << errorColumn;
        Amarok::Components::logger()->shortMessage( i18n( "Error: Unable to parse temporary file. :-(" ) );

        m_checkoutButton->setEnabled( true );
        requestJob->deleteLater();
        QFile::remove( tempFileName );
        return;
    }

    debug() << responseDocument.toString();

    // now that's the whole parser for this reply:
    QUrl url;
    url.setEncodedUrl( responseDocument.documentElement().elementsByTagName( QLatin1String( "purchaseurl" ) ).at( 0 ).firstChild().nodeValue().toAscii() );

    qDebug() << url;
    QDesktopServices::openUrl( url );

    requestJob->deleteLater();
    QFile::remove( tempFileName );
    AmazonCart::instance()->clear();
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
