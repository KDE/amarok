/****************************************************************************************
 * Copyright (c) 2006,2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>               *
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

#include "MagnatuneStore.h"

#include "Amarok.h"
#include "amarokurls/AmarokUrlHandler.h"
#include "statusbar/StatusBar.h"
#include "statusbar/ProgressBar.h"
#include "EngineController.h"
#include "MagnatuneConfig.h"
#include "MagnatuneDatabaseWorker.h"
#include "MagnatuneInfoParser.h"
#include "browsers/InfoProxy.h"
#include "MagnatuneUrlRunner.h"

#include "../ServiceSqlRegistry.h"
#include "CollectionManager.h"
#include "Debug.h"
#include "playlist/PlaylistController.h"
#include "widgets/SearchWidget.h"

#include <KAction>
#include <KMenuBar>
#include <KStandardDirs>  //locate()
#include <KTemporaryFile>
#include <KUrl>
#include <threadweaver/ThreadWeaver.h>

#include <QAction>
#include <QDateTime>
#include <QMenu>

#include <typeinfo>

AMAROK_EXPORT_PLUGIN( MagnatuneServiceFactory )

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// class MagnatuneServiceFactory
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void MagnatuneServiceFactory::init()
{
    DEBUG_BLOCK
    MagnatuneStore* service = new MagnatuneStore( this, "Magnatune.com" );
    m_activeServices << service;
    m_initialized = true;
    emit newService( service );
}

QString MagnatuneServiceFactory::name()
{
    return "Magnatune.com";
}

KPluginInfo MagnatuneServiceFactory::info()
{
    KPluginInfo pluginInfo( "amarok_service_magnatunestore.desktop", "services" );
    pluginInfo.setConfig( config() );
    return pluginInfo;
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
        , m_purchaseHandler( 0 )
        , m_redownloadHandler( 0 )
        , m_purchaseInProgress( 0 )
        , m_currentAlbum( 0 )
        , m_streamType( MagnatuneMetaFactory::OGG )
        , m_magnatuneTimestamp( 0 )
        , m_registry( 0 )
{
    setObjectName(name);
    DEBUG_BLOCK
    //initTopPanel( );

    setShortDescription( i18n( "\"Fair trade\" online music store." ) );
    setIcon( KIcon( "view-services-magnatune-amarok" ) );

    // xgettext: no-c-format
    setLongDescription( i18n( "Magnatune.com is a different kind of record company with the motto \"We are not evil!\" 50% of every purchase goes directly to the artist and if you purchase an album through Amarok, the Amarok project receives a 10% commission. Magnatune.com also offers \"all you can eat\" memberships that lets you download as much of their music as you like." ) );
    setImagePath( KStandardDirs::locate( "data", "amarok/images/hover_info_magnatune.png" ) );


    //initBottomPanel();
//    m_currentlySelectedItem = 0;

    m_polished = false;
    //polish( );  //FIXME not happening when shown for some reason


    //do this stuff now to make us function properly as a track provider on startup. The expensive stuff will
    //not happen untill the model is added to the view anyway.
    MagnatuneMetaFactory * metaFactory = new MagnatuneMetaFactory( "magnatune", this );

    MagnatuneConfig config;
    if ( config.isMember() ) {
        setMembership( config.membershipType(), config.username(), config.password() );
        metaFactory->setMembershipInfo( m_membershipType.toLower(), m_username, m_password );
    }

    setStreamType( config.streamType() );

    metaFactory->setStreamType( m_streamType );
    m_registry = new ServiceSqlRegistry( metaFactory );
    m_collection = new MagnatuneSqlCollection( "magnatune", "Magnatune.com", metaFactory, m_registry );
    m_serviceready = true;
    CollectionManager::instance()->addUnmanagedCollection( m_collection, CollectionManager::CollectionDisabled );
    emit( ready() );
}

MagnatuneStore::~MagnatuneStore()
{
    CollectionManager::instance()->removeUnmanagedCollection( m_collection );
    //hm, memory handling?
}


void MagnatuneStore::purchase( )
{
    DEBUG_BLOCK
    if ( m_purchaseInProgress )
        return;

    if ( !m_polished )
        polish();

    debug() << "here";

    m_purchaseInProgress = true;
    m_purchaseAlbumButton->setEnabled( false );

    if ( !m_purchaseHandler )
    {
        m_purchaseHandler = new MagnatunePurchaseHandler();
        m_purchaseHandler->setParent( this );
        connect( m_purchaseHandler, SIGNAL( purchaseCompleted( bool ) ), this, SLOT( purchaseCompleted( bool ) ) );
    }

    if ( m_currentAlbum != 0 )
        m_purchaseHandler->purchaseAlbum( m_currentAlbum );
}


void MagnatuneStore::purchase( Meta::MagnatuneTrack * track )
{
    Meta::MagnatuneAlbum * album = dynamic_cast<Meta::MagnatuneAlbum *>( track->album().data() );
    if ( album )
        purchase( album );
}

void MagnatuneStore::purchase( Meta::MagnatuneAlbum * album )
{

    DEBUG_BLOCK
    if ( m_purchaseInProgress )
        return;

    if ( !m_polished )
        polish();

    m_purchaseInProgress = true;
    m_purchaseAlbumButton->setEnabled( false );
    
    if ( !m_purchaseHandler )
    {
        m_purchaseHandler = new MagnatunePurchaseHandler();
        m_purchaseHandler->setParent( this );
        connect( m_purchaseHandler, SIGNAL( purchaseCompleted( bool ) ), this, SLOT( purchaseCompleted( bool ) ) );
    }
    
    m_purchaseHandler->purchaseAlbum( album );
}


void MagnatuneStore::initTopPanel( )
{

    QMenu *filterMenu = new QMenu( 0 );
    
    QAction *action = filterMenu->addAction( i18n("Artist") );
    connect( action, SIGNAL( triggered( bool ) ), SLOT( sortByArtist() ) );

    action = filterMenu->addAction( i18n( "Artist / Album" ) );
    connect( action, SIGNAL( triggered( bool ) ), SLOT( sortByArtistAlbum() ) );

    action = filterMenu->addAction( i18n( "Album" ) ) ;
    connect( action, SIGNAL( triggered( bool ) ), SLOT( sortByAlbum() ) );

    action = filterMenu->addAction( i18n( "Genre / Artist" ) );
    connect( action, SIGNAL( triggered( bool ) ), SLOT( sortByGenreArtist() ) );

    action = filterMenu->addAction( i18n( "Genre / Artist / Album" ) );
    connect( action, SIGNAL( triggered( bool ) ), SLOT( sortByGenreArtistAlbum() ) );

    KAction *filterMenuAction = new KAction( KIcon( "preferences-other" ), i18n( "Sort Options" ), this );
    filterMenuAction->setMenu( filterMenu );

    m_searchWidget->toolBar()->addSeparator();
    m_searchWidget->toolBar()->addAction( filterMenuAction );

    QToolButton *tbutton = qobject_cast<QToolButton*>( m_searchWidget->toolBar()->widgetForAction( filterMenuAction ) );
    if( tbutton )
        tbutton->setPopupMode( QToolButton::InstantPopup );

    QMenu * actionsMenu = new QMenu( 0 );

    action = actionsMenu->addAction( i18n( "Re-download" ) );
    connect( action, SIGNAL( triggered( bool) ), SLOT( processRedownload() ) );

    m_updateAction = actionsMenu->addAction( i18n( "Update Database" ) );
    connect( m_updateAction, SIGNAL( triggered( bool) ), SLOT( updateButtonClicked() ) );

    KAction *actionsMenuAction = new KAction( KIcon( "list-add" ), i18n( "Tools" ), this );
    actionsMenuAction->setMenu( actionsMenu );
    
    m_searchWidget->toolBar()->addAction( actionsMenuAction );

    tbutton = qobject_cast<QToolButton*>( m_searchWidget->toolBar()->widgetForAction( actionsMenuAction ) );
    if( tbutton )
        tbutton->setPopupMode( QToolButton::InstantPopup );

}

void MagnatuneStore::initBottomPanel()
{
    //m_bottomPanel->setMaximumHeight( 24 );

    m_purchaseAlbumButton = new QPushButton;
    m_purchaseAlbumButton->setParent( m_bottomPanel );

    MagnatuneConfig config;
    if ( config.isMember() && config.membershipType() == "Download" )
        m_purchaseAlbumButton->setText( i18n( "Download Album" ) );
    else
        m_purchaseAlbumButton->setText( i18n( "Purchase Album" ) );
    
    m_purchaseAlbumButton->setObjectName( "purchaseButton" );
    m_purchaseAlbumButton->setIcon( KIcon( "download-amarok" ) );
    m_purchaseAlbumButton->setEnabled( false );

    connect( m_purchaseAlbumButton, SIGNAL( clicked() ) , this, SLOT( purchase() ) );
}


void MagnatuneStore::updateButtonClicked()
{
    DEBUG_BLOCK
    m_updateAction->setEnabled( false );
    updateMagnatuneList();
}


bool MagnatuneStore::updateMagnatuneList()
{
    DEBUG_BLOCK
    //download new list from magnatune

     debug() << "MagnatuneStore: start downloading xml file";


    KTemporaryFile tempFile;
    tempFile.setSuffix( ".bz2" );
    tempFile.setAutoRemove( false );  //file must be removed later
    if( !tempFile.open() )
    {
        return false; //error
    }

    m_tempFileName = tempFile.fileName();

    m_listDownloadJob = KIO::file_copy( KUrl( "http://magnatune.com/info/album_info_xml.bz2" ),  KUrl( m_tempFileName ), 0700 , KIO::HideProgressInfo | KIO::Overwrite );
    The::statusBar()->newProgressOperation( m_listDownloadJob, i18n( "Downloading Magnatune.com Database" ) )
    ->setAbortSlot( this, SLOT( listDownloadCancelled() ) );
            

    connect( m_listDownloadJob, SIGNAL( result( KJob * ) ),
            this, SLOT( listDownloadComplete( KJob * ) ) );


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
    if ( !downLoadJob->error() == 0 )
    {
        debug() << "Got an error, bailing out: " << downLoadJob->errorString();
        //TODO: error handling here
        return ;
    }


    The::statusBar()->shortMessage( i18n( "Updating the local Magnatune database."  ) );
    debug() << "MagnatuneStore: create xml parser";
    MagnatuneXmlParser * parser = new MagnatuneXmlParser( m_tempFileName );
    parser->setDbHandler( new MagnatuneDatabaseHandler() );
    connect( parser, SIGNAL( doneParsing() ), SLOT( doneParsing() ) );

    ThreadWeaver::Weaver::instance()->enqueue( parser );
}


void MagnatuneStore::listDownloadCancelled( )
{
    DEBUG_BLOCK
    
    //The::statusBar()->endProgressOperation( m_listDownloadJob );
    m_listDownloadJob->kill();
    delete m_listDownloadJob;
    m_listDownloadJob = 0;
    debug() << "Aborted xml download";

    m_updateAction->setEnabled( true );
}



void MagnatuneStore::doneParsing()
{
    debug() << "MagnatuneStore: done parsing";
    m_collection->emitUpdated();

    //update the last update timestamp

    MagnatuneConfig config;
    if ( m_magnatuneTimestamp == 0 )
        config.setLastUpdateTimestamp( QDateTime::currentDateTime().toTime_t() );
    else
        config.setLastUpdateTimestamp( m_magnatuneTimestamp );
    
    config.save();
}


void MagnatuneStore::processRedownload( )
{
    debug() << "Process redownload";

    if ( m_redownloadHandler == 0 )
    {
        m_redownloadHandler = new MagnatuneRedownloadHandler( this );
    }
    m_redownloadHandler->showRedownloadDialog();
}


void MagnatuneStore::purchaseCompleted( bool )
{
    delete m_purchaseHandler;
    m_purchaseHandler = 0;

    m_purchaseAlbumButton->setEnabled( true );
    m_purchaseInProgress = false;

    debug() << "Purchase operation complete";

    //TODO: display some kind of success dialog here?
}


void MagnatuneStore::itemSelected( CollectionTreeItem * selectedItem )
{
    DEBUG_BLOCK

    //we only enable the purchase button if there is only one item selected and it happens to
    //be an album or a track
    Meta::DataPtr dataPtr = selectedItem->data();

    if ( typeid( * dataPtr.data() ) == typeid( Meta::MagnatuneTrack ) )  {

        debug() << "is right type (track)";
        Meta::MagnatuneTrack * track = static_cast<Meta::MagnatuneTrack *> ( dataPtr.data() );
        m_currentAlbum = static_cast<Meta::MagnatuneAlbum *> ( track->album().data() );
        m_purchaseAlbumButton->setEnabled( true );

    } else if ( typeid( * dataPtr.data() ) == typeid( Meta::MagnatuneAlbum ) ) {

        m_currentAlbum = static_cast<Meta::MagnatuneAlbum *> ( dataPtr.data() );
        debug() << "is right type (album) named " << m_currentAlbum->name();

        m_purchaseAlbumButton->setEnabled( true );
    } else {

        debug() << "is wrong type";
        m_purchaseAlbumButton->setEnabled( false );

    }
}


void MagnatuneStore::addMoodyTracksToPlaylist( const QString &mood, int count )
{
    MagnatuneDatabaseWorker * databaseWorker = new MagnatuneDatabaseWorker();
    databaseWorker->fetchTrackswithMood( mood, count, m_registry );
    connect( databaseWorker, SIGNAL( gotMoodyTracks( Meta::TrackList ) ), this, SLOT( moodyTracksReady(Meta::TrackList ) ) );
    
    ThreadWeaver::Weaver::instance()->enqueue( databaseWorker );
}


void MagnatuneStore::polish()
{
    DEBUG_BLOCK;

    if (!m_polished) {
        m_polished = true;

        initTopPanel( );
        initBottomPanel();

        QList<int> levels;
        levels << CategoryId::Genre << CategoryId::Artist << CategoryId::Album;

        m_magnatuneInfoParser = new MagnatuneInfoParser();
        
        setInfoParser( m_magnatuneInfoParser );
        setModel( new SingleCollectionTreeItemModel( m_collection, levels ) );

        connect( m_contentView, SIGNAL( itemSelected( CollectionTreeItem * ) ), this, SLOT( itemSelected( CollectionTreeItem * ) ) );

        //add a custom url runner

        MagnatuneUrlRunner * runner = new MagnatuneUrlRunner();

        connect( runner, SIGNAL( showFavorites() ), this, SLOT( showFavoritesPage() ) );
        connect( runner, SIGNAL( showHome() ), this, SLOT( showHomePage() ) );
        connect( runner, SIGNAL( showRecommendations() ), this, SLOT( showRecommendationsPage() ) );
        connect( runner, SIGNAL( buyOrDownload( const QString & ) ), this, SLOT( purchase( const QString & ) ) );
        connect( runner, SIGNAL( removeFromFavorites( const QString & ) ), this, SLOT( removeFromFavorites( const QString & ) ) );
        
        The::amarokUrlHandler()->registerRunner( runner, "service_magnatune" );
    }

    const KUrl url( KStandardDirs::locate( "data", "amarok/data/" ) );
    QString imagePath = url.url();

    MagnatuneInfoParser * parser = dynamic_cast<MagnatuneInfoParser *> ( infoParser() );
    if ( parser )
        parser->getFrontPage();

    //get a mood map we can show to the cloud view

    MagnatuneDatabaseWorker * databaseWorker = new MagnatuneDatabaseWorker();
    databaseWorker->fetchMoodMap();
    connect( databaseWorker, SIGNAL( gotMoodMap(QMap< QString, int >) ), this, SLOT( moodMapReady(QMap< QString, int >) ) );
    ThreadWeaver::Weaver::instance()->enqueue( databaseWorker );

    checkForUpdates();
}



void MagnatuneStore::setMembership(const QString & type, const QString & username, const QString & password)
{
    m_isMember = true;
    m_membershipType = type;
    m_username = username;
    m_password = password;
}


void MagnatuneStore::moodMapReady(QMap< QString, int > map)
{
    QVariantMap variantMap;
    QList<QVariant> strings;
    QList<QVariant> weights;
    QVariantMap dbusActions;

    foreach( const QString &key, map.keys() ) {
    
        strings << key;
        weights << map.value( key );

        QString escapedKey = key;
        escapedKey.replace( ' ', "%20" );
        QVariantMap action;
        action["component"]  = "/ServicePluginManager";
        action["function"] = "sendMessage";
        action["arg1"] = QString( "Magnatune.com").arg( escapedKey );
        action["arg2"] = QString( "addMoodyTracks %1 10").arg( escapedKey );

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


void MagnatuneStore::purchaseCurrentTrackAlbum()
{
    //get current track
    Meta::TrackPtr track = The::engineController()->currentTrack();

    //check if this is indeed a magnatune track
    Meta::SourceInfoCapability *sic = track->create<Meta::SourceInfoCapability>();
    if( sic )
    {
        //is the source defined
        QString source = sic->sourceName();
        if ( source != "Magnatune.com" ) {
            //not a Magnatune track, so don't bother...
            delete sic;
            return;
        }
        delete sic;
    } else {
        //not a Magnatune track, so don't bother...
        return;
    }

    //so far so good...
    //now the casting begins:

    Meta::MagnatuneTrack * magnatuneTrack = dynamic_cast<Meta::MagnatuneTrack *> ( track.data() );
    if ( !magnatuneTrack )
        return;

    Meta::MagnatuneAlbum * magnatuneAlbum = dynamic_cast<Meta::MagnatuneAlbum *> ( magnatuneTrack->album().data() );
    if ( !magnatuneAlbum )
        return;

    if ( !m_purchaseHandler )
    {
        m_purchaseHandler = new MagnatunePurchaseHandler();
        m_purchaseHandler->setParent( this );
        connect( m_purchaseHandler, SIGNAL( purchaseCompleted( bool ) ), this, SLOT( purchaseCompleted( bool ) ) );
    }

    m_purchaseHandler->purchaseAlbum( magnatuneAlbum );
}


void MagnatuneStore::checkForUpdates()
{
    m_updateTimestampDownloadJob = KIO::storedGet( KUrl( "http://magnatune.com/info/last_update_timestamp" ), KIO::Reload, KIO::HideProgressInfo );
    connect( m_updateTimestampDownloadJob, SIGNAL( result( KJob * ) ), SLOT( timestampDownloadComplete( KJob *  ) ) );
}


void MagnatuneStore::timestampDownloadComplete( KJob *  job )
{
    DEBUG_BLOCK
    
    if ( !job->error() == 0 )
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


void MagnatuneStore::moodyTracksReady( Meta::TrackList tracks )
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
    QStringList args = message.split( ' ', QString::SkipEmptyParts );

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

void MagnatuneStore::purchase( const QString &sku )
{
    DEBUG_BLOCK
    debug() << "sku: " << sku;
    MagnatuneDatabaseWorker * databaseWorker = new MagnatuneDatabaseWorker();
    databaseWorker->fetchAlbumBySku( sku, m_registry );
    connect( databaseWorker, SIGNAL( gotAlbumBySku( Meta::MagnatuneAlbum * ) ), this, SLOT( purchase( Meta::MagnatuneAlbum * ) ) );
    
    ThreadWeaver::Weaver::instance()->enqueue( databaseWorker );
}

void MagnatuneStore::addToFavorites( const QString &sku )
{
    DEBUG_BLOCK
    MagnatuneConfig config;

    if( !config.isMember() )
        return;

    QString url = "http://%1:%2@%3.magnatune.com/member/favorites?action=add_api&sku=%4";
    url = url.arg( config.username(), config.password(), config.membershipType(), sku );

    debug() << "favorites url: " << url;

    m_favoritesJob = KIO::storedGet( KUrl( url ), KIO::Reload, KIO::HideProgressInfo );
    connect( m_favoritesJob, SIGNAL( result( KJob * ) ), SLOT( favoritesResult( KJob *  ) ) );
}

void MagnatuneStore::removeFromFavorites( const QString &sku )
{
    DEBUG_BLOCK
    MagnatuneConfig config;

    if( !config.isMember() )
        return;

    QString url = "http://%1:%2@%3.magnatune.com/member/favorites?action=remove_api&sku=%4";
    url = url.arg( config.username(), config.password(), config.membershipType(), sku );

    debug() << "favorites url: " << url;

    m_favoritesJob = KIO::storedGet( KUrl( url ), KIO::Reload, KIO::HideProgressInfo );
    connect( m_favoritesJob, SIGNAL( result( KJob * ) ), SLOT( favoritesResult( KJob *  ) ) );
}

void MagnatuneStore::favoritesResult( KJob* addToFavoritesJob )
{
    if( addToFavoritesJob != m_favoritesJob )
        return;

    QString result = m_favoritesJob->data();

    The::statusBar()->longMessage( result );

    //show the favorites page
    showFavoritesPage();
}



#include "MagnatuneStore.moc"


