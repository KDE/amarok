/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "OpmlDirectoryService.h"

#include "Debug.h"
#include "OpmlDirectoryInfoParser.h"
#include "OpmlDirectoryXmlParser.h"
#include "playlistmanager/PlaylistManager.h"
#include "podcasts/PodcastProvider.h"
#include "ServiceSqlRegistry.h"

#include <KTemporaryFile>
#include <KRun>
#include <KShell>
#include <threadweaver/ThreadWeaver.h>

#include <typeinfo>

using namespace Meta;

AMAROK_EXPORT_PLUGIN( OpmlDirectoryServiceFactory )

void OpmlDirectoryServiceFactory::init()
{
    ServiceBase* service = new OpmlDirectoryService( this, "OpmlDirectory" );
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
    KPluginInfo pluginInfo(  "amarok_service_opmldirectory.desktop", "services" );
    pluginInfo.setConfig( config() );
    return pluginInfo;
}


KConfigGroup OpmlDirectoryServiceFactory::config()
{
    return Amarok::config( "Service_OpmlDirectory" );
}


OpmlDirectoryService::OpmlDirectoryService( OpmlDirectoryServiceFactory* parent, const QString & name )
 : ServiceBase( name, parent )
 , m_currentFeed( 0 )
{

    setShortDescription(  i18n( "A large listing of podcasts" ) );
    setIcon( KIcon( "amarok_podcast" ) );
    m_serviceready = true;
    emit( ready() );
}


OpmlDirectoryService::~OpmlDirectoryService()
{
}

void OpmlDirectoryService::polish()
{

    generateWidgetInfo();
    if ( m_polished ) return;

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

    setInfoParser( new OpmlDirectoryInfoParser() );

    //m_model = new DatabaseDrivenContentModel();
    //m_dbHandler = new OpmlDirectoryDatabaseHandler();
    //m_model->setDbHandler( m_dbHandler );

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
    tempFile.setAutoRemove( false );  //file will be removed in OpmlDirectoryXmlParser
    if( !tempFile.open() )
    {
        return; //error
    }

    m_tempFileName = tempFile.fileName();
    m_listDownloadJob = KIO::file_copy( KUrl( "http://www.digitalpodcast.com/opml/digitalpodcastnoadult.opml" ), KUrl( m_tempFileName ), 0700 , KIO::HideProgressInfo | KIO::Overwrite );
    The::statusBarNG() ->newProgressOperation( m_listDownloadJob, i18n( "Downloading OpmlDirectory Database" ) )
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


    The::statusBarNG()->shortMessage( i18n( "Updating the local OPML database."  ) );
    debug() << "OpmlDirectoryService: create xml parser";
    OpmlDirectoryXmlParser * parser = new OpmlDirectoryXmlParser( m_tempFileName );
    connect( parser, SIGNAL( doneParsing() ), SLOT( doneParsing() ) );

    ThreadWeaver::Weaver::instance()->enqueue( parser );
    downloadJob->deleteLater();
    m_listDownloadJob = 0;

}

void OpmlDirectoryService::listDownloadCancelled()
{

    The::statusBarNG()->endProgressOperation( m_listDownloadJob );
    m_listDownloadJob->kill();
    delete m_listDownloadJob;
    m_listDownloadJob = 0;
    debug() << "Aborted xml download";

    m_updateListButton->setEnabled( true );
}

void OpmlDirectoryService::doneParsing()
{
    debug() << "OpmlDirectoryService: done parsing";
    m_updateListButton->setEnabled( true );
    // model->setGenre("All");
    //delete sender
    sender()->deleteLater();
    m_collection->emitUpdated();
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
    
    PlaylistProvider *provider = The::playlistManager()->playlistProvider(
            PlaylistManager::PodcastChannel, i18n( "Local Podcasts" ) );
    if( provider )
    {
        if ( m_currentFeed != 0 ) {
            PodcastProvider * podcastProvider = The::playlistManager()->defaultPodcasts();
            podcastProvider->addPodcast( m_currentFeed->uidUrl() );
        }
    }
    else
    {
        debug() << "PodcastChannel provider is null";
    }
}



#include "OpmlDirectoryService.moc"






