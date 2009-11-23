/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#include "OpmlDirectoryService.h"

#include "Debug.h"
#include "browsers/CollectionTreeItem.h"
#include "browsers/SingleCollectionTreeItemModel.h"
#include "OpmlDirectoryInfoParser.h"
#include "OpmlParser.h"
#include "playlistmanager/PlaylistManager.h"
#include "podcasts/PodcastProvider.h"
#include "ServiceSqlRegistry.h"

#include <KTemporaryFile>
#include <threadweaver/ThreadWeaver.h>

#include <typeinfo>

using namespace Meta;

AMAROK_EXPORT_PLUGIN( OpmlDirectoryServiceFactory )

void OpmlDirectoryServiceFactory::init()
{
    ServiceBase* service = new OpmlDirectoryService( this, "OpmlDirectory", i18n( "Podcast Directory" ) );
    m_activeServices << service;
    m_initialized = true;
    emit newService( service );
}


QString OpmlDirectoryServiceFactory::name()
{
    return "OpmlDirectory";
}

KPluginInfo OpmlDirectoryServiceFactory::info()
{
    KPluginInfo pluginInfo( "amarok_service_opmldirectory.desktop", "services" );
    pluginInfo.setConfig( config() );
    return pluginInfo;
}


KConfigGroup OpmlDirectoryServiceFactory::config()
{
    return Amarok::config( "Service_OpmlDirectory" );
}


OpmlDirectoryService::OpmlDirectoryService( OpmlDirectoryServiceFactory* parent, const QString &name, const QString &prettyName )
 : ServiceBase( name, parent, true, prettyName )
 , m_currentFeed( 0 )
 , n_maxNumberOfTransactions ( 5000 )
{
    setShortDescription( i18n( "A large listing of podcasts" ) );
    setIcon( KIcon( "view-services-opml-amarok" ) );


    setLongDescription( i18n( "A comprehensive list of searchable podcasts from www.digitalpodcast.com that you can subscribe to directly from within Amarok." ) );

    KIconLoader loader;
    setImagePath( loader.iconPath( "view-services-opml-amarok", -128, true ) );
    
    m_serviceready = true;
    emit( ready() );
}


OpmlDirectoryService::~OpmlDirectoryService()
{
}

void OpmlDirectoryService::polish()
{
    generateWidgetInfo();
    if ( m_polished )
        return;

    //do not allow this content to get added to the playlist. At least not for now
    setPlayableTracks( false );

    KHBox * bottomPanelLayout = new KHBox;
    bottomPanelLayout->setParent( m_bottomPanel );

    m_updateListButton = new QPushButton;
    m_updateListButton->setParent( bottomPanelLayout );
    m_updateListButton->setText( i18nc( "Fetch new information from the website", "Update" ) );
    m_updateListButton->setObjectName( "updateButton" );
    m_updateListButton->setIcon( KIcon( "view-refresh-amarok" ) );


    m_subscribeButton = new QPushButton;
    m_subscribeButton->setParent( bottomPanelLayout );
    m_subscribeButton->setText( i18n( "Subscribe" ) );
    m_subscribeButton->setObjectName( "subscribeButton" );
    m_subscribeButton->setIcon( KIcon( "get-hot-new-stuff-amarok" ) );

    m_subscribeButton->setEnabled( false );

    connect( m_updateListButton, SIGNAL( clicked() ), this, SLOT( updateButtonClicked() ) );
    connect( m_subscribeButton, SIGNAL( clicked() ), this, SLOT( subscribe() ) );
    updateButtonClicked(); // Update when loaded.

    setInfoParser( new OpmlDirectoryInfoParser() );

    QList<int> levels;
    levels << CategoryId::Album;

    ServiceMetaFactory * metaFactory = new OpmlDirectoryMetaFactory( "opmldirectory", this );
    ServiceSqlRegistry * registry = new ServiceSqlRegistry( metaFactory );
    m_collection = new ServiceSqlCollection( "opmldirectory", "opmldirectory", metaFactory, registry );

    setModel( new SingleCollectionTreeItemModel( m_collection, levels ) );

    connect( m_contentView, SIGNAL( itemSelected( CollectionTreeItem * ) ), this, SLOT( itemSelected( CollectionTreeItem * ) ) );

    m_polished = true;
}

void OpmlDirectoryService::updateButtonClicked()
{
    m_updateListButton->setEnabled( false );

    debug() << "OpmlDirectoryService: start downloading xml file";

    KTemporaryFile tempFile;
    tempFile.setSuffix( ".gz" );
    tempFile.setAutoRemove( false );  //file will be removed in OpmlParser
    if( !tempFile.open() )
    {
        return; //error
    }

    m_tempFileName = tempFile.fileName();
    m_listDownloadJob = KIO::file_copy( KUrl( "http://www.digitalpodcast.com/opml/digitalpodcastnoadult.opml" ), KUrl( m_tempFileName ), 0700 , KIO::HideProgressInfo | KIO::Overwrite );
    The::statusBar() ->newProgressOperation( m_listDownloadJob, i18n( "Downloading Podcast Directory Database" ) )
    ->setAbortSlot( this, SLOT( listDownloadCancelled() ) );

    connect( m_listDownloadJob, SIGNAL( result( KJob * ) ),
            this, SLOT( listDownloadComplete( KJob * ) ) );

 
}

void OpmlDirectoryService::listDownloadComplete(KJob * downloadJob)
{


    if ( downloadJob != m_listDownloadJob )
        return ; //not the right job, so let's ignore it
    debug() << "OpmlDirectoryService: xml file download complete";


    //testing



    if ( !downloadJob->error() == 0 )
    {
        //TODO: error handling here
        return ;
    }


    The::statusBar()->shortMessage( i18n( "Updating the local Podcast database."  ) );
    debug() << "OpmlDirectoryService: create xml parser";
    //reset counters
    n_numberOfTransactions = m_numberOfCategories = m_numberOfFeeds = 0;

    m_dbHandler->destroyDatabase();
    m_dbHandler->createDatabase();

    OpmlParser *parser = new OpmlParser( m_tempFileName );
    connect( parser, SIGNAL( doneParsing() ), SLOT( doneParsing() ) );
    connect( parser, SIGNAL( outlineParsed( OpmlOutline* ) ),
            SLOT( outlineParsed( OpmlOutline* ) )
           );

    m_dbHandler->begin(); //start transaction (MAJOR speedup!!)
    ThreadWeaver::Weaver::instance()->enqueue( parser );
    downloadJob->deleteLater();
    m_listDownloadJob = 0;

}

void OpmlDirectoryService::listDownloadCancelled()
{

    The::statusBar()->endProgressOperation( m_listDownloadJob );
    m_listDownloadJob->kill();
    delete m_listDownloadJob;
    m_listDownloadJob = 0;
    debug() << "Aborted xml download";

    m_updateListButton->setEnabled( true );
}

void OpmlDirectoryService::doneParsing()
{
    debug() << "OpmlDirectoryService: done parsing";
    m_dbHandler->commit(); //complete transaction

    The::statusBar()->longMessage(
            i18ncp( "This string is the first part of the following example phrase: "
                "Podcast Directory update complete. Added 4 feeds in 6 categories.",
                "Podcast Directory update complete. Added 1 feed in ",
                "Podcast Directory update complete. Added %1 feeds in ", m_numberOfFeeds
              )
            + i18ncp( "This string is the second part of the following example phrase: "
                  "Podcast Directory update complete. Added 4 feeds in 6 categories.",
                  "1 category.", "%1 categories.", m_numberOfCategories
                ),
            StatusBar::Information
        );


    debug() << "OpmlParser: total number of albums: " << m_numberOfCategories;
    debug() << "OpmlParser: total number of tracks: " << m_numberOfFeeds;

    m_updateListButton->setEnabled( true );

    QFile::remove( m_tempFileName );

    //delete sender
    sender()->deleteLater();
    m_collection->emitUpdated();
}

void
OpmlDirectoryService::outlineParsed( OpmlOutline *outline )
{
    if( !outline )
    {
        error() << "NULL outline in " << __FILE__ << ":"<<__LINE__;
        return;
    }

    if( outline->hasChildren() )
    {
        QString name = outline->attributes().value( "text", "Unknown" );
        ServiceAlbumPtr currentCategory =
                ServiceAlbumPtr( new OpmlDirectoryCategory( name ) );
        m_numberOfCategories++;

        m_currentCategoryId = m_dbHandler->insertAlbum( currentCategory );
        countTransaction();
    }
    else if( outline->attributes().contains( "text" )
             && outline->attributes().contains( "url" )
    )
    {
        QString name = outline->attributes().value( "text" );
        QString url = outline->attributes().value( "url" );

        OpmlDirectoryFeedPtr currentFeed =
                OpmlDirectoryFeedPtr( new OpmlDirectoryFeed( name ) );
        currentFeed->setAlbumId( m_currentCategoryId );
        currentFeed->setUidUrl( url );
        m_numberOfFeeds++;

        m_dbHandler->insertTrack( ServiceTrackPtr::dynamicCast( currentFeed ) );
        countTransaction();
    }
}

void
OpmlDirectoryService::countTransaction()
{
    n_numberOfTransactions++;
    if ( n_numberOfTransactions >= n_maxNumberOfTransactions )
    {
        m_dbHandler->commit();
        m_dbHandler->begin();
        n_numberOfTransactions = 0;
    }
}

void OpmlDirectoryService::itemSelected( CollectionTreeItem * selectedItem ){

    DEBUG_BLOCK

    //we only enable the subscribe button if there is only one item selected and it happens to
    //be a feed
    DataPtr dataPtr = selectedItem->data();

    if ( typeid( * dataPtr.data() ) == typeid( OpmlDirectoryFeed ) )  {

        debug() << "is right type (feed)";
        OpmlDirectoryFeed * feed = static_cast<OpmlDirectoryFeed *> ( dataPtr.data() );
        m_currentFeed = feed;
        m_subscribeButton->setEnabled( true );

    } else {

        debug() << "is wrong type";
        m_currentFeed = 0;
        m_subscribeButton->setEnabled( false );

    }

    return;
}

void OpmlDirectoryService::subscribe()
{
    PodcastProvider *podcastProvider = The::playlistManager()->defaultPodcasts();
    if( podcastProvider )
    {
        if( m_currentFeed != 0 )
            podcastProvider->addPodcast( m_currentFeed->uidUrl() );
    }
    else
    {
        debug() << "PodcastChannel provider is null";
    }
}



#include "OpmlDirectoryService.moc"






