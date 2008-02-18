/***************************************************************************
 *   Copyright (c) 2006, 2007                                              *
 *        Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                   *
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
#include "MagnatuneStore.h"


#include "amarok.h"
#include "collection/CollectionManager.h"
#include "ContextStatusBar.h"
#include "debug.h"
#include "MagnatuneConfig.h"
#include "magnatuneinfoparser.h"
#include "playlist/PlaylistModel.h"
#include "ServiceSqlRegistry.h"
#include "TheInstances.h"

//#include "../../contextview/contextview.h"
//#include "../../contextview/cloudbox.h"
//#include "../../contextview/graphicsitemfader.h"

#include <kstandarddirs.h> //locate()
#include <kurl.h>
#include <kiconloader.h>   //multiTabBar icons
#include <KTemporaryFile>

#include <QGraphicsScene>
#include <QSplitter>
#include <q3dragobject.h>
#include <QLabel>
#include <QMenu>

#include <QTextStream>

#include <QStandardItemModel>
#include <QDirModel>

#include <typeinfo>

using namespace Meta;

AMAROK_EXPORT_PLUGIN( MagnatuneServiceFactory )

void MagnatuneServiceFactory::init()
{
    DEBUG_BLOCK
    MagnatuneStore* service = new MagnatuneStore( "Magnatune.com" );
    MagnatuneConfig config;

    if ( config.isMember() )
        service->setMembership( config.membershipType(), config.username(), config.password() );

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




MagnatuneStore::MagnatuneStore( const char *name )
        : ServiceBase( name )
        , m_currentAlbum( 0 )
{
    setObjectName(name);
    DEBUG_BLOCK
    //initTopPanel( );


    setShortDescription( i18n( "The friendly record company with the motto \"We are not evil!\"" ) );
    setIcon( KIcon( "view-services-magnatune-amarok" ) );


    debug() << "Magnatune browser starting...";



    //initBottomPanel();

    m_currentInfoUrl = "";

    m_purchaseHandler = 0;
    m_redownloadHandler = 0;

    m_purchaseInProgress = 0;

//    m_currentlySelectedItem = 0;

    m_polished = false;
    //polish( );  //FIXME not happening when shown for some reason

}




void MagnatuneStore::purchase( )
{
    DEBUG_BLOCK
    if ( m_purchaseInProgress )
        return;

    debug() << "here";

    m_purchaseInProgress = true;
    m_purchaseAlbumButton->setEnabled( false );

    if ( !m_purchaseHandler )
    {
        m_purchaseHandler = new MagnatunePurchaseHandler();
        m_purchaseHandler->setParent( this );
        connect( m_purchaseHandler, SIGNAL( purchaseCompleted( bool ) ), this, SLOT( purchaseCompleted( bool ) ) );
    }

    if (m_currentAlbum != 0)
        m_purchaseHandler->purchaseAlbum( m_currentAlbum );
}



void MagnatuneStore::initTopPanel( )
{

    //connect( m_genreComboBox, SIGNAL( currentIndexChanged ( const QString ) ), this, SLOT( genreChanged( QString ) ) );
}

void MagnatuneStore::initBottomPanel()
{
    m_bottomPanel->setMaximumHeight( 72 );


    KHBox *hBoxTop = new KHBox( m_bottomPanel);
    KHBox *hBoxBottom = new KHBox( m_bottomPanel);
    hBoxTop->setMaximumHeight( 36 );
    hBoxTop->setSpacing( 2 );
    hBoxBottom->setMaximumHeight( 36 );
    hBoxBottom->setSpacing( 2 );
    //hBox->setMargin( 2 );

    m_redownloadButton = new QPushButton;
    m_redownloadButton->setParent( hBoxTop );
    m_redownloadButton->setText( i18n( "Redownload" ) );
    m_redownloadButton->setObjectName( "advancedButton" );
    connect( m_redownloadButton, SIGNAL( clicked() ), this, SLOT( processRedownload() ) );

    m_purchaseAlbumButton = new QPushButton;
    m_purchaseAlbumButton->setParent( hBoxTop );

    MagnatuneConfig config;
    if ( config.isMember() && config.membershipType() == "Download" )
        m_purchaseAlbumButton->setText( i18n( "Download Album" ) );
    else
        m_purchaseAlbumButton->setText( i18n( "Purchase Album" ) );
    
    m_purchaseAlbumButton->setObjectName( "purchaseButton" );
    m_purchaseAlbumButton->setIcon( KIcon( "get-hot-new-stuff-amarok" ) );
    m_purchaseAlbumButton->setEnabled( false );
    //m_purchaseAlbumButton->setMaximumHeight( 30 );

    m_updateListButton = new QPushButton;
    m_updateListButton->setParent( hBoxBottom );
    m_updateListButton->setText( i18n( "Update" ) );
    m_updateListButton->setObjectName( "updateButton" );
    m_updateListButton->setIcon( KIcon( "view-refresh-amarok" ) );


    m_showInfoToggleButton = new QPushButton;
    m_showInfoToggleButton->setText( i18n( "Show Info" ) );
    m_showInfoToggleButton->setParent( hBoxBottom );
    m_showInfoToggleButton->setObjectName( "showInfoCheckbox" );
    m_showInfoToggleButton->setCheckable( true );
    m_showInfoToggleButton->setIcon( KIcon( "help-about-amarok" ) );
    m_showInfoToggleButton->setChecked( true );

    m_isInfoShown = true;

    connect( m_showInfoToggleButton, SIGNAL( toggled( bool ) ), this, SLOT( showInfo( bool ) ) );
    connect( m_updateListButton, SIGNAL( clicked() ), this, SLOT( updateButtonClicked() ) );
    connect( m_purchaseAlbumButton, SIGNAL( clicked() ) , this, SLOT( purchase() ) );
}

void MagnatuneStore::updateButtonClicked()
{
    m_updateListButton->setEnabled( false );
    updateMagnatuneList();


}
bool MagnatuneStore::updateMagnatuneList()
{
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

    m_listDownloadJob = KIO::file_copy( KUrl( "http://magnatune.com/info/album_info_xml.bz2" ),  KUrl( m_tempFileName ), 0774 , KIO::Overwrite );
    Amarok::ContextStatusBar::instance()->newProgressOperation( m_listDownloadJob )
    .setDescription( i18n( "Downloading Magnatune.com Database" ) )
    .setAbortSlot( this, SLOT( listDownloadCancelled() ) );

    connect( m_listDownloadJob, SIGNAL( result( KJob * ) ),
            this, SLOT( listDownloadComplete( KJob * ) ) );


    return true;
}


void MagnatuneStore::listDownloadComplete( KJob * downLoadJob )
{

   debug() << "MagnatuneStore: xml file download complete";

    if ( downLoadJob != m_listDownloadJob )
        return ; //not the right job, so let's ignore it

    m_updateListButton->setEnabled( true );
    if ( !downLoadJob->error() == 0 )
    {
        //TODO: error handling here
        return ;
    }


    debug() << "MagnatuneStore: create xml parser";
    MagnatuneXmlParser * parser = new MagnatuneXmlParser( m_tempFileName );
    parser->setDbHandler( new MagnatuneDatabaseHandler() );
    connect( parser, SIGNAL( doneParsing() ), SLOT( doneParsing() ) );

    ThreadManager::instance() ->queueJob( parser );
}

void MagnatuneStore::listDownloadCancelled( )
{


    Amarok::ContextStatusBar::instance() ->endProgressOperation( m_listDownloadJob );
    m_listDownloadJob->kill();
    delete m_listDownloadJob;
    m_listDownloadJob = 0;
    debug() << "Aborted xml download";

    m_updateListButton->setEnabled( true );
}



void MagnatuneStore::doneParsing()
{

    debug() << "MagnatuneStore: done parsing";
    m_collection->emitUpdated();
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

    if ( m_purchaseHandler != 0 )
    {
        delete m_purchaseHandler;
        m_purchaseHandler = 0;
    }

    m_purchaseAlbumButton->setEnabled( true );
    m_purchaseInProgress = false;

    debug() << "Purchase operation complete";

    //TODO: display some kind of success dialog here?


}


void MagnatuneStore::itemSelected( CollectionTreeItem * selectedItem ){

    DEBUG_BLOCK

    //we only enable the purchase button if there is only one item selected and it happens to
    //be an album or a track
    DataPtr dataPtr = selectedItem->data();

    if ( typeid( * dataPtr.data() ) == typeid( MagnatuneTrack ) )  {

        debug() << "is right type (track)";
        MagnatuneTrack * track = static_cast<MagnatuneTrack *> ( dataPtr.data() );
        m_currentAlbum = static_cast<MagnatuneAlbum *> ( track->album().data() );
        m_purchaseAlbumButton->setEnabled( true );

    } else if ( typeid( * dataPtr.data() ) == typeid( MagnatuneAlbum ) ) {

        m_currentAlbum = static_cast<MagnatuneAlbum *> ( dataPtr.data() );
        debug() << "is right type (album) named " << m_currentAlbum->name();

        m_purchaseAlbumButton->setEnabled( true );
    } else {

        debug() << "is wrong type";
        m_purchaseAlbumButton->setEnabled( false );

    }



    return;
}


/*void MagnatuneStore::addMoodyTracksToPlaylist(QString mood)
{
   debug() << "addMoody: " << mood;
   SimpleServiceTrackList tracks = m_dbHandler->getTracksByMood( mood );

   int numberOfTracks = tracks.size();

   if ( numberOfTracks < 11 ) {

       foreach (SimpleServiceTrack * track, tracks) {
           addTrackToPlaylist( dynamic_cast<MagnatuneTrack *> ( track ) );
}
} else {

       int randomIndex;
       for ( int i = 0; i < 10; i++ ) {
           randomIndex = rand() % (numberOfTracks - i);
           addTrackToPlaylist( dynamic_cast<MagnatuneTrack *> ( tracks.takeAt( randomIndex ) ) );

}
}


    qDeleteAll( tracks );

}*/



//using namespace Context;

void MagnatuneStore::polish( )
{

    DEBUG_BLOCK;


    if (!m_polished) {
        m_polished = true;




       // m_dbHandler = new MagnatuneDatabaseHandler();


        initTopPanel( );
        initBottomPanel();

        QList<int> levels;
        //levels << CategoryId::Artist << CategoryId::Album << CategoryId::None;
        levels << CategoryId::Genre << CategoryId::Artist << CategoryId::Album;


        MagnatuneMetaFactory * metaFactory = new MagnatuneMetaFactory( "magnatune", this );

        if( m_isMember ) {
            metaFactory->setMembershipInfo( m_membershipType.toLower(), m_username, m_password );
        }


        ServiceSqlRegistry * registry = new ServiceSqlRegistry( metaFactory );
        m_collection = new ServiceSqlCollection( "magnatune", "Magnatune.com", metaFactory, registry );

        m_infoParser = new MagnatuneInfoParser();

        connect ( m_infoParser, SIGNAL( info( QString) ), this, SLOT( infoChanged( QString ) ) );


        setModel( new SingleCollectionTreeItemModel( m_collection, levels ) );
        //model->setInfoParser( infoParser );

        //setModel(new MagnatuneContentModel ( this ) );
        //setModel( model );
        //connect ( m_model, SIGNAL( infoChanged ( QString ) ), this, SLOT( infoChanged ( QString ) ) );

        connect( m_contentView, SIGNAL( itemSelected( CollectionTreeItem * ) ), this, SLOT( itemSelected( CollectionTreeItem * ) ) );

       /* m_contentView->setWindowTitle(QObject::tr("Simple Tree Model"));
        m_contentView->setSortingEnabled ( true );
        m_contentView->sortByColumn ( 0, Qt::AscendingOrder );
    */

        m_infoBox->begin( KUrl( KStandardDirs::locate( "data", "amarok/data/" ) ) );
        m_infoBox->write( "<table align='center' border='0'><tbody align='center' valign='top'>"
          "<tr align='center'><td><div align='center'>"
          "<IMG src='magnatune_logo.png' width='200' height='36' align='center' border='0'>"
          "</div></td></tr><tr><td><BR>"
        + i18n( "Welcome to Amarok's integrated Magnatune.com store. If this is the "
                "first time you run it, you must update the database by pressing the "
                "'Update' button below." )
        + "</td></tr></tbody></table>" );
        m_infoBox->end();


    }

     generateWidgetInfo();




}


/*bool MagnatuneStore::updateContextView()
{

    MagnatuneMoodMap moodMap = m_dbHandler->getMoodMap( 20 );

    int minMoodCount = 10000;
    int maxMoodCount = 0;

    //determine max and min counts
    QMapIterator<QString, int> i(moodMap);
    while (i.hasNext()) {
        i.next();
        if ( i.value() > maxMoodCount ) maxMoodCount = i.value();
        if ( i.value() < minMoodCount ) minMoodCount = i.value();
    }


    //normalize and insert into cloud view


    CloudBox * cloudBox = new CloudBox( 0, 0 );
    cloudBox->setPos( 0, 0 );
    cloudBox->setTitle( "Magnatune Moods" );

    int steps = 10;
    int stepBoundry = maxMoodCount / steps;


    i.toFront();
    while (i.hasNext()) {
        i.next();
        if ( i.value() < stepBoundry ) cloudBox->addText( i.key(), 8, this, SLOT( addMoodyTracksToPlaylist( QString ) ) );
        else  if ( i.value() < stepBoundry * 2 ) cloudBox->addText( i.key(), 12, this, SLOT( addMoodyTracksToPlaylist( QString ) ) );
        else  if ( i.value() < stepBoundry * 4 ) cloudBox->addText( i.key(), 16, this, SLOT( addMoodyTracksToPlaylist( QString ) ) );
        else  if ( i.value() < stepBoundry * 7 ) cloudBox->addText( i.key(), 20, this, SLOT( addMoodyTracksToPlaylist( QString ) ) );
        else cloudBox->addText( i.key(), 24, this, SLOT( addMoodyTracksToPlaylist( QString ) ) );
    }

    cloudBox->done();

    ContextView::instance()->clear();
    ContextView::instance()->addContextBox( cloudBox, -1 , true );

    //connect( cloudBox, SIGNAL( itemSelected( QString ) ), this, SLOT( addMoodyTracksToPlaylist( QString ) ) );

    return true;
}*/

void MagnatuneStore::setMembership(const QString & type, const QString & username, const QString & password)
{
    m_isMember = true;
    m_membershipType = type;
    m_username = username;
    m_password = password;
    
}





#include "MagnatuneStore.moc"



