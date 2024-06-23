/****************************************************************************************
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

#include "MagnatuneStore.h"

#include "core/support/Amarok.h"
#include "core/support/Components.h"
#include "core/logger/Logger.h"
#include "amarokurls/AmarokUrlHandler.h"
#include "browsers/CollectionTreeItem.h"
#include "browsers/CollectionTreeView.h"
#include "browsers/SingleCollectionTreeItemModel.h"
#include "EngineController.h"
#include "MagnatuneConfig.h"
#include "MagnatuneDatabaseWorker.h"
#include "MagnatuneInfoParser.h"
#include "MagnatuneNeedUpdateWidget.h"
#include "browsers/InfoProxy.h"
#include "MagnatuneUrlRunner.h"

#include "ui_MagnatuneSignupDialogBase.h"

#include "../ServiceSqlRegistry.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core/support/Debug.h"
#include "playlist/PlaylistModelStack.h"
#include "widgets/SearchWidget.h"

#include <QAction>
#include <QDateTime>
#include <QMenu>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>

#include <KSharedConfig>
#include <ThreadWeaver/ThreadWeaver>
#include <ThreadWeaver/Queue>


////////////////////////////////////////////////////////////////////////////////////////////////////////////
// class MagnatuneServiceFactory
////////////////////////////////////////////////////////////////////////////////////////////////////////////

MagnatuneServiceFactory::MagnatuneServiceFactory()
    : ServiceFactory()
{
}

void MagnatuneServiceFactory::init()
{
    DEBUG_BLOCK
    MagnatuneStore* service = new MagnatuneStore( this, "Magnatune.com" );
    m_initialized = true;
    Q_EMIT newService( service );
}

QString MagnatuneServiceFactory::name()
{
    return "Magnatune.com";
}

KConfigGroup MagnatuneServiceFactory::config()
{
    return Amarok::config( "Service_Magnatune" );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// class MagnatuneStore
////////////////////////////////////////////////////////////////////////////////////////////////////////////

MagnatuneStore::MagnatuneStore( MagnatuneServiceFactory* parent, const char *name )
        : ServiceBase( name, parent )
        , m_downloadHandler( nullptr )
        , m_redownloadHandler( nullptr )
        , m_needUpdateWidget( nullptr )
        , m_downloadInProgress( 0 )
        , m_currentAlbum( nullptr )
        , m_streamType( MagnatuneMetaFactory::OGG )
        , m_magnatuneTimestamp( 0 )
        , m_registry( nullptr )
        , m_signupInfoWidget( nullptr )
{
    DEBUG_BLOCK
    setObjectName(name);
    //initTopPanel( );

    setShortDescription( i18n( "\"Fair trade\" online music store" ) );
    setIcon( QIcon::fromTheme( "view-services-magnatune-amarok" ) );

    // xgettext: no-c-format
    setLongDescription( i18n( "Magnatune.com is a different kind of record company with the motto \"We are not evil!\" 50% of every purchase goes directly to the artist and if you purchase an album through Amarok, the Amarok project receives a 10% commission. Magnatune.com also offers \"all you can eat\" memberships that lets you download as much of their music as you like." ) );
    setImagePath( QStandardPaths::locate( QStandardPaths::GenericDataLocation, "amarok/images/hover_info_magnatune.png" ) );


    //initBottomPanel();
//    m_currentlySelectedItem = 0;

    m_polished = false;
    //polish( );  //FIXME not happening when shown for some reason


    //do this stuff now to make us function properly as a track provider on startup. The expensive stuff will
    //not happen until the model is added to the view anyway.
    MagnatuneMetaFactory * metaFactory = new MagnatuneMetaFactory( "magnatune", this );
    
    MagnatuneConfig config;
    if ( config.isMember() ) {
        setMembership( config.membershipType(), config.username(), config.password() );
        metaFactory->setMembershipInfo( config.membershipPrefix(), m_username, m_password );
    }

    setStreamType( config.streamType() );

    metaFactory->setStreamType( m_streamType );
    m_registry = new ServiceSqlRegistry( metaFactory );
    m_collection = new Collections::MagnatuneSqlCollection( "magnatune", "Magnatune.com", metaFactory, m_registry );
    CollectionManager::instance()->addTrackProvider( m_collection );
    setServiceReady( true );
}

MagnatuneStore::~MagnatuneStore()
{
    CollectionManager::instance()->removeTrackProvider( m_collection );
    delete m_registry;
    delete m_collection;
}


void MagnatuneStore::download( )
{
    DEBUG_BLOCK
    if ( m_downloadInProgress )
        return;

    if ( !m_polished )
        polish();

    debug() << "here";

    //check if we need to start a download or show the signup dialog
    if( !m_isMember || m_membershipType != MagnatuneConfig::DOWNLOAD )
    {
        showSignupDialog();
        return;
    }

    m_downloadInProgress = true;
    m_downloadAlbumButton->setEnabled( false );

    if ( !m_downloadHandler )
    {
        m_downloadHandler = new MagnatuneDownloadHandler();
        m_downloadHandler->setParent( this );
        connect( m_downloadHandler, &MagnatuneDownloadHandler::downloadCompleted,
                 this, &MagnatuneStore::downloadCompleted );
    }

    if ( m_currentAlbum != nullptr )
        m_downloadHandler->downloadAlbum( m_currentAlbum );
}


void MagnatuneStore::downloadTrack( Meta::MagnatuneTrack * track )
{
    Meta::MagnatuneAlbum * album = dynamic_cast<Meta::MagnatuneAlbum *>( track->album().data() );
    if ( album )
        downloadAlbum( album );
}

void MagnatuneStore::downloadAlbum( Meta::MagnatuneAlbum * album )
{
    DEBUG_BLOCK
    if ( m_downloadInProgress )
        return;

    if ( !m_polished )
        polish();

    m_downloadInProgress = true;
    m_downloadAlbumButton->setEnabled( false );

    if ( !m_downloadHandler )
    {
        m_downloadHandler = new MagnatuneDownloadHandler();
        m_downloadHandler->setParent( this );
        connect( m_downloadHandler, &MagnatuneDownloadHandler::downloadCompleted,
                 this, &MagnatuneStore::downloadCompleted );
    }

    m_downloadHandler->downloadAlbum( album );
}


void MagnatuneStore::initTopPanel( )
{
    QMenu *filterMenu = new QMenu( nullptr );

    QAction *action = filterMenu->addAction( i18n("Artist") );
    connect( action, &QAction::triggered, this, &MagnatuneStore::sortByArtist );

    action = filterMenu->addAction( i18n( "Artist / Album" ) );
    connect( action, &QAction::triggered, this, &MagnatuneStore::sortByArtistAlbum );

    action = filterMenu->addAction( i18n( "Album" ) ) ;
    connect( action, &QAction::triggered, this, &MagnatuneStore::sortByAlbum );

    action = filterMenu->addAction( i18n( "Genre / Artist" ) );
    connect( action, &QAction::triggered, this, &MagnatuneStore::sortByGenreArtist );

    action = filterMenu->addAction( i18n( "Genre / Artist / Album" ) );
    connect( action, &QAction::triggered, this, &MagnatuneStore::sortByGenreArtistAlbum );

    QAction *filterMenuAction = new QAction( QIcon::fromTheme( "preferences-other" ), i18n( "Sort Options" ), this );
    filterMenuAction->setMenu( filterMenu );

    m_searchWidget->toolBar()->addSeparator();
    m_searchWidget->toolBar()->addAction( filterMenuAction );

    QToolButton *tbutton = qobject_cast<QToolButton*>( m_searchWidget->toolBar()->widgetForAction( filterMenuAction ) );
    if( tbutton )
        tbutton->setPopupMode( QToolButton::InstantPopup );

    QMenu * actionsMenu = new QMenu( nullptr );

    action = actionsMenu->addAction( i18n( "Re-download" ) );
    connect( action, &QAction::triggered, this, &MagnatuneStore::processRedownload );

    m_updateAction = actionsMenu->addAction( i18n( "Update Database" ) );
    connect( m_updateAction, &QAction::triggered, this, &MagnatuneStore::updateButtonClicked );

    QAction *actionsMenuAction = new QAction( QIcon::fromTheme( "list-add" ), i18n( "Tools" ), this );
    actionsMenuAction->setMenu( actionsMenu );

    m_searchWidget->toolBar()->addAction( actionsMenuAction );

    tbutton = qobject_cast<QToolButton*>( m_searchWidget->toolBar()->widgetForAction( actionsMenuAction ) );
    if( tbutton )
        tbutton->setPopupMode( QToolButton::InstantPopup );

}

void MagnatuneStore::initBottomPanel()
{
    //m_bottomPanel->setMaximumHeight( 24 );

    m_downloadAlbumButton = new QPushButton;
    m_downloadAlbumButton->setParent( m_bottomPanel );

    MagnatuneConfig config;
    if ( config.isMember() && config.membershipType() == MagnatuneConfig::DOWNLOAD )
    {
        m_downloadAlbumButton->setText( i18n( "Download Album" ) );
        m_downloadAlbumButton->setEnabled( false );
    }
    else if ( config.isMember() )
        m_downloadAlbumButton->hide();
    else
    {
        m_downloadAlbumButton->setText( i18n( "Signup" ) );
        m_downloadAlbumButton->setEnabled( true );
    }

    m_downloadAlbumButton->setObjectName( "downloadButton" );
    m_downloadAlbumButton->setIcon( QIcon::fromTheme( "download-amarok" ) );
    
    connect( m_downloadAlbumButton, &QPushButton::clicked, this, &MagnatuneStore::download );

    if ( !config.lastUpdateTimestamp() )
    {
        m_needUpdateWidget = new MagnatuneNeedUpdateWidget(m_bottomPanel);

        connect( m_needUpdateWidget, &MagnatuneNeedUpdateWidget::wantUpdate,
                 this, &MagnatuneStore::updateButtonClicked );

        m_downloadAlbumButton->setParent(nullptr);
    }
}


void MagnatuneStore::updateButtonClicked()
{
    DEBUG_BLOCK
    m_updateAction->setEnabled( false );
    if ( m_needUpdateWidget )
        m_needUpdateWidget->disable();

    updateMagnatuneList();
}


bool MagnatuneStore::updateMagnatuneList()
{
    DEBUG_BLOCK
    //download new list from magnatune

     debug() << "MagnatuneStore: start downloading xml file";


    QTemporaryFile tempFile;
//     tempFile.setSuffix( ".bz2" );
    tempFile.setAutoRemove( false );  // file will be removed in MagnatuneXmlParser
    if( !tempFile.open() )
    {
        return false; //error
    }

    m_tempFileName = tempFile.fileName();

    m_listDownloadJob = KIO::file_copy( QUrl("http://magnatune.com/info/album_info_xml.bz2"),  QUrl::fromLocalFile( m_tempFileName ), 0700 , KIO::HideProgressInfo | KIO::Overwrite );
    Amarok::Logger::newProgressOperation( m_listDownloadJob, i18n( "Downloading Magnatune.com database..." ), this, &MagnatuneStore::listDownloadCancelled );

    connect( m_listDownloadJob, &KJob::result,
            this, &MagnatuneStore::listDownloadComplete );

    return true;
}


void MagnatuneStore::listDownloadComplete( KJob * downLoadJob )
{
   DEBUG_BLOCK
   debug() << "MagnatuneStore: xml file download complete";

    if ( downLoadJob != m_listDownloadJob ) {
        debug() << "wrong job, ignoring....";
        return ; //not the right job, so let's ignore it
    }

    m_updateAction->setEnabled( true );

    if ( downLoadJob->error() != 0 )
    {
        debug() << "Got an error, bailing out: " << downLoadJob->errorString();
        //TODO: error handling here
        return ;
    }


    Amarok::Logger::shortMessage( i18n( "Updating the local Magnatune database."  ) );
    MagnatuneXmlParser * parser = new MagnatuneXmlParser( m_tempFileName );
    parser->setDbHandler( new MagnatuneDatabaseHandler() );
    connect( parser, &MagnatuneXmlParser::doneParsing, this, &MagnatuneStore::doneParsing );

    ThreadWeaver::Queue::instance()->enqueue( QSharedPointer<ThreadWeaver::Job>(parser) );
}


void MagnatuneStore::listDownloadCancelled( )
{
    DEBUG_BLOCK

    m_listDownloadJob->kill();
    m_listDownloadJob = nullptr;
    debug() << "Aborted xml download";

    m_updateAction->setEnabled( true );
    if ( m_needUpdateWidget )
        m_needUpdateWidget->enable();
}



void MagnatuneStore::doneParsing()
{
    debug() << "MagnatuneStore: done parsing";
    m_collection->emitUpdated();

    //update the last update timestamp

    MagnatuneConfig config;
    if ( m_magnatuneTimestamp == 0 )
        config.setLastUpdateTimestamp( QDateTime::currentDateTimeUtc().toSecsSinceEpoch() );
    else
        config.setLastUpdateTimestamp( m_magnatuneTimestamp );

    config.save();

    if ( m_needUpdateWidget )
    {
        m_needUpdateWidget->setParent(nullptr);
        m_needUpdateWidget->deleteLater();
        m_needUpdateWidget = nullptr;

        m_downloadAlbumButton->setParent(m_bottomPanel);
    }
}


void MagnatuneStore::processRedownload( )
{
    debug() << "Process redownload";

    if ( m_redownloadHandler == nullptr )
    {
        m_redownloadHandler = new MagnatuneRedownloadHandler( this );
    }
    m_redownloadHandler->showRedownloadDialog();
}


void MagnatuneStore::downloadCompleted( bool )
{
    delete m_downloadHandler;
    m_downloadHandler = nullptr;

    m_downloadAlbumButton->setEnabled( true );
    m_downloadInProgress = false;

    debug() << "Purchase operation complete";

    //TODO: display some kind of success dialog here?
}


void MagnatuneStore::itemSelected( CollectionTreeItem * selectedItem )
{
    DEBUG_BLOCK

    //only care if the user has a download membership
    if( !m_isMember || m_membershipType != MagnatuneConfig::DOWNLOAD )
        return;

    //we only enable the purchase button if there is only one item selected and it happens to
    //be an album or a track
    Meta::DataPtr dataPtr = selectedItem->data();

    if ( auto track = AmarokSharedPointer<Meta::MagnatuneTrack>::dynamicCast( dataPtr ) )
    {
        debug() << "is right type (track)";
        m_currentAlbum = static_cast<Meta::MagnatuneAlbum *> ( track->album().data() );
        m_downloadAlbumButton->setEnabled( true );
    }
    else if ( auto album = AmarokSharedPointer<Meta::MagnatuneAlbum>::dynamicCast( dataPtr ) )
    {
        m_currentAlbum = album.data();
        debug() << "is right type (album) named " << m_currentAlbum->name();

        m_downloadAlbumButton->setEnabled( true );
    }
    else
    {
        debug() << "is wrong type";
        m_downloadAlbumButton->setEnabled( false );
    }
}


void MagnatuneStore::addMoodyTracksToPlaylist( const QString &mood, int count )
{
    MagnatuneDatabaseWorker *databaseWorker = new MagnatuneDatabaseWorker();
    databaseWorker->fetchTrackswithMood( mood, count, m_registry );
    connect( databaseWorker, &MagnatuneDatabaseWorker::gotMoodyTracks, this, &MagnatuneStore::moodyTracksReady );

    ThreadWeaver::Queue::instance()->enqueue( QSharedPointer<ThreadWeaver::Job>(databaseWorker) );
}


void MagnatuneStore::polish()
{
    DEBUG_BLOCK;

    if (!m_polished) {
        m_polished = true;

        initTopPanel( );
        initBottomPanel();

        QList<CategoryId::CatMenuId> levels;
        levels << CategoryId::Genre << CategoryId::Artist << CategoryId::Album;

        m_magnatuneInfoParser = new MagnatuneInfoParser();

        setInfoParser( m_magnatuneInfoParser );
        setModel( new SingleCollectionTreeItemModel( m_collection, levels ) );

        connect( qobject_cast<CollectionTreeView*>(m_contentView), &CollectionTreeView::itemSelected,
                 this, &MagnatuneStore::itemSelected );

        //add a custom url runner
        MagnatuneUrlRunner *runner = new MagnatuneUrlRunner();

        connect( runner, &MagnatuneUrlRunner::showFavorites, this, &MagnatuneStore::showFavoritesPage );
        connect( runner, &MagnatuneUrlRunner::showHome, this, &MagnatuneStore::showHomePage );
        connect( runner, &MagnatuneUrlRunner::showRecommendations, this, &MagnatuneStore::showRecommendationsPage );
        connect( runner, &MagnatuneUrlRunner::buyOrDownload, this, &MagnatuneStore::downloadSku );
        connect( runner, &MagnatuneUrlRunner::removeFromFavorites, this, &MagnatuneStore::removeFromFavorites );

        The::amarokUrlHandler()->registerRunner( runner, runner->command() );
    }

    MagnatuneInfoParser * parser = dynamic_cast<MagnatuneInfoParser *> ( infoParser() );
    if ( parser )
        parser->getFrontPage();

    //get a mood map we can show to the cloud view

    MagnatuneDatabaseWorker * databaseWorker = new MagnatuneDatabaseWorker();
    databaseWorker->fetchMoodMap();
    connect( databaseWorker, &MagnatuneDatabaseWorker::gotMoodMap, this, &MagnatuneStore::moodMapReady );
    ThreadWeaver::Queue::instance()->enqueue( QSharedPointer<ThreadWeaver::Job>(databaseWorker) );

    if ( MagnatuneConfig().autoUpdateDatabase() )
        checkForUpdates();
}



void MagnatuneStore::setMembership( int type, const QString & username, const QString & password)
{
    m_isMember = true;
    m_membershipType = type;
    m_username = username;
    m_password = password;
}


void MagnatuneStore::moodMapReady(const QMap< QString, int > &map)
{
    QVariantMap variantMap;
    QList<QVariant> strings;
    QList<QVariant> weights;
    QVariantMap dbusActions;

    for( const QString &key : map.keys() ) {

        strings << key;
        weights << map.value( key );

        QString escapedKey = key;
        escapedKey.replace( QLatin1Char(' '), "%20" );
        QVariantMap action;
        action["component"]  = "/ServicePluginManager";
        action["function"] = "sendMessage";
        action["arg1"] = QStringLiteral( "Magnatune.com");
        action["arg2"] = QStringLiteral( "addMoodyTracks %1 10").arg( escapedKey );

        dbusActions[key] = action;

    }

    variantMap["cloud_name"] = QVariant( "Magnatune Moods" );
    variantMap["cloud_strings"] = QVariant( strings );
    variantMap["cloud_weights"] = QVariant( weights );
    variantMap["cloud_actions"] = QVariant( dbusActions );

    The::infoProxy()->setCloud( variantMap );
}


void MagnatuneStore::setStreamType( int type )
{
    m_streamType = type;
}

void MagnatuneStore::checkForUpdates()
{
    m_updateTimestampDownloadJob = KIO::storedGet( QUrl("http://magnatune.com/info/last_update_timestamp"), KIO::Reload, KIO::HideProgressInfo );
    connect( m_updateTimestampDownloadJob, &KJob::result, this, &MagnatuneStore::timestampDownloadComplete );
}


void MagnatuneStore::timestampDownloadComplete( KJob *  job )
{
    DEBUG_BLOCK

    if ( job->error() != 0 )
    {
        //TODO: error handling here
        return ;
    }
    if ( job != m_updateTimestampDownloadJob )
        return ; //not the right job, so let's ignore it


    QString timestampString = ( ( KIO::StoredTransferJob* ) job )->data();
    debug() << "Magnatune timestamp: " << timestampString;

    bool ok;
    qulonglong magnatuneTimestamp = timestampString.toULongLong( &ok );

    MagnatuneConfig config;
    qulonglong localTimestamp = config.lastUpdateTimestamp();

    debug() << "Last update timestamp: " << QString::number( localTimestamp );

    if ( ok && magnatuneTimestamp > localTimestamp ) {
        m_magnatuneTimestamp = magnatuneTimestamp;
        updateButtonClicked();
    }
}


void MagnatuneStore::moodyTracksReady( const Meta::TrackList &tracks )
{
    DEBUG_BLOCK
    The::playlistController()->insertOptioned( tracks, Playlist::Replace );
}


QString MagnatuneStore::messages()
{
    QString text = i18n( "The Magnatune.com service accepts the following messages: \n\n\taddMoodyTracks mood count: Adds a number of random tracks with the specified mood to the playlist. The mood argument must have spaces escaped with %%20" );

    return text;
}


QString MagnatuneStore::sendMessage( const QString & message )
{
    QStringList args = message.split( QLatin1Char(' '), Qt::SkipEmptyParts );

    if ( args.size() < 1 ) {
        return i18n( "ERROR: No arguments supplied" );
    }

    if ( args[0] == "addMoodyTracks" ) {
        if ( args.size() != 3 ) {
            return i18n( "ERROR: Wrong number of arguments for addMoodyTracks" );
        }

        QString mood = args[1];
        mood = mood.replace( "%20", " " );

        bool ok;
        int count = args[2].toInt( &ok );

        if ( !ok )
            return i18n( "ERROR: Parse error for argument 2 ( count )" );

        addMoodyTracksToPlaylist( mood, count );

        return i18n( "ok" );
    }

    return i18n( "ERROR: Unknown argument." );
}

void MagnatuneStore::showFavoritesPage()
{
    DEBUG_BLOCK
    m_magnatuneInfoParser->getFavoritesPage();
}

void MagnatuneStore::showHomePage()
{
    DEBUG_BLOCK
    m_magnatuneInfoParser->getFrontPage();
}

void MagnatuneStore::showRecommendationsPage()
{
    DEBUG_BLOCK
    m_magnatuneInfoParser->getRecommendationsPage();
}

void MagnatuneStore::downloadSku( const QString &sku )
{
    DEBUG_BLOCK
    debug() << "sku: " << sku;
    MagnatuneDatabaseWorker * databaseWorker = new MagnatuneDatabaseWorker();
    databaseWorker->fetchAlbumBySku( sku, m_registry );
    connect( databaseWorker, &MagnatuneDatabaseWorker::gotAlbumBySku, this, &MagnatuneStore::downloadAlbum );

    ThreadWeaver::Queue::instance()->enqueue( QSharedPointer<ThreadWeaver::Job>(databaseWorker) );
}

void MagnatuneStore::addToFavorites( const QString &sku )
{
    DEBUG_BLOCK
    MagnatuneConfig config;

    if( !config.isMember() )
        return;

    QString url = "http://%1:%2@%3.magnatune.com/member/favorites?action=add_api&sku=%4";
    url = url.arg( config.username(), config.password(), config.membershipPrefix(), sku );

    debug() << "favorites url: " << url;

    m_favoritesJob = KIO::storedGet( QUrl( url ), KIO::Reload, KIO::HideProgressInfo );
    connect( m_favoritesJob, &KJob::result, this, &MagnatuneStore::favoritesResult );
}

void MagnatuneStore::removeFromFavorites( const QString &sku )
{
    DEBUG_BLOCK
    MagnatuneConfig config;

    if( !config.isMember() )
        return;

    QString url = "http://%1:%2@%3.magnatune.com/member/favorites?action=remove_api&sku=%4";
    url = url.arg( config.username(), config.password(), config.membershipPrefix(), sku );

    debug() << "favorites url: " << url;

    m_favoritesJob = KIO::storedGet( QUrl( url ), KIO::Reload, KIO::HideProgressInfo );
    connect( m_favoritesJob, &KJob::result, this, &MagnatuneStore::favoritesResult );
}

void MagnatuneStore::favoritesResult( KJob* addToFavoritesJob )
{
    if( addToFavoritesJob != m_favoritesJob )
        return;

    QString result = m_favoritesJob->data();

    Amarok::Logger::longMessage( result );

    //show the favorites page
    showFavoritesPage();
}

void
MagnatuneStore::showSignupDialog()
{

    if ( m_signupInfoWidget== nullptr )
    {
        m_signupInfoWidget = new QDialog;
        Ui::SignupDialog ui;
        ui.setupUi( m_signupInfoWidget );
    }

     m_signupInfoWidget->show();
}
