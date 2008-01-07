/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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

#include "jamendoservice.h"

#include "debug.h"
#include "JamendoInfoParser.h"
#include "jamendoxmlparser.h"
#include "ServiceSqlRegistry.h"

#include <KTemporaryFile>
#include <KRun>
#include <KShell>

#include <typeinfo>

using namespace Meta;

AMAROK_EXPORT_PLUGIN( JamendoServiceFactory )

void JamendoServiceFactory::init()
{
    ServiceBase* service = new JamendoService( "Jamendo.com" );
    emit newService( service );
}


QString JamendoServiceFactory::name()
{
    return "Jamendo.com";
}

KPluginInfo JamendoServiceFactory::info()
{
    KPluginInfo pluginInfo(  "amarok_service_jamendo.desktop", "services" );
    pluginInfo.setConfig( config() );
    return pluginInfo;
}


KConfigGroup JamendoServiceFactory::config()
{
    return Amarok::config( "Service_Jamendo" );
}


JamendoService::JamendoService(const QString & name)
 : ServiceBase( name )
 , m_currentAlbum( 0 )
{

    setShortDescription(  i18n( "A site where artists can showcase their creations to the world" ) );
    setIcon( KIcon( "amarok_jamendo" ) );

}


JamendoService::~JamendoService()
{
}

void JamendoService::polish()
{

    generateWidgetInfo();
    if ( m_polished ) return;

    KHBox * bottomPanelLayout = new KHBox;
    bottomPanelLayout->setParent( m_bottomPanel );

    m_updateListButton = new QPushButton;
    m_updateListButton->setParent( bottomPanelLayout );
    m_updateListButton->setText( i18n( "Update" ) );
    m_updateListButton->setObjectName( "updateButton" );
    m_updateListButton->setIcon( KIcon( "view-refresh-amarok" ) );


    m_downloadButton = new QPushButton;
    m_downloadButton->setParent( bottomPanelLayout );
    m_downloadButton->setText( i18n( "Download" ) );
    m_downloadButton->setObjectName( "downloadButton" );
    m_downloadButton->setIcon( KIcon( "get-hot-new-stuff-amarok" ) );

    m_downloadButton->setEnabled( false );

    connect( m_updateListButton, SIGNAL( clicked() ), this, SLOT( updateButtonClicked() ) );
    connect( m_downloadButton, SIGNAL( clicked() ), this, SLOT( download() ) );


    m_infoParser = new JamendoInfoParser();

    //TODO: move this to base class?
    connect ( m_infoParser, SIGNAL( info( QString) ), this, SLOT( infoChanged( QString ) ) );

    //m_model = new DatabaseDrivenContentModel();
    //m_dbHandler = new JamendoDatabaseHandler();
    //m_model->setDbHandler( m_dbHandler );

    QList<int> levels;
    //levels << CategoryId::Artist << CategoryId::Album << CategoryId::None;
    levels << CategoryId::Genre << CategoryId::Artist << CategoryId::Album;


    ServiceMetaFactory * metaFactory = new JamendoMetaFactory( "jamendo", this );
    ServiceSqlRegistry * registry = new ServiceSqlRegistry( metaFactory );
    m_collection = new ServiceSqlCollection( "jamendo", "Jamendo.com", metaFactory, registry );

    setModel( new SingleCollectionTreeItemModel( m_collection, levels ) );

    connect( m_contentView, SIGNAL( itemSelected( CollectionTreeItem * ) ), this, SLOT( itemSelected( CollectionTreeItem * ) ) );

    m_polished = true;



}

void JamendoService::updateButtonClicked()
{
    m_updateListButton->setEnabled( false );

    debug() << "JamendoService: start downloading xml file";

    KTemporaryFile tempFile;
    tempFile.setSuffix( ".gz" );
    tempFile.setAutoRemove( false );  //file will be removed in JamendoXmlParser
    if( !tempFile.open() )
    {
        return; //error
    }

    m_tempFileName = tempFile.fileName();
    m_listDownloadJob = KIO::file_copy( KUrl( "http://img.jamendo.com/data/dbdump.en.xml.gz" ), KUrl( m_tempFileName ), 0774 , KIO::Overwrite );
    Amarok::ContextStatusBar::instance() ->newProgressOperation( m_listDownloadJob )
    .setDescription( i18n( "Downloading Jamendo.com Database" ) )
    .setAbortSlot( this, SLOT( listDownloadCancelled() ) );

    connect( m_listDownloadJob, SIGNAL( result( KJob * ) ),
            this, SLOT( listDownloadComplete( KJob * ) ) );

 
  /* KIO::StoredTransferJob * job =  KIO::storedGet(  KUrl( "http://img.jamendo.com/data/dbdump.en.xml.gz" ) );
    Amarok::ContextStatusBar::instance() ->newProgressOperation( job )
    .setDescription( i18n( "Downloading Jamendo.com Database" ) )
    .setAbortSlot( this, SLOT( listDownloadCancelled() ) );
*/
    //return true;
}

void JamendoService::listDownloadComplete(KJob * downloadJob)
{


    if ( downloadJob != m_listDownloadJob )
        return ; //not the right job, so let's ignore it
    debug() << "JamendoService: xml file download complete";


    //testing



    if ( !downloadJob->error() == 0 )
    {
        //TODO: error handling here
        return ;
    }

    //system( "gzip -df /tmp/dbdump.en.xml.gz" ); //FIXME!!!!!!!!!

    debug() << "JamendoService: create xml parser";
    JamendoXmlParser * parser = new JamendoXmlParser( m_tempFileName );
    connect( parser, SIGNAL( doneParsing() ), SLOT( doneParsing() ) );

    ThreadManager::instance()->queueJob( parser );
    downloadJob->deleteLater();
    m_listDownloadJob = 0;

}

void JamendoService::listDownloadCancelled()
{

    Amarok::ContextStatusBar::instance()->endProgressOperation( m_listDownloadJob );
    m_listDownloadJob->kill();
    delete m_listDownloadJob;
    m_listDownloadJob = 0;
    debug() << "Aborted xml download";

    m_updateListButton->setEnabled( true );
}

void JamendoService::doneParsing()
{
    debug() << "JamendoService: done parsing";
    m_updateListButton->setEnabled( true );
    // getModel->setGenre("All");
    //delete sender
    sender()->deleteLater();
    m_collection->emitUpdated();
}

void JamendoService::itemSelected( CollectionTreeItem * selectedItem ){

    DEBUG_BLOCK

    //we only enable the download button if there is only one item selected and it happens to
    //be an album or a track
    DataPtr dataPtr = selectedItem->data();

    if ( typeid( * dataPtr.data() ) == typeid( JamendoTrack ) )  {

        debug() << "is right type (track)";
        JamendoTrack * track = static_cast<JamendoTrack *> ( dataPtr.data() );
        m_currentAlbum = static_cast<JamendoAlbum *> ( track->album().data() );
        m_downloadButton->setEnabled( true );

    } else if ( typeid( * dataPtr.data() ) == typeid( JamendoAlbum ) ) {

        m_currentAlbum = static_cast<JamendoAlbum *> ( dataPtr.data() );
        debug() << "is right type (album) named " << m_currentAlbum->name();

        m_downloadButton->setEnabled( true );
    } else {

        debug() << "is wrong type";
        m_downloadButton->setEnabled( false );

    }

    return;
}

void JamendoService::download()
{

    if ( m_currentAlbum ){

        m_downloadButton->setEnabled( false );

        KTemporaryFile tempFile;
        tempFile.setSuffix( ".torrent" );
        tempFile.setAutoRemove( false );  
        if( !tempFile.open() )
        {
            return;
        }


        m_torrentFileName = tempFile.fileName();
        m_torrentDownloadJob = KIO::file_copy( KUrl( m_currentAlbum->oggTorrentUrl() ), KUrl( m_torrentFileName ), 0774 , KIO::Overwrite );
        connect( m_torrentDownloadJob, SIGNAL( result( KJob * ) ),
                this, SLOT( torrentDownloadComplete( KJob * ) ) );


    }

}

void JamendoService::torrentDownloadComplete(KJob * downloadJob)
{


    if ( downloadJob != m_torrentDownloadJob )
        return ; //not the right job, so let's ignore it

    if ( !downloadJob->error() == 0 )
    {
        //TODO: error handling here
        return ;
    }

    debug() << "Torrent downloaded";

    KRun::runUrl( KShell::quoteArg( m_torrentFileName ), "application/x-bittorrent", 0, true );

    downloadJob->deleteLater();
    m_torrentDownloadJob = 0;
}


#include "jamendoservice.moc"






