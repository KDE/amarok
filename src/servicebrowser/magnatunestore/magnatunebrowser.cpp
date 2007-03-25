/*
Copyright (c) 2006  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public License
along with this library; see the file COPYING.LIB.  If not, write to
the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
Boston, MA 02110-1301, USA.
*/


#include "amarok.h"
#include "magnatunebrowser.h"
#include "playlist.h"
#include "magnatunedatabasehandler.h"
#include "debug.h"
#include "../../contextview/contextview.h"
#include "../../contextview/cloudbox.h"

#include <kstandarddirs.h> //locate()
#include <kurl.h>
#include <kiconloader.h>   //multiTabBar icons

#include <QGraphicsScene>
#include <QSplitter>
#include <q3dragobject.h>
#include <QLabel>
#include <QMenu>

#include <QTextStream>

#include <QStandardItemModel>
#include <QDirModel>

MagnatuneBrowser::MagnatuneBrowser( const char *name )
        : ServiceBase( "Magnatune Store" )
{
    setObjectName(name);
    DEBUG_BLOCK
    initTopPanel( );

    setShortDescription("A very friendly service!");
    setIcon( KIcon( Amarok::icon( "amarok_magnatune" ) ) );

    //QSplitter *spliter = new QSplitter( Qt::Vertical, this );

    debug() << "Magnatune browser starting..." << endl;

    //m_popupMenu = new QMenu( spliter);
    //m_infoBox = new MagnatuneArtistInfoBox( spliter, "ArtistInfoBox" );


    initBottomPanel();

    m_currentInfoUrl = "";

    m_purchaseHandler = 0;
    m_redownloadHandler = 0;

    m_purchaseInProgress = 0;

    m_currentlySelectedItem = 0;

    m_polished = false;
    polish( );  //FIXME not happening when shown for some reason

}

void MagnatuneBrowser::addTrackToPlaylist( MagnatuneTrack *item )
{
    if ( !item ) return ; // sanity check

    debug() << "Magnatune browser: adding single track" << endl;
    QString url = item->getHifiURL();
    Playlist * playlist = Playlist::instance();
    playlist->insertMedia( KUrl( url ) );
}

void MagnatuneBrowser::addAlbumToPlaylist( MagnatuneAlbum * item )
{
    if ( !item ) return ; // sanity check
    debug() << "Magnatune browser: adding album" << endl;

    MagnatuneTrackList tracks = MagnatuneDatabaseHandler::instance() ->getTracksByAlbumId( item->getId() );

    MagnatuneTrackList::iterator it;
    for ( it = tracks.begin(); it != tracks.end(); ++it )
        addTrackToPlaylist( &( *it ) );

}

void MagnatuneBrowser::addArtistToPlaylist( MagnatuneArtist *item )
{
    if ( !item ) return ; // sanity check
    debug() << "Magnatune browser: adding artist" << endl;

    MagnatuneAlbumList albums = MagnatuneDatabaseHandler::instance() ->getAlbumsByArtistId( item->getId(), "" );

    MagnatuneAlbumList::iterator it;
    for ( it = albums.begin(); it != albums.end(); ++it )
        addAlbumToPlaylist( &( *it ) );
}


/*void MagnatuneBrowser::showPopupMenu( QListWidgetItem * item, const QPoint & pos, int column )
{
    if ( !item ) return ;

    m_popupMenu->exec( pos );
}*/

void MagnatuneBrowser::addSelectionToPlaylist( )
{
    /*QListWidgetItem * selectedItem = m_contentList->selectedItems ()[0]; //FIXME!!!

    switch ( selectedItem->type() )
    {
    case 1000:
        addArtistToPlaylist( dynamic_cast<MagnatuneListViewArtistItem *>( selectedItem ) );
        break;
    case 1001:
        addAlbumToPlaylist( dynamic_cast<MagnatuneListViewAlbumItem *>( selectedItem ) );
        break;
    case 1002:
        addTrackToPlaylist( dynamic_cast<MagnatuneListViewTrackItem *>( selectedItem ) );
    }*/
}

void MagnatuneBrowser::menuAboutToShow( )
{
    m_popupMenu->clear();

   /*  QListWidgetItem * selectedItem = m_contentList->selectedItems ()[0]; //FIXME!!

    if ( !selectedItem ) return ;

    switch ( selectedItem->type() )
    {
    case 1000:
        m_popupMenu->addAction( i18n( "Add artist to playlist" ), this, SLOT( addSelectionToPlaylist() ) );
        break;
    case 1001:
        m_popupMenu->addAction( i18n( "Add album to playlist" ), this, SLOT( addSelectionToPlaylist() ) );
        m_popupMenu->addAction( i18n( "Purchase album" ), this, SLOT( purchaseSelectedAlbum() ) );
        break;
    case 1002:
        m_popupMenu->addAction( i18n( "Add track to playlist" ), this, SLOT( addSelectionToPlaylist() ) );
        m_popupMenu->addAction( i18n( "Purchase album" ), this, SLOT( purchaseAlbumContainingSelectedTrack() ) );
    }*/
}

void MagnatuneBrowser::purchaseButtonClicked( )
{

    if ( !m_purchaseInProgress && m_currentlySelectedItem != 0)
    {

        if ( m_currentlySelectedItem->getType() == MAGNATUNE_ALBUM )
            purchaseSelectedAlbum( );
        else if ( m_currentlySelectedItem->getType() == MAGNATUNE_TRACK )
            purchaseAlbumContainingSelectedTrack( );
    }
}

void MagnatuneBrowser::purchaseSelectedAlbum( )
{

   m_purchaseInProgress = true;
   m_purchaseAlbumButton->setEnabled( false );

    if ( !m_purchaseHandler ) //FIXME!!
    {
        m_purchaseHandler = new MagnatunePurchaseHandler();
        m_purchaseHandler->setParent( this );
        connect( m_purchaseHandler, SIGNAL( purchaseCompleted( bool ) ), this, SLOT( purchaseCompleted( bool ) ) );
    }

    MagnatuneAlbum * selectedAlbum = m_currentlySelectedItem->getContentUnion().albumValue;

    if (selectedAlbum)
        m_purchaseHandler->purchaseAlbum( *selectedAlbum );
}

void MagnatuneBrowser::purchaseAlbumContainingSelectedTrack( )
{

   m_purchaseInProgress = true;
   m_purchaseAlbumButton->setEnabled( false );

    if ( !m_purchaseHandler ) //FIXME!!
    {
        m_purchaseHandler = new MagnatunePurchaseHandler();
        m_purchaseHandler->setParent( this );
        connect( m_purchaseHandler, SIGNAL( purchaseCompleted( bool ) ), this, SLOT( purchaseCompleted( bool ) ) );
    }

    MagnatuneContentItem *albumContentItem = dynamic_cast<MagnatuneContentItem*>( m_currentlySelectedItem->parent() );
   
    if (!albumContentItem) {
	debug() << "dynamic_cast to albumContentItem failed!" << endl;

        m_purchaseInProgress = false;
        m_purchaseAlbumButton->setEnabled( true );
	return;
    }

    MagnatuneAlbum * selectedAlbum = albumContentItem->getContentUnion().albumValue;

    m_purchaseHandler->purchaseAlbum( *selectedAlbum );
}

void MagnatuneBrowser::initTopPanel( )
{

    KHBox *hBox = new KHBox( m_topPanel);
    //m_topPanel->setMaximumHeight( 24 );
    QLabel *label = new QLabel ( i18n( "Genre: " ), hBox );
    label->setObjectName( "genreLabel" );

    m_genreComboBox = new QComboBox;
    m_genreComboBox->setParent( hBox );
    m_genreComboBox->setObjectName( "genreComboBox" );

    updateGenreBox();

    m_advancedFeaturesButton = new QPushButton;
    m_advancedFeaturesButton->setParent( hBox );
    m_advancedFeaturesButton->setText( i18n( "Advanced" ) );
    m_advancedFeaturesButton->setObjectName( "advancedButton" );
    connect( m_advancedFeaturesButton, SIGNAL( clicked() ), this, SLOT( processRedownload() ) );

    connect( m_genreComboBox, SIGNAL( currentIndexChanged ( const QString ) ), this, SLOT( genreChanged( QString ) ) );
}

void MagnatuneBrowser::initBottomPanel()
{
    m_bottomPanel->setMaximumHeight( 54 );


    KHBox *hBox = new KHBox( m_bottomPanel);
    hBox->setMaximumHeight( 24 );
    hBox->setSpacing( 2 );
    //hBox->setMargin( 2 );

    m_purchaseAlbumButton = new QPushButton;
    m_purchaseAlbumButton->setParent( m_bottomPanel );
    m_purchaseAlbumButton->setText( i18n( "Purchase Album" ) );
    m_purchaseAlbumButton->setObjectName( "purchaseButton" );
    m_purchaseAlbumButton->setIcon( KIcon( Amarok::icon( "download" ) ) );
    m_purchaseAlbumButton->setEnabled( false );
    m_purchaseAlbumButton->setMaximumHeight( 24 );

    m_updateListButton = new QPushButton;
    m_updateListButton->setParent( hBox );
    m_updateListButton->setText( i18n( "Update" ) );
    m_updateListButton->setObjectName( "updateButton" );
    m_updateListButton->setIcon( KIcon( Amarok::icon( "rescan" ) ) );
    m_showInfoToggleButton = new QPushButton;
    m_showInfoToggleButton->setText( i18n( "Show Info" ) );
    m_showInfoToggleButton->setParent( hBox );
    m_showInfoToggleButton->setObjectName( "showInfoCheckbox" );
    m_showInfoToggleButton->setCheckable( true );
    m_showInfoToggleButton->setIcon( KIcon( Amarok::icon( "info" ) ) );
    m_showInfoToggleButton->setChecked( true );

    m_isInfoShown = true;

    connect( m_showInfoToggleButton, SIGNAL( toggled( bool ) ), this, SLOT( showInfo( bool ) ) );
    connect( m_updateListButton, SIGNAL( clicked() ), this, SLOT( updateButtonClicked() ) );
    connect( m_purchaseAlbumButton, SIGNAL( clicked() ) , this, SLOT( purchaseButtonClicked() ) );
}

void MagnatuneBrowser::updateButtonClicked()
{
    m_updateListButton->setEnabled( false );
    updateMagnatuneList();
}

bool MagnatuneBrowser::updateMagnatuneList()
{
    //download new list from magnatune

     debug() << "MagnatuneBrowser: start downloading xml file" << endl;

    m_listDownloadJob = KIO::file_copy( KUrl( "http://magnatune.com/info/album_info.xml" ), KUrl("/tmp/album_info.xml"), -1, true, false, true );
    Amarok::StatusBar::instance() ->newProgressOperation( m_listDownloadJob )
    .setDescription( i18n( "Downloading Magnatune.com Database" ) )
    .setAbortSlot( this, SLOT( listDownloadCancelled() ) );

    connect( m_listDownloadJob, SIGNAL( result( KJob * ) ),
            this, SLOT( listDownloadComplete( KJob * ) ) );


    return true;
}


void MagnatuneBrowser::listDownloadComplete( KJob * downLoadJob )
{

   debug() << "MagnatuneBrowser: xml file download complete" << endl;

    if ( downLoadJob != m_listDownloadJob )
        return ; //not the right job, so let's ignore it

    m_updateListButton->setEnabled( true );
    if ( !downLoadJob->error() == 0 )
    {
        //TODO: error handling here
        return ;
    }


    debug() << "MagnatuneBrowser: create xml parser" << endl;
    MagnatuneXmlParser * parser = new MagnatuneXmlParser( "/tmp/album_info.xml" );
    connect( parser, SIGNAL( doneParsing() ), SLOT( doneParsing() ) );

    ThreadManager::instance() ->queueJob( parser );
}

void MagnatuneBrowser::listDownloadCancelled( )
{


    Amarok::StatusBar::instance() ->endProgressOperation( m_listDownloadJob );
    m_listDownloadJob->kill();
    delete m_listDownloadJob;
    m_listDownloadJob = 0;
    debug() << "Aborted xml download" << endl;

    m_updateListButton->setEnabled( true );
}

void MagnatuneBrowser::showInfo( bool show )
{
    if ( show )
    {
        m_isInfoShown = true;
        m_infoBox->widget() ->setMaximumHeight( 2000 );
    }
    else
    {
        m_infoBox->widget() ->setMaximumHeight( 0 );
        m_isInfoShown = false;
    }
}

void MagnatuneBrowser::genreChanged( QString genre )
{
    debug() << "Genre changed to: " << genre << endl;
    static_cast< MagnatuneContentModel *>( getModel() )->setGenre( genre );
}


void MagnatuneBrowser::doneParsing()
{

    debug() << "MagnatuneBrowser: done parsing" << endl;
    updateGenreBox( );
}

void MagnatuneBrowser::updateGenreBox()
{
    const QStringList genres = MagnatuneDatabaseHandler::instance() ->getAlbumGenres();

    m_genreComboBox->clear();
    m_genreComboBox->addItem( "All" , 0 ); // should not be i18n'ed as it is
    //used as a trigger in the code in the database handler.

    oldForeach( genres )
    m_genreComboBox->addItem( ( *it ), -1 );
}

void MagnatuneBrowser::processRedownload( )
{
    debug() << "Process redownload" << endl;

    if ( m_redownloadHandler == 0 )
    {
        m_redownloadHandler = new MagnatuneRedownloadHandler( this );
    }
    m_redownloadHandler->showRedownloadDialog();
}

void MagnatuneBrowser::purchaseCompleted( bool /*success*/ )
{

    if ( m_purchaseHandler != 0 )
    {
        delete m_purchaseHandler;
        m_purchaseHandler = 0;
    }

    m_purchaseAlbumButton->setEnabled( true );
    m_purchaseInProgress = false;

    debug() << "Purchase operation complete" << endl;

    //TODO: display some kind of success dialog here?


}

void MagnatuneBrowser::slotSelectionChanged( ServiceModelItemBase * selectedItem ) {

   m_currentlySelectedItem = static_cast<MagnatuneContentItem*>( selectedItem );
   if ( ( m_currentlySelectedItem->getType() == MAGNATUNE_ALBUM ) ||  ( m_currentlySelectedItem->getType() == MAGNATUNE_TRACK )  ) {

       m_purchaseAlbumButton->setEnabled( true );
   } else {
       m_purchaseAlbumButton->setEnabled( false );
   } 


}

using namespace Context;

void MagnatuneBrowser::polish( )
{

    DEBUG_BLOCK;


    if (!m_polished) {
        m_polished = true;


        setModel(new MagnatuneContentModel ( this ) );
        //connect ( m_model, SIGNAL( infoChanged ( QString ) ), this, SLOT( infoChanged ( QString ) ) );

       
        connect ( this, SIGNAL( selectionChanged ( ServiceModelItemBase * ) ) , this, SLOT( slotSelectionChanged( ServiceModelItemBase * ) ) );

        m_contentView->setWindowTitle(QObject::tr("Simple Tree Model"));
        m_contentView->setSortingEnabled ( true );
        m_contentView->sortByColumn ( 0, Qt::AscendingOrder ); 


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

        MagnatuneMoodMap moodMap = MagnatuneDatabaseHandler::instance()->getMoodMap();

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

        int steps = 10;
        int stepBoundry = maxMoodCount / steps;


        i.toFront();
        while (i.hasNext()) {
           i.next();
           if ( i.value() < stepBoundry ) cloudBox->addText( i.key(), 8 );
           else  if ( i.value() < stepBoundry * 2 ) cloudBox->addText( i.key(), 12 );
           else  if ( i.value() < stepBoundry * 4 ) cloudBox->addText( i.key(), 16 );
           else  if ( i.value() < stepBoundry * 7 ) cloudBox->addText( i.key(), 20 );
           else cloudBox->addText( i.key(), 24 );
        }


         //ContextView::instance()->clear();
         ContextView::instance()->scene()->addItem( cloudBox );


    }


}





#include "magnatunebrowser.moc"
