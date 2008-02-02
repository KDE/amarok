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

#include "debug.h"
#include "OpmlDirectoryInfoParser.h"
#include "OpmlDirectoryXmlParser.h"
#include "ServiceSqlRegistry.h"

#include <KTemporaryFile>
#include <KRun>
#include <KShell>

#include <typeinfo>

using namespace Meta;

AMAROK_EXPORT_PLUGIN( OpmlDirectoryServiceFactory )

void OpmlDirectoryServiceFactory::init()
{
    ServiceBase* service = new OpmlDirectoryService( "OpmlDirectory" );
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


OpmlDirectoryService::OpmlDirectoryService(const QString & name)
 : ServiceBase( name )
 , m_currentAlbum( 0 )
{

    setShortDescription(  i18n( "A large listing of podcasts" ) );
    setIcon( KIcon( "amarok_podcast" ) );

}


OpmlDirectoryService::~OpmlDirectoryService()
{
}

void OpmlDirectoryService::polish()
{

    generateWidgetInfo();
    if ( m_polished ) return;

    KHBox * bottomPanelLayout = new KHBox;
    bottomPanelLayout->setParent( m_bottomPanel );

    m_updateListButton = new QPushButton;
    m_updateListButton->setParent( bottomPanelLayout );
    m_updateListButton->setText( i18n( "Subscribe" ) );
    m_updateListButton->setObjectName( "subscribeButton" );
    m_updateListButton->setIcon( KIcon( "view-refresh-amarok" ) );


    m_subscribeButton = new QPushButton;
    m_subscribeButton->setParent( bottomPanelLayout );
    m_subscribeButton->setText( i18n( "Download" ) );
    m_subscribeButton->setObjectName( "downloadButton" );
    m_subscribeButton->setIcon( KIcon( "get-hot-new-stuff-amarok" ) );

    m_subscribeButton->setEnabled( false );

    connect( m_updateListButton, SIGNAL( clicked() ), this, SLOT( updateButtonClicked() ) );
    connect( m_subscribeButton, SIGNAL( clicked() ), this, SLOT( subscribe() ) );


    m_infoParser = new OpmlDirectoryInfoParser();

    //TODO: move this to base class?
    connect ( m_infoParser, SIGNAL( info( QString) ), this, SLOT( infoChanged( QString ) ) );

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

void OpmlDirectoryService::subscribeButtonClicked()
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
    m_listDownloadJob = KIO::file_copy( KUrl( "http://www.digitalpodcast.com/opml/digitalpodcast.opml" ), KUrl( m_tempFileName ), 0774 , KIO::Overwrite );
    Amarok::ContextStatusBar::instance() ->newProgressOperation( m_listDownloadJob )
    .setDescription( i18n( "Downloading OpmlDirectory Database" ) )
    .setAbortSlot( this, SLOT( listDownloadCancelled() ) );

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


    debug() << "OpmlDirectoryService: create xml parser";
    OpmlDirectoryXmlParser * parser = new OpmlDirectoryXmlParser( m_tempFileName );
    connect( parser, SIGNAL( doneParsing() ), SLOT( doneParsing() ) );

    ThreadManager::instance()->queueJob( parser );
    downloadJob->deleteLater();
    m_listDownloadJob = 0;

}

void OpmlDirectoryService::listDownloadCancelled()
{

    Amarok::ContextStatusBar::instance()->endProgressOperation( m_listDownloadJob );
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
    // getModel->setGenre("All");
    //delete sender
    sender()->deleteLater();
    m_collection->emitUpdated();
}

/*void OpmlDirectoryService::itemSelected( CollectionTreeItem * selectedItem ){

    DEBUG_BLOCK

    //we only enable the download button if there is only one item selected and it happens to
    //be an album or a track
    DataPtr dataPtr = selectedItem->data();

    if ( typeid( * dataPtr.data() ) == typeid( OpmlDirectoryTrack ) )  {

        debug() << "is right type (track)";
        OpmlDirectoryTrack * track = static_cast<OpmlDirectoryTrack *> ( dataPtr.data() );
        m_currentAlbum = static_cast<OpmlDirectoryAlbum *> ( track->album().data() );
        m_downloadButton->setEnabled( true );

    } else if ( typeid( * dataPtr.data() ) == typeid( OpmlDirectoryAlbum ) ) {

        m_currentAlbum = static_cast<OpmlDirectoryAlbum *> ( dataPtr.data() );
        debug() << "is right type (album) named " << m_currentAlbum->name();

        m_downloadButton->setEnabled( true );
    } else {

        debug() << "is wrong type";
        m_downloadButton->setEnabled( false );

    }

    return;
}*/



#include "OpmlDirectoryService.moc"






