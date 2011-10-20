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

#include "Amazon.h"
#include "AmazonCart.h"
#include "AmazonConfig.h"
#include "AmazonMeta.h"
#include "AmazonParser.h"
#include "AmazonStore.h"

#include "browsers/CollectionTreeItem.h"
#include "browsers/SingleCollectionTreeItemModel.h"
#include "core/interfaces/Logger.h"
#include "widgets/SearchWidget.h"

#include <QDesktopServices>
#include <QDomDocument>
#include <QTemporaryFile>
#include <QToolBar>

#include <KCMultiDialog>
#include <KStandardDirs>
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

void AmazonServiceFactory::init()
{
    DEBUG_BLOCK
    AmazonStore* service = new AmazonStore( this, "MP3 Music Store" );
    m_activeServices << service;
    m_initialized = true;
    emit newService( service );
}

QString AmazonServiceFactory::name()
{
    return "Amazon";
}

KConfigGroup AmazonServiceFactory::config()
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
    setShortDescription( i18n( "Access the Amazon MP3 Store directly from Amarok" ) );
    setIcon( KIcon( "view-services-amazon-amarok" ) );

    // used in Info applet
    setLongDescription( i18n( "This plugin allows searching and purchasing songs and albums from the Amazon MP3 store. Amarok gets a share of the profits made by this service." ) );

    setImagePath( KStandardDirs::locate( "data", "amarok/images/hover_info_amazon.png" ) );

    m_metaFactory = new AmazonMetaFactory( "amazon", this );
    m_collection = new Collections::AmazonCollection( this, "amazon", "MP3 Music Store" );
    polish();
    setPlayableTracks( true );
    m_serviceready = true;

    // add the collection, exclude it from global queries
    CollectionManager::instance()->addUnmanagedCollection( m_collection, CollectionManager::CollectionDisabled );

    connect( m_searchWidget, SIGNAL( filterChanged( const QString ) ), this, SLOT( newSearchRequest( const QString ) ) );

    emit( ready() );
}

AmazonStore::~AmazonStore()
{
    CollectionManager::instance()->removeUnmanagedCollection( m_collection );
    delete m_collection;
}

void AmazonStore::polish()
{
    DEBUG_BLOCK;

    if( !m_polished ) {
        m_polished = true;

        initTopPanel();
        initView();

        connect( m_itemView, SIGNAL( itemSelected( QModelIndex ) ), this, SLOT( itemSelected( QModelIndex ) ) );

        // TODO for later versions:
        // add the Amazon URL runner
        // AmazonRunner * runner = new AmazonUrlRunner();
        // connect URL runner signals to AmazonStore slots
    }
}


void AmazonStore::initTopPanel()
{
    m_searchWidget->setTimeout( 3000 );
    m_searchWidget->showAdvancedButton( false );

    m_searchSelectionBox = new QComboBox;
    m_searchSelectionBox->setToolTip( i18n( "Select kind of items to search for" ) );
    m_searchSelectionBox->insertItem( 0, i18nc( "search for all types of items in the Amazon store", "all items" ) );
    m_searchSelectionBox->insertItem( 1, i18n( "albums" ) );
    m_searchSelectionBox->insertItem( 2, i18n( "songs" ) );

    m_searchWidget->toolBar()->addWidget( m_searchSelectionBox );

    m_resultpageSpinBox = new QSpinBox;
    m_resultpageSpinBox->setMinimum( 1 );
    m_resultpageSpinBox->setToolTip( i18n( "Select results page to show" ) );

    m_searchWidget->toolBar()->addWidget( m_resultpageSpinBox );

    m_searchWidget->toolBar()->addSeparator();

    connect( m_resultpageSpinBox, SIGNAL( valueChanged( int i ) ), this, SLOT( newSpinBoxSearchRequest( int i ) ) );
    // TODO: Action to open the config dialog
}

void AmazonStore::initView()
{
    m_itemView = new AmazonItemTreeView;
    m_itemModel = new AmazonItemTreeModel( m_collection );
    m_itemView->setParent( this );
    m_itemView->setRootIsDecorated( false ); // items cannot be expanded
    m_itemView->setUniformRowHeights( true ); // for perf reasons
    m_itemView->setModel( m_itemModel );

    KHBox* bottomPanelLayout = new KHBox;
    bottomPanelLayout->setParent( this );

    m_addToCartButton = new QPushButton;
    m_addToCartButton->setText( i18nc( "Add selected item to your shopping cart", "Add" ) );
    m_addToCartButton->setToolTip( i18n( "Add selected item to your shopping cart" ) );
    m_addToCartButton->setEnabled( false );
    m_addToCartButton->setObjectName( "addToCartButton" );
    m_addToCartButton->setParent( bottomPanelLayout );
    //m_downloadAlbumButton->setIcon( KIcon( "addToCart-amarok" ) );

    // TODO move this to the shopping cart gui. it makes no sense in the browser, as the to-be-removed tracks might no longer show up there
//    m_removeFromCartButton = new QPushButton;
//    m_removeFromCartButton->setText( i18nc( "Remove selected item from your shopping cart", "Remove" ) );
//    m_removeFromCartButton->setToolTip( i18n( "Remove selected item from your shopping cart" ) );
//    m_removeFromCartButton->setEnabled( false );
//    m_removeFromCartButton->setObjectName( "removeFromCartButton" );
//    m_removeFromCartButton->setParent( bottomPanelLayout );
//    //m_downloadAlbumButton->setIcon( KIcon( "removeFromCartButton-amarok" ) );

    m_viewCartButton = new QPushButton;
    m_viewCartButton->setText( i18nc( "View your shopping cart contents", "View Cart" ) );
    m_viewCartButton->setToolTip( i18n( "View your shopping cart contents" ) );
    m_viewCartButton->setEnabled( true );
    m_viewCartButton->setObjectName( "viewCartButtonButton" );
    m_viewCartButton->setParent( bottomPanelLayout );
    //m_downloadAlbumButton->setIcon( KIcon( "viewCartButton-amarok" ) );

    m_checkoutButton = new QPushButton;
    m_checkoutButton->setText( i18nc( "Checkout your shopping cart", "Checkout" ) );
    m_checkoutButton->setToolTip( i18n( "Checkout your shopping cart" ) );
    m_checkoutButton->setEnabled( false );
    m_checkoutButton->setObjectName( "checkoutButtonButton" );
    m_checkoutButton->setParent( bottomPanelLayout );
    //m_downloadAlbumButton->setIcon( KIcon( "checkoutButton-amarok" ) );

    connect( m_addToCartButton, SIGNAL( clicked() ), this, SLOT( addToCart() ) );
    connect( m_viewCartButton, SIGNAL( clicked() ), this, SLOT( viewCart() ) );
    connect( m_checkoutButton, SIGNAL( clicked() ), this, SLOT( checkout() ) );
}

void AmazonStore::itemSelected( QModelIndex index )
{
    m_addToCartButton->setEnabled( true );
    m_selectedIndex = index;
}


/* public slots */

void AmazonStore::addToCart()
{
    QString asin, name, price;

    // get item from collection
    if( m_selectedIndex.row() < m_collection->albumIDMap()->size() ) // it's an album
    {
        Meta::AmazonAlbum* album;
        // row == albumId
        album = dynamic_cast<Meta::AmazonAlbum*>( m_collection->albumById( m_selectedIndex.row() + 1 ).data() );

        if( !album )
            return;

        name = m_collection->artistById( album->artistId() )->name() + " - " + album->name();
        asin = album->asin();
        price = album->price();
    }
    else // track
    {
        Meta::AmazonTrack* track;
        int id = m_selectedIndex.row() - m_collection->albumIDMap()->size();
        // row == albumId
        track = dynamic_cast<Meta::AmazonTrack*>( m_collection->trackById( id + 1 ).data() );

        if( !track )
            return;

        name = m_collection->artistById( track->artistId() )->name() + " - " + track->name();
        asin = track->asin();
        price = track->price();
    }

    AmazonCart::instance()->add( asin, price, name );
    Amarok::Components::logger()->longMessage( i18n( "<b>MP3 Music Store</b><br/><br/>The item <em>%1</em> has been added to your shopping cart.", name ) );
    m_checkoutButton->setEnabled( true );
}

void AmazonStore::viewCart()
{
}

void AmazonStore::checkout()
{
    QUrl url = AmazonCart::instance()->checkoutUrl();
    debug() << url;
    m_checkoutButton->setEnabled( false );

    m_ApiMutex.lock();
    QTemporaryFile tempFile;
    tempFile.setAutoRemove( false );  // file must be removed later-> parser

    if( !tempFile.open() )
    {
        Amarok::Components::logger()->shortMessage( i18n( "Error: Unable to write temporary file." ) );
        m_checkoutButton->setEnabled( true );
        m_ApiMutex.unlock();
        return;
    }

    m_tempFileName = tempFile.fileName();

    m_requestJob = KIO::file_copy( url, KUrl( m_tempFileName ), 0700 , KIO::HideProgressInfo | KIO::Overwrite );

    connect( m_requestJob, SIGNAL( result( KJob * ) ), this, SLOT( openCheckoutUrl( KJob * ) ) );
    m_requestJob->start();
}

void AmazonStore::newSearchRequest( const QString request )
{
    DEBUG_BLOCK

    if( AmazonConfig::instance()->country().isEmpty() )
    {
        Amarok::Components::logger()->longMessage( i18n( "<b>MP3 Music Store</b><br/><br/>You have to select your country  before you can search for items in your local Amazon store." ) );

        KCMultiDialog KCM;

        KCM.setWindowTitle( i18n( "Select your Amazon locale - Amarok" ) );
        KCM.addModule( KCModuleInfo( QString( "amarok_service_amazonstore_config.desktop" ) ) );
        KCM.setButtons( KCMultiDialog::Ok | KCMultiDialog::Cancel );
        KCM.resize( 5, 5 ); // HACK but works
        KCM.exec();
        return;
    }

    // create request fetcher thread
    debug() << "Amazon: newSearchRequest: " << request;
    createRequestUrl( request );

    QTemporaryFile tempFile;
    tempFile.setAutoRemove( false );  // file must be removed later -> AmazonParser does it

    if( !tempFile.open() )
    {
        Amarok::Components::logger()->shortMessage( i18n( "Error: Unable to write temporary file." ) );
        return;
    }

    m_ApiMutex.lock();
    m_tempFileName = tempFile.fileName();

    m_requestJob = KIO::file_copy( m_requestUrl, KUrl( m_tempFileName ), 0700 , KIO::HideProgressInfo | KIO::Overwrite );

    connect( m_requestJob, SIGNAL( result( KJob * ) ), this, SLOT( parseReply( KJob * ) ) );
    m_requestJob->start();
}

void AmazonStore::newSpinBoxSearchRequest( int i )
{
    Q_UNUSED( i )
    newSearchRequest( m_searchWidget->currentText() );
}

void AmazonStore::createRequestUrl( QString request )
{
    // TODO enhance this method to allow searching for tracks/albums only and to get futher result pages
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
    m_requestUrl.setUrl( urlString );
    debug() << m_requestUrl;
}

void AmazonStore::parseReply( KJob* requestJob )
{
    DEBUG_BLOCK
    if( requestJob->error() )
    {
        Amarok::Components::logger()->shortMessage( i18n( "Error: Querying MP3 Music Store database failed. :-("  ) );
        debug() << requestJob->errorString();
        requestJob->deleteLater();
        m_ApiMutex.unlock();
        return;
    }

    requestJob->deleteLater();

    // create parser thread
    AmazonParser *parser = new AmazonParser( m_tempFileName, m_collection, m_metaFactory );
    connect( parser, SIGNAL( done( ThreadWeaver::Job* ) ), this, SLOT( parsingDone( ThreadWeaver::Job* ) ) );
    connect( parser, SIGNAL( failed( ThreadWeaver::Job* ) ), this, SLOT( parsingFailed( ThreadWeaver::Job* ) ) );
    ThreadWeaver::Weaver::instance()->enqueue( parser );
}

void AmazonStore::parsingDone( ThreadWeaver::Job* parserJob )
{
    Q_UNUSED( parserJob )
    m_itemModel->collectionChanged();
    m_itemView->setModel( m_itemModel );
    m_addToCartButton->setEnabled( false );
    m_ApiMutex.unlock();
}

void AmazonStore::parsingFailed( ThreadWeaver::Job* parserJob )
{
    Q_UNUSED( parserJob )
    m_ApiMutex.unlock();
}

void AmazonStore::openCheckoutUrl( KJob* requestJob )
{
    // very short document, we can parse it in the main thread
    QDomDocument responseDocument;
    QFile responseFile( m_tempFileName );

    if( !responseFile.open( QIODevice::ReadOnly ) )
    {
        Amarok::Components::logger()->shortMessage( i18n( "Error: Unable to open temporary file." ) );

        m_checkoutButton->setEnabled( true );
        requestJob->deleteLater();
        QFile::remove( m_tempFileName );
        m_ApiMutex.unlock();
        return;
    }

    QString errorMsg;
    int errorLine;
    int errorColumn;

    if( !responseDocument.setContent( &responseFile, false, &errorMsg, &errorLine, &errorColumn ) ) // parse error
    {
        debug() << responseDocument.toString();
        debug() << "Parse ERROR";
        debug() << "Message:" << errorMsg;
        debug() << "Line:" << errorLine;
        debug() << "Column:" << errorColumn;
        Amarok::Components::logger()->shortMessage( i18n( "Error: Unable to parse temporary file." ) );

        m_checkoutButton->setEnabled( true );
        requestJob->deleteLater();
        QFile::remove( m_tempFileName );
        m_ApiMutex.unlock();
        return;
    }

    debug() << responseDocument.toString();

    // now that's the whole parser for this reply:
    QUrl url;
    url.setEncodedUrl( responseDocument.documentElement().elementsByTagName( QString( "purchaseurl" ) ).at( 0 ).firstChild().nodeValue().toAscii() );

    qDebug() << url;
    QDesktopServices::openUrl( url );

    requestJob->deleteLater();
    QFile::remove( m_tempFileName );
    AmazonCart::instance()->clear();
    m_ApiMutex.unlock();
}
