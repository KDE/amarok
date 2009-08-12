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
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "JamendoService.h"

#include "collection/CollectionManager.h"
#include "Debug.h"
#include "EngineController.h"
#include "JamendoInfoParser.h"
#include "JamendoXmlParser.h"
#include "ServiceSqlRegistry.h"
#include "widgets/SearchWidget.h"

#include <KMenuBar>
#include <KRun>
#include <KShell>
#include <KTemporaryFile>
#include <threadweaver/ThreadWeaver.h>

#include <QAction>

#include <typeinfo>
#include <KAction>

using namespace Meta;

AMAROK_EXPORT_PLUGIN( JamendoServiceFactory )

void JamendoServiceFactory::init()
{
    ServiceBase* service = new JamendoService( this, "Jamendo.com" );
    m_activeServices << service;
    m_initialized = true;
    emit newService( service );
}

QString
JamendoServiceFactory::name()
{
    return "Jamendo.com";
}

KPluginInfo
JamendoServiceFactory::info()
{
    KPluginInfo pluginInfo(  "amarok_service_jamendo.desktop", "services" );
    pluginInfo.setConfig( config() );
    return pluginInfo;
}

KConfigGroup
JamendoServiceFactory::config()
{
    return Amarok::config( "Service_Jamendo" );
}

JamendoService::JamendoService( JamendoServiceFactory* parent, const QString & name)
    : ServiceBase( name, parent )
    , m_currentAlbum( 0 )
{
    setShortDescription(  i18n( "A site where artists can showcase their creations to the world" ) );
    setIcon( KIcon( "view-services-jamendo-amarok" ) );

    setLongDescription( i18n( "Jamendo.com puts artists and music lovers in touch with each other. The sites allows artists to upload their own albums to share them with the world and users to download all of them for free. Listen to and download all Jamendo.com contents from within Amarok." ) );

    KIconLoader loader;
    setImagePath( loader.iconPath( "view-services-jamendo-amarok", -128, true ) );

    ServiceMetaFactory * metaFactory = new JamendoMetaFactory( "jamendo", this );
    ServiceSqlRegistry * registry = new ServiceSqlRegistry( metaFactory );
    m_collection = new ServiceSqlCollection( "jamendo", "Jamendo.com", metaFactory, registry );
    CollectionManager::instance()->addUnmanagedCollection( m_collection, CollectionManager::CollectionDisabled );
    m_serviceready = true;
    emit( ready() );
}

JamendoService::~JamendoService()
{
}

void
JamendoService::polish()
{
    generateWidgetInfo();
    if ( m_polished )
        return;

    KHBox * bottomPanelLayout = new KHBox;
    bottomPanelLayout->setParent( m_bottomPanel );

    m_updateListButton = new QPushButton;
    m_updateListButton->setParent( bottomPanelLayout );
    m_updateListButton->setText( i18nc( "Fetch new information from the website", "Update" ) );
    m_updateListButton->setObjectName( "updateButton" );
    m_updateListButton->setIcon( KIcon( "view-refresh-amarok" ) );

    m_downloadButton = new QPushButton;
    m_downloadButton->setParent( bottomPanelLayout );
    m_downloadButton->setText( i18n( "Download" ) );
    m_downloadButton->setObjectName( "downloadButton" );
    m_downloadButton->setIcon( KIcon( "download-amarok" ) );
    m_downloadButton->setEnabled( false );

    connect( m_updateListButton, SIGNAL( clicked() ), this, SLOT( updateButtonClicked() ) );
    connect( m_downloadButton, SIGNAL( clicked() ), this, SLOT( download() ) );
    updateButtonClicked(); // Automatically fetch the database on polish.

    setInfoParser( new JamendoInfoParser() );

    QList<int> levels;
    levels << CategoryId::Genre << CategoryId::Artist << CategoryId::Album;

    setModel( new SingleCollectionTreeItemModel( m_collection, levels ) );

    connect( m_contentView, SIGNAL( itemSelected( CollectionTreeItem * ) ), this, SLOT( itemSelected( CollectionTreeItem * ) ) );

    QMenu *filterMenu = new QMenu( 0 );

//     QAction *action = filterMenu->addAction( i18n("Artist") );
//     connect( action, SIGNAL(triggered(bool)), SLOT(sortByArtist() ) );
// 
//     action = filterMenu->addAction( i18n( "Artist / Album" ) );
//     connect( action, SIGNAL(triggered(bool)), SLOT(sortByArtistAlbum() ) );
// 
//     action = filterMenu->addAction( i18n( "Album" ) );
//     connect( action, SIGNAL(triggered(bool)), SLOT( sortByAlbum() ) );

    QAction *action = filterMenu->addAction( i18n( "Genre / Artist" ) );
    connect( action, SIGNAL(triggered(bool)), SLOT( sortByGenreArtist() ) );

    action = filterMenu->addAction( i18n( "Genre / Artist / Album" ) );
    connect( action, SIGNAL(triggered(bool)), SLOT(sortByGenreArtistAlbum() ) );

    KAction *filterMenuAction = new KAction( KIcon( "preferences-other" ), i18n( "Sort Options" ), this );
    filterMenuAction->setMenu( filterMenu );

    m_searchWidget->toolBar()->addSeparator();
    m_searchWidget->toolBar()->addAction( filterMenuAction );

    QToolButton *tbutton = qobject_cast< QToolButton* >( m_searchWidget->toolBar()->widgetForAction( filterMenuAction ) );
    if( tbutton )
        tbutton->setPopupMode( QToolButton::InstantPopup );

//     m_menubar->show();

    m_polished = true;
}

void
JamendoService::updateButtonClicked()
{
    m_updateListButton->setEnabled( false );

    debug() << "JamendoService: start downloading xml file";

    KTemporaryFile tempFile;
    tempFile.setSuffix( ".gz" );
    tempFile.setAutoRemove( false );  //file will be removed in JamendoXmlParser
    if( !tempFile.open() )
        return; //error
    m_tempFileName = tempFile.fileName();
    m_listDownloadJob = KIO::file_copy( KUrl( "http://img.jamendo.com/data/dbdump_artistalbumtrack.xml.gz" ), KUrl( m_tempFileName ), 0700 , KIO::HideProgressInfo | KIO::Overwrite );

    The::statusBar()->newProgressOperation( m_listDownloadJob, i18n( "Downloading Jamendo.com Database" ) )
            ->setAbortSlot( this, SLOT( listDownloadCancelled() ) );

    connect( m_listDownloadJob, SIGNAL( result( KJob * ) ),
            this, SLOT( listDownloadComplete( KJob * ) ) );


}

void
JamendoService::listDownloadComplete(KJob * downloadJob)
{
    if( downloadJob != m_listDownloadJob )
        return ; //not the right job, so let's ignore it
    debug() << "JamendoService: xml file download complete";

    //testing
    if ( !downloadJob->error() == 0 )
    {
        //TODO: error handling here
        return;
    }

    The::statusBar()->shortMessage( i18n( "Updating the local Jamendo database."  ) );
    debug() << "JamendoService: create xml parser";
    JamendoXmlParser * parser = new JamendoXmlParser( m_tempFileName );
    connect( parser, SIGNAL( doneParsing() ), SLOT( doneParsing() ) );

    ThreadWeaver::Weaver::instance()->enqueue( parser );
    downloadJob->deleteLater();
    m_listDownloadJob = 0;
}

void
JamendoService::listDownloadCancelled()
{
    DEBUG_BLOCK
    //The::statusBarNg()->endProgressOperation( m_listDownloadJob );
    m_listDownloadJob->kill();
    delete m_listDownloadJob;
    m_listDownloadJob = 0;
    debug() << "Aborted xml download";

    m_updateListButton->setEnabled( true );
}

void
JamendoService::doneParsing()
{
    debug() << "JamendoService: done parsing";
    m_updateListButton->setEnabled( true );
    // model->setGenre("All");
    //delete sender
    sender()->deleteLater();
    m_collection->emitUpdated();
}

void
JamendoService::itemSelected( CollectionTreeItem * selectedItem )
{
    DEBUG_BLOCK

    //we only enable the download button if there is only one item selected and it happens to
    //be an album or a track
    DataPtr dataPtr = selectedItem->data();

    if ( typeid( *dataPtr.data() ) == typeid( JamendoTrack ) )
    {
        debug() << "is right type (track)";
        JamendoTrack * track = static_cast<JamendoTrack *> ( dataPtr.data() );
        m_currentAlbum = static_cast<JamendoAlbum *> ( track->album().data() );
        m_downloadButton->setEnabled( true );
    }
    else if ( typeid( * dataPtr.data() ) == typeid( JamendoAlbum ) )
    {
        m_currentAlbum = static_cast<JamendoAlbum *> ( dataPtr.data() );
        debug() << "is right type (album) named " << m_currentAlbum->name();
        m_downloadButton->setEnabled( true );
    }
    else
    {
        debug() << "is wrong type";
        m_downloadButton->setEnabled( false );
    }
    return;
}

void
JamendoService::download()
{
    if ( m_currentAlbum )
        download( m_currentAlbum );
}

void JamendoService::download( JamendoAlbum * album )
{
    DEBUG_BLOCK
    if ( !m_polished )
        polish();

    m_downloadButton->setEnabled( false );

    KTemporaryFile tempFile;
    tempFile.setSuffix( ".torrent" );
    tempFile.setAutoRemove( false );
    if( !tempFile.open() )
        return;

    m_torrentFileName = tempFile.fileName();
    debug() << "downloading " << album->oggTorrentUrl() << " to " << m_torrentFileName;
    m_torrentDownloadJob = KIO::file_copy( KUrl( album->oggTorrentUrl() ), KUrl( m_torrentFileName ), 0774 , KIO::Overwrite );
    connect( m_torrentDownloadJob, SIGNAL( result( KJob * ) ),
             this, SLOT( torrentDownloadComplete( KJob * ) ) );
}

void
JamendoService::torrentDownloadComplete(KJob * downloadJob)
{
    if( downloadJob != m_torrentDownloadJob )
        return; //not the right job, so let's ignore it

    if( !downloadJob->error() == 0 )
    {
        //TODO: error handling here
        return;
    }

    debug() << "Torrent downloaded";

    //HACK: since all we get is actually the url of the really real torrent, pass the contents of the file to the system
    //and not just the filename...

    QFile torrentFile( m_torrentFileName );
    if ( torrentFile.open( QFile::ReadOnly ) )
    {
        QString torrentLink = torrentFile.readAll();
        KRun::runUrl( KShell::quoteArg( torrentLink ), "application/x-bittorrent", 0, false );
        torrentFile.close();
    }
    downloadJob->deleteLater();
    m_torrentDownloadJob = 0;
}

void
JamendoService::downloadCurrentTrackAlbum()
{
    //get current track
    Meta::TrackPtr track = The::engineController()->currentTrack();

    //check if this is indeed a Jamendo track
    Meta::SourceInfoCapability *sic = track->create<Meta::SourceInfoCapability>();
    if( sic )
    {
        //is the source defined
        QString source = sic->sourceName();
        if ( source != "Jamendo.com" )
        {
            //not a Jamendo track, so don't bother...
            delete sic;
            return;
        }
        delete sic;
    }
    else
    {
        //not a Jamendo track, so don't bother...
        return;
    }

    //so far so good...
    //now the casting begins:

    JamendoTrack * jamendoTrack = dynamic_cast<JamendoTrack *> ( track.data() );
    if ( !jamendoTrack )
        return;

    JamendoAlbum * jamendoAlbum = dynamic_cast<JamendoAlbum *> ( jamendoTrack->album().data() );
    if ( !jamendoAlbum )
        return;

    download( jamendoAlbum );
}
#include "JamendoService.moc"

