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

#include <kstandarddirs.h> //locate()
#include <kurl.h>
#include <kiconloader.h>   //multiTabBar icons

#include <qsplitter.h>
#include <qdragobject.h>
#include <qlabel.h>

MagnatuneBrowser *MagnatuneBrowser::s_instance = 0;

MagnatuneBrowser::MagnatuneBrowser( const char *name )
    : QVBox( 0, name )
{
DEBUG_BLOCK
    initTopPanel( );

    QSplitter *spliter = new QSplitter( Qt::Vertical, this );

    debug() << "Magnatune browser starting..." << endl;
    m_listView = new MagnatuneListView(spliter);

    m_popupMenu     = new QPopupMenu( spliter, "MagnatuneMenu" );
    m_artistInfobox = new MagnatuneArtistInfoBox( spliter, "ArtistInfoBox" );


    initBottomPanel();

    //connect (m_listView, SIGNAL(executed(QListViewItem *)), this, SLOT(itemExecuted(QListViewItem *)));
    connect( m_listView,  SIGNAL( doubleClicked(QListViewItem *) ),
                   this,    SLOT( itemExecuted(QListViewItem *) ) );
    connect( m_listView,  SIGNAL( selectionChanged(QListViewItem *) ),
                   this,    SLOT( selectionChanged(QListViewItem *) ) );
    connect( m_listView,  SIGNAL( rightButtonClicked ( QListViewItem *, const QPoint &, int ) ),
                   this,    SLOT( showPopupMenu( QListViewItem *, const QPoint &, int) ) );
    connect( m_popupMenu, SIGNAL( aboutToShow() ),
                    this,   SLOT(menuAboutToShow()));

    updateList( );

    m_currentInfoUrl = "";

    m_artistInfobox->openURL( KURL(locate( "data", "amarok/data/magnatune_start_page.html" ) ) );

    m_purchaseHandler = 0;
}

void MagnatuneBrowser::itemExecuted( QListViewItem * item )
{
DEBUG_BLOCK
    switch( item->depth() )
    {
        case 2:
            addTrackToPlaylist( dynamic_cast<MagnatuneListViewTrackItem *>(item) );
            break;

        case 1:
            addAlbumToPlaylist( dynamic_cast<MagnatuneListViewAlbumItem *>(item) );
            break;

        case 0:
            addArtistToPlaylist( dynamic_cast<MagnatuneListViewArtistItem *>(item) );
            break;

        default:
            break;
    }
}

void MagnatuneBrowser::addTrackToPlaylist( MagnatuneTrack *item )
{
    if( !item ) return; // sanity check

    debug() << "Magnatune browser: adding single track" << endl;
    QString url = item->getHifiURL();
    Playlist * playlist =  Playlist::instance();
    playlist->insertMedia( KURL( url ) );
}

void MagnatuneBrowser::addAlbumToPlaylist( MagnatuneAlbum * item )
{
    if( !item ) return; // sanity check
    debug() << "Magnatune browser: adding album" << endl;

    MagnatuneTrackList tracks =  MagnatuneDatabaseHandler::instance()->getTracksByAlbumId(item->getId());

    MagnatuneTrackList::iterator it;
    for ( it = tracks.begin(); it != tracks.end(); ++it )
        addTrackToPlaylist( &(*it) );

}

void MagnatuneBrowser::addArtistToPlaylist( MagnatuneArtist *item )
{
    if( !item ) return; // sanity check
    debug() << "Magnatune browser: adding artist" << endl;

    MagnatuneAlbumList albums =  MagnatuneDatabaseHandler::instance()->getAlbumsByArtistId( item->getId(), "" );

    MagnatuneAlbumList::iterator it;
    for ( it = albums.begin(); it != albums.end(); ++it )
        addAlbumToPlaylist( &(*it) );
}

void MagnatuneBrowser::selectionChanged( QListViewItem *item )
{
    if( !item ) return; // sanity check

    debug() << "Selection changed..." << endl;

    if( item->depth() == 0 )
        m_purchaseAlbumButton->setEnabled( false );
    else
        m_purchaseAlbumButton->setEnabled( true );


    if( !m_isInfoShown )
        return;

    switch( item->depth() )
    {
        case 0:
            {
                MagnatuneListViewArtistItem *artistItem = dynamic_cast<MagnatuneListViewArtistItem *>(item);
                if( artistItem && m_currentInfoUrl != artistItem->getHomeURL() )
                {
                    m_currentInfoUrl = artistItem->getHomeURL();
                    m_artistInfobox->displayArtistInfo( KURL( m_currentInfoUrl ) );
                }
            }
            break;

        case 1:
            {
                MagnatuneListViewAlbumItem *albumItem = dynamic_cast<MagnatuneListViewAlbumItem *>(item);
                if( albumItem && m_currentInfoUrl != albumItem->getCoverURL() )
                {
                    m_currentInfoUrl = albumItem->getCoverURL();
                    m_artistInfobox->displayAlbumInfo(albumItem);
                }
            }
            break;

        case 2:
            {
                // a track is selected, show the corrosponding album info!
                MagnatuneListViewTrackItem *trackItem = dynamic_cast<MagnatuneListViewTrackItem*>(item);
                int albumId = trackItem->getAlbumId();
                MagnatuneAlbum album = MagnatuneDatabaseHandler::instance()->getAlbumById(albumId);
                m_artistInfobox->displayAlbumInfo(&album);
            }
            break;

        default:
            break;
    }
}

void MagnatuneBrowser::showPopupMenu( QListViewItem * item, const QPoint & pos, int /*column*/ )
{
    if( !item ) return;

    m_popupMenu->exec( pos );
}

void MagnatuneBrowser::addSelectionToPlaylist( )
{
    QListViewItem *selectedItem = m_listView->selectedItem();

    switch( selectedItem->depth() )
    {
        case 0:
            addArtistToPlaylist( dynamic_cast<MagnatuneListViewArtistItem *>(selectedItem) );
            break;
        case 1:
            addAlbumToPlaylist( dynamic_cast<MagnatuneListViewAlbumItem *>(selectedItem) );
            break;
        case 2:
            addTrackToPlaylist( dynamic_cast<MagnatuneListViewTrackItem *>(selectedItem) );
    }
    }

void MagnatuneBrowser::menuAboutToShow( )
{
    m_popupMenu->clear();

    QListViewItem  *selectedItem = m_listView->selectedItem();

    if( !selectedItem ) return;

    switch( selectedItem->depth() )
    {
        case 0:
            m_popupMenu->insertItem( i18n( "Add artist to playlist" ), this, SLOT( addSelectionToPlaylist() ) );
            break;
        case 1:
            m_popupMenu->insertItem( i18n( "Add album to playlist" ), this, SLOT( addSelectionToPlaylist() ) );
            m_popupMenu->insertItem( i18n( "Purchase album" ),        this, SLOT( purchaseAlbum()) );
            break;
        case 2:
            m_popupMenu->insertItem( i18n( "Add track to playlist" ), this, SLOT( addSelectionToPlaylist() ) );
            m_popupMenu->insertItem( i18n( "Purchase album" ), this, SLOT( purchaseAlbumContainingSelectedTrack() ) );
    }
}

void MagnatuneBrowser::purchaseButtonClicked( )
{
    if (m_listView->selectedItem()->depth() == 1)
        purchaseSelectedAlbum( );
    else if (m_listView->selectedItem()->depth() == 2)
        purchaseAlbumContainingSelectedTrack( );
}

void MagnatuneBrowser::purchaseSelectedAlbum( )
{
    if( !m_purchaseHandler )
    {
        m_purchaseHandler = new MagnatunePurchaseHandler();
        m_purchaseHandler->setParent( this );
    }

    MagnatuneListViewAlbumItem *selectedAlbum = dynamic_cast<MagnatuneListViewAlbumItem *>(m_listView->selectedItem() );

    m_purchaseHandler->purchaseAlbum( selectedAlbum );
}

void MagnatuneBrowser::purchaseAlbumContainingSelectedTrack( )
{
    if( !m_purchaseHandler )
    {
        m_purchaseHandler = new MagnatunePurchaseHandler();
        m_purchaseHandler->setParent( this );
    }

    MagnatuneListViewTrackItem * selectedTrack = (MagnatuneListViewTrackItem *) m_listView->selectedItem();

    MagnatuneAlbum album = MagnatuneDatabaseHandler::instance()->getAlbumById(selectedTrack->getAlbumId());

    m_purchaseHandler->purchaseAlbum(&album);
}

void MagnatuneBrowser::initTopPanel( )
{
    m_topPanel = new QHBox( this, "topPanel", 0 );
    m_topPanel->setMaximumHeight( 24 );
    m_topPanel->setSpacing( 2 );
    m_topPanel->setMargin( 2 );

    new QLabel ( i18n("Genre: "), m_topPanel, "genreLabel", 0 );

    m_genreComboBox = new QComboBox( false, m_topPanel, "genreComboBox" );

    updateGenreBox();

    connect( m_genreComboBox, SIGNAL( activated ( int ) ), this, SLOT( genreChanged() ) );
}

void MagnatuneBrowser::initBottomPanel()
{
    m_bottomPanel = new QVBox( this, "bottomPanel", 0 );
    m_bottomPanel->setMaximumHeight( 54 );
    m_bottomPanel->setSpacing( 2 );
    m_bottomPanel->setMargin( 2 );

    QHBox *hBox = new QHBox( m_bottomPanel, "bottomHBox", 0 );
    hBox->setMaximumHeight( 24 );
    hBox->setSpacing( 2 );
    //hBox->setMargin( 2 );

    m_purchaseAlbumButton = new QPushButton( i18n( "Purchase Album" ), m_bottomPanel, "purchaseButton" );
    m_purchaseAlbumButton->setIconSet( SmallIconSet( Amarok::icon( "download" ) ) );
    m_purchaseAlbumButton->setEnabled( false );
    m_purchaseAlbumButton->setMaximumHeight( 24 );

    m_updateListButton = new QPushButton( i18n( "Update" ), hBox, "updateButton" );
    m_updateListButton->setIconSet( SmallIconSet( Amarok::icon( "rescan" ) ) );
    m_showInfoToggleButton = new QPushButton( i18n( "Show Info" ) ,hBox, "showInfoCheckbox" );
    m_showInfoToggleButton->setToggleButton( true );
    m_showInfoToggleButton->setIconSet( SmallIconSet( Amarok::icon( "info" ) ) );
    m_showInfoToggleButton->setOn( true );

    m_isInfoShown = true;

    connect( m_showInfoToggleButton, SIGNAL( toggled( bool ) ), this, SLOT( showInfo(bool) ) );
    connect( m_updateListButton, SIGNAL( clicked() ), this, SLOT( updateButtonClicked()) );
    connect( m_purchaseAlbumButton, SIGNAL( clicked() ) , this, SLOT(purchaseButtonClicked()));
}

void MagnatuneBrowser::updateButtonClicked()
{
    updateMagnatuneList();
}

bool MagnatuneBrowser::updateMagnatuneList()
{
    //download new list from magnatune

    m_listDownloadJob = KIO::storedGet( KURL( "http://magnatune.com/info/album_info.xml" ), false, false );
    Amarok::StatusBar::instance()->newProgressOperation( m_listDownloadJob )
                                   .setDescription( i18n( "Downloading Magnatune.com Database" ) );

    connect( m_listDownloadJob, SIGNAL( result( KIO::Job* ) ), SLOT( listDownloadComplete( KIO::Job* ) ) );

    return true;
}


void MagnatuneBrowser::listDownloadComplete( KIO::Job * downLoadJob )
{
    if ( !downLoadJob->error() == 0 )
    {
        //TODO: error handling here
        return;
    }
    if ( downLoadJob != m_listDownloadJob )
        return; //not the right job, so let's ignore it

    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( downLoadJob );
    QString list = QString( storedJob->data() );


    QFile file( "/tmp/album_info.xml" );

    if( file.exists() )
        file.remove();

    if ( file.open( IO_WriteOnly ) )
    {
        QTextStream stream( &file );
        stream << list;
        file.close();
    }


    MagnatuneXmlParser * parser = new MagnatuneXmlParser( "/tmp/album_info.xml" );
    connect( parser, SIGNAL( doneParsing() ), SLOT( doneParsing() ) );

    ThreadWeaver::instance()->queueJob(parser);
}

void MagnatuneBrowser::showInfo( bool show )
{
    if( show )
    {
        m_isInfoShown = true;
        m_artistInfobox->widget()->setMaximumHeight( 2000 );
    }
    else
    {
        m_artistInfobox->widget()->setMaximumHeight( 0 );
        m_isInfoShown = false;
    }
}

void MagnatuneBrowser::updateList()
{
    const QString genre = m_genreComboBox->currentText();

    MagnatuneArtistList artists;
    artists = MagnatuneDatabaseHandler::instance()->getArtistsByGenre( genre );

    m_listView->clear();
    MagnatuneArtistList::iterator it;
    for ( it = artists.begin(); it != artists.end(); ++it )
        new MagnatuneListViewArtistItem( (*it), m_listView );

    m_listView->repaintContents();
}

void MagnatuneBrowser::genreChanged()
{
    debug() << "Genre changed..." << endl;
    updateList( );
}


void MagnatuneBrowser::doneParsing()
{
    updateList();
    updateGenreBox( );
    updateList(); // stupid stupid hack....
}

void MagnatuneBrowser::updateGenreBox()
{
    const QStringList genres = MagnatuneDatabaseHandler::instance()->getAlbumGenres();

    m_genreComboBox->clear();
    m_genreComboBox->insertItem ( "All" , 0 ); // should not be i18n'ed as it is
    //used as a trigger in the code in the database handler.

    foreach( genres )
        m_genreComboBox->insertItem( (*it), -1 );
}

#include "magnatunebrowser.moc"
