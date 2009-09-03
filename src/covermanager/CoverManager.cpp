/****************************************************************************************
 * Copyright (c) 2004 Pierpaolo Di Panfilo <pippo_dp@libero.it>                         *
 * Copyright (c) 2005 Isaiah Damron <xepo@trifault.net>                                 *
 * Copyright (c) 2007 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
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

#include "CoverManager.h"
#include "CoverViewDialog.h"

#include "Amarok.h"
#include "amarokconfig.h"
#include "collection/Collection.h"
#include "CollectionManager.h"
#include "Debug.h"
#include "meta/capabilities/CurrentTrackActionsCapability.h"
#include "meta/Meta.h"
#include "QueryMaker.h"
#include <config-amarok.h>
#include "PixmapViewer.h"
#include "playlist/PlaylistController.h"
#include "widgets/LineEdit.h"

#include <KIO/NetAccess>
#include <KLocale>
#include <KMenu>    //showCoverMenu()
#include <KPushButton>
#include <KSqueezedTextLabel> //status label
#include <KStatusBar>
#include <KToolBar>
#include <KVBox>

#include <QAction>
#include <QDesktopWidget>
#include <QProgressBar>
#include <QProgressDialog>
#include <QStringList>
#include <QTimer>    //search filter timer
#include <QTreeWidget>
#include <QTreeWidgetItem>


static QString artistToSelectInInitFunction;
CoverManager *CoverManager::s_instance = 0;

class ArtistItem : public QTreeWidgetItem
{
    public:
        ArtistItem( QTreeWidget *parent, Meta::ArtistPtr artist )
            : QTreeWidgetItem( parent )
            , m_artist( artist )
        {
            setText( 0, artist->prettyName() );
        }

        ArtistItem( const QString &text, QTreeWidget *parent = 0 )
            : QTreeWidgetItem( parent )
            , m_artist( 0 )
        {
            setText( 0, text );
        }

        Meta::ArtistPtr artist() const { return m_artist; }

    private:
        Meta::ArtistPtr m_artist;
};


CoverManager::CoverManager()
        : QSplitter( 0 )
        , m_timer( new QTimer( this ) )    //search filter timer
        , m_fetchingCovers( 0 )
        , m_coversFetched( 0 )
        , m_coverErrors( 0 )
{
    DEBUG_BLOCK

    setObjectName( "TheCoverManager" );

    s_instance = this;

    // Sets caption and icon correctly (needed e.g. for GNOME)
    kapp->setTopWidget( this );
    setWindowTitle( KDialog::makeStandardCaption( i18n("Cover Manager") ) );
    setAttribute( Qt::WA_DeleteOnClose );
    setContentsMargins( 4, 4, 4, 4 );

    //artist listview
    m_artistView = new QTreeWidget( this );
    m_artistView->setHeaderLabel( i18n( "Albums By" ) );
    m_artistView->setSortingEnabled( false );
    m_artistView->setTextElideMode( Qt::ElideRight );
    m_artistView->setMinimumWidth( 140 );
    m_artistView->setColumnCount( 1 );

    setSizes( QList<int>() << 120 << width() - 120 );

    ArtistItem *item = 0;
    item = new ArtistItem( i18n( "All Artists" ) );
    item->setIcon(0, SmallIcon( "media-optical-audio-amarok" ) );
    m_items.append( item );

    Amarok::Collection *coll = CollectionManager::instance()->primaryCollection();
    QueryMaker *qm = coll->queryMaker();
    qm->setAutoDelete( true );
    qm->setQueryType( QueryMaker::Artist );
    qm->setAlbumQueryMode( QueryMaker::OnlyNormalAlbums );
    qm->orderBy( Meta::valArtist );

    connect( qm, SIGNAL( newResultReady( QString, Meta::ArtistList ) ),
             this, SLOT( slotArtistQueryResult( QString, Meta::ArtistList ) ) );

    connect( qm, SIGNAL( queryDone() ), this, SLOT( slotContinueConstruction() ) );

    qm->run();
}

void
CoverManager::slotArtistQueryResult( QString collectionId, Meta::ArtistList artists ) //SLOT
{
    DEBUG_BLOCK
    Q_UNUSED( collectionId );
    foreach( Meta::ArtistPtr artist, artists )
        m_artistList << artist;
}

void
CoverManager::slotContinueConstruction() //SLOT
{
    DEBUG_BLOCK
    foreach( Meta::ArtistPtr artist, m_artistList )
    {
        ArtistItem* item = new ArtistItem( m_artistView, artist );
        item->setIcon( 0, SmallIcon( "view-media-artist-amarok" ) );
        m_items.append( item );
    }
    m_artistView->insertTopLevelItems( 0, m_items );

    //TODO: Port Compilation listing
//     ArtistItem *last = static_cast<ArtistItem *>(m_artistView->item( m_artistView->count() - 1));
//     QueryBuilder qb;
//     qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName );
//     qb.groupBy( QueryBuilder::tabAlbum, QueryBuilder::valName );
//     qb.setOptions( QueryBuilder::optOnlyCompilations );
//     qb.setLimit( 0, 1 );
//     if ( qb.run().count() ) {
//         item = new ArtistItem( m_artistView, last, i18n( "Various Artists" ) );
//         item->setPixmap( 0, SmallIcon("personal") );
//     }

    KVBox *vbox = new KVBox( this );
    KHBox *hbox = new KHBox( vbox );

    vbox->setSpacing( 4 );
    hbox->setSpacing( 4 );

    { //<Search LineEdit>
        m_searchEdit = new Amarok::LineEdit( hbox );
        m_searchEdit->setClickMessage( i18n( "Enter search terms here" ) );
        m_searchEdit->setFrame( true );

        m_searchEdit->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Minimum);
        m_searchEdit->setClearButtonShown( true );

        // TODO FIX FUCKED UP ENGLISH
        m_searchEdit->setToolTip( i18n( "Enter space-separated terms to search in the albums" ) );

        hbox->setStretchFactor( m_searchEdit, 1 );
    } //</Search LineEdit>

    // view menu
    m_viewMenu = new KMenu( this );
    m_selectAllAlbums          = m_viewMenu->addAction( i18n("All Albums"),           this, SLOT( slotShowAllAlbums() ) );
    m_selectAlbumsWithCover    = m_viewMenu->addAction( i18n("Albums With Cover"),    this, SLOT( slotShowAlbumsWithCover() ) );
    m_selectAlbumsWithoutCover = m_viewMenu->addAction( i18n("Albums Without Cover"), this, SLOT( slotShowAlbumsWithoutCover() ) );

    QActionGroup *viewGroup = new QActionGroup( this );
    viewGroup->setExclusive( true );
    viewGroup->addAction( m_selectAllAlbums );
    viewGroup->addAction( m_selectAlbumsWithCover );
    viewGroup->addAction( m_selectAlbumsWithoutCover );
    m_selectAllAlbums->setChecked( true );

    //fetch missing covers button
    m_fetchButton = new KPushButton( KGuiItem( i18n("Fetch Missing Covers"), "get-hot-new-stuff-amarok" ), hbox );
    connect( m_fetchButton, SIGNAL(clicked()), SLOT(fetchMissingCovers()) );

    //cover view
    m_coverView = new CoverView( vbox );

    //status bar
    KStatusBar *m_statusBar = new KStatusBar( vbox );

    m_statusLabel = new KSqueezedTextLabel( m_statusBar );
    m_statusLabel->setIndent( 3 );
    m_progressBox = new KHBox( m_statusBar );

    m_statusBar->addWidget( m_statusLabel, 4 );
    m_statusBar->addPermanentWidget( m_progressBox, 1 );

    KPushButton *stopButton = new KPushButton( KGuiItem(i18n("Abort"), "stop"), m_progressBox );
    connect( stopButton, SIGNAL(clicked()), SLOT(stopFetching()) );
    m_progress = new QProgressBar( m_progressBox );
    m_progress->setTextVisible( true );

    const int h = m_statusLabel->height() + 3;
    m_statusLabel->setFixedHeight( h );
    m_progressBox->setFixedHeight( h );
    m_progressBox->hide();


    // signals and slots connections
    connect( m_artistView, SIGNAL(itemSelectionChanged() ),
                           SLOT( slotArtistSelected() ) );
    connect( m_coverView,  SIGNAL(itemActivated( QListWidgetItem* )),
                           SLOT(coverItemExecuted( QListWidgetItem* )) );
    connect( m_timer,      SIGNAL(timeout()),
                           SLOT(slotSetFilter()) );
    connect( m_searchEdit, SIGNAL(textChanged( const QString& )),
                           SLOT(slotSetFilterTimeout()) );

    m_currentView = AllAlbums;

    QSize size = QApplication::desktop()->screenGeometry( this ).size() / 1.5;
    QSize sz = Amarok::config( "Cover Manager" ).readEntry( "Window Size", size );
    resize( sz.width(), sz.height() );

    show();

    m_fetcher = The::coverFetcher();

    QTimer::singleShot( 0, this, SLOT(init()) );
}


CoverManager::~CoverManager()
{
    DEBUG_BLOCK

    Amarok::config( "Cover Manager" ).writeEntry( "Window Size", size() );    s_instance = 0;
}


void CoverManager::init()
{
    DEBUG_BLOCK

    QTreeWidgetItem *item = 0;

    int i = 0;
    if ( !artistToSelectInInitFunction.isEmpty() )
    {
        for( item = m_artistView->invisibleRootItem()->child( 0 );
             i < m_artistView->invisibleRootItem()->childCount();
             item = m_artistView->invisibleRootItem()->child( i++ ) )
        {
            if ( item->text( 0 ) == artistToSelectInInitFunction )
                break;
        }
    }

    if ( item == 0 )
        item = m_artistView->invisibleRootItem()->child( 0 );

    item->setSelected( true );

}


void CoverManager::viewCover( Meta::AlbumPtr album, QWidget *parent ) //static
{
    //QDialog means "escape" works as expected
    QDialog *dialog = new CoverViewDialog( album, parent );
    dialog->show();
}

void
CoverManager::metadataChanged( Meta::AlbumPtr album )
{
    DEBUG_BLOCK

    ArtistItem *selectedItem = static_cast<ArtistItem*>(m_artistView->selectedItems().first());
    if( selectedItem->text( 0 ) != i18n( "All Artists" ) )
    {
        if ( album->albumArtist() != selectedItem->artist() )
            return;
    }
    else
    {
        foreach( CoverViewItem *item, m_coverItems )
        {
            if( album->name() == item->albumPtr()->name() )
                item->loadCover();
        }
        // Update have/missing count.
        updateStatusBar();
    }
}

void CoverManager::fetchMissingCovers() //SLOT
{
    DEBUG_BLOCK

    int i = 0;
    for ( QListWidgetItem *item = m_coverView->item( i );
          i < m_coverView->count();
          item =  m_coverView->item( i++ ) )
    {
        CoverViewItem *coverItem = static_cast<CoverViewItem*>( item );
        if( !coverItem->hasCover() ) {
            m_fetchCovers += coverItem->albumPtr();
        }
    }

    m_fetcher->queueAlbums( m_fetchCovers );

    updateStatusBar();
    m_fetchButton->setEnabled( false );

}

void CoverManager::showOnce( const QString &artist )
{
    if( !s_instance )
    {
        artistToSelectInInitFunction = artist;
        new CoverManager();
    }
    else
    {
        s_instance->activateWindow();
        s_instance->raise();
    }
}

void CoverManager::slotArtistSelected() //SLOT
{
    m_coverView->clear();
    m_coverItems.clear();

    // reset current view mode state to "AllAlbum" which is the default on artist change in left panel
    m_currentView = AllAlbums;
    m_selectAllAlbums->trigger();
    m_selectAllAlbums->setChecked( true );

    //Extra time for the sake of init() and aesthetics
    QTimer::singleShot( 0, this, SLOT( slotArtistSelectedContinue() ) );
}


void CoverManager::slotArtistSelectedContinue() //SLOT
{
    DEBUG_BLOCK
    QTreeWidgetItem *item = m_artistView->selectedItems().first();
    ArtistItem *artistItem = static_cast< ArtistItem* >(item);
    Meta::ArtistPtr artist = artistItem->artist();

    m_progressDialog = new QProgressDialog( this );
    m_progressDialog->setLabelText( i18n("Loading Thumbnails...") );
    m_progressDialog->setWindowModality( Qt::WindowModal );

    //this can be a bit slow
    QApplication::setOverrideCursor( Qt::WaitCursor );

    Amarok::Collection *coll = CollectionManager::instance()->primaryCollection();

    QueryMaker *qm = coll->queryMaker();
    qm->setAutoDelete( true );
    qm->setQueryType( QueryMaker::Album );
    qm->orderBy( Meta::valAlbum );
    if( item != m_artistView->invisibleRootItem()->child( 0 ) )
        qm->addMatch( artist );
    else
        qm->excludeFilter( Meta::valAlbum, QString(), true, true );

    m_albumList.clear();

    connect( qm, SIGNAL( newResultReady( QString, Meta::AlbumList ) ),
             this, SLOT( slotAlbumQueryResult( QString, Meta::AlbumList ) ) );

    connect( qm, SIGNAL( queryDone() ), this, SLOT( slotArtistSelectedContinueAgain() ) );

    qm->run();
}

void
CoverManager::slotAlbumQueryResult( QString collectionId, Meta::AlbumList albums ) //SLOT
{
    DEBUG_BLOCK
    Q_UNUSED( collectionId );
    m_albumList += albums;
}

void CoverManager::slotArtistSelectedContinueAgain() //SLOT
{
    DEBUG_BLOCK
    //TODO: Port 2.0
    //also retrieve compilations when we're showing all items (first treenode) or
    //"Various Artists" (last treenode)
//     if ( item == m_artistView->firstChild() || item == m_artistView->lastChild() )
//     {
//         QStringList cl;
//
//         qb.clear();
//         qb.addReturnValue( QueryBuilder::tabAlbum,  QueryBuilder::valName );
//
//         qb.excludeMatch( QueryBuilder::tabAlbum, i18n( "Unknown" ) );
//         qb.sortBy( QueryBuilder::tabAlbum, QueryBuilder::valName );
//         qb.setOptions( QueryBuilder::optRemoveDuplicates );
//         qb.setOptions( QueryBuilder::optOnlyCompilations );
//         cl = qb.run();
//
//         for( int i = 0; i < cl.count(); i++ ) {
//             albums.append( i18n( "Various Artists" ) );
//             albums.append( cl[ i ] );
//         }
//     }

    QApplication::restoreOverrideCursor();

    m_progressDialog->setMaximum( m_albumList.count() );

    uint x = 0;
    foreach( Meta::AlbumPtr album, m_albumList )
    {
        CoverViewItem *item = new CoverViewItem( m_coverView, album );
        m_coverItems.append( item );
        item->loadCover();

        if ( ++x % 50 == 0 )
        {
            m_progressDialog->setValue( x );
            kapp->processEvents(); // QProgressDialog also calls this, but not always due to Qt bug!

            //only worth testing for after processEvents() is called
            if( m_progressDialog->wasCanceled() )
               break;
        }
    }

    updateStatusBar();
    delete m_progressDialog;
}

// called when a cover item is clicked
void CoverManager::coverItemExecuted( QListWidgetItem *item ) //SLOT
{
    #define item static_cast<CoverViewItem*>(item)

    if( !item ) return;

    item->setSelected( true );
    if ( item->hasCover() )
        viewCover( item->albumPtr(), this );
    else
        m_fetcher->manualFetch( item->albumPtr() );

    #undef item
}


void CoverManager::slotSetFilter() //SLOT
{
    m_filter = m_searchEdit->text();

    m_coverView->clearSelection();
    uint i = 0;
    QListWidgetItem *item = m_coverView->item( i );
    while ( item )
    {
        QListWidgetItem *tmp = m_coverView->item( i + 1 );
        m_coverView->takeItem( i );
        item = tmp;
    }

    foreach( QListWidgetItem *item, m_coverItems )
    {
        CoverViewItem *coverItem = static_cast<CoverViewItem*>(item);
        if( coverItem->album().contains( m_filter, Qt::CaseInsensitive ) || coverItem->artist().contains( m_filter, Qt::CaseInsensitive ) )
            m_coverView->insertItem( m_coverView->count() -  1, item );
    }

    updateStatusBar();
}


void CoverManager::slotSetFilterTimeout() //SLOT
{
    if ( m_timer->isActive() ) m_timer->stop();
    m_timer->setSingleShot( true );
    m_timer->start( 180 );
}

void CoverManager::changeView( int id  ) //SLOT
{
    DEBUG_BLOCK

    if( m_currentView == id )
        return;

    //clear the iconview without deleting items
    m_coverView->clearSelection();

    while ( m_coverView->count() > 0 )
       m_coverView->takeItem( 0 );

    foreach( QListWidgetItem *item, m_coverItems )
    {
        bool show = false;
        CoverViewItem *coverItem = static_cast<CoverViewItem*>(item);
        if( !m_filter.isEmpty() )
        {
            if( !coverItem->album().contains( m_filter, Qt::CaseInsensitive ) &&
                !coverItem->artist().contains( m_filter, Qt::CaseInsensitive ) )
                continue;
        }

        if( id == AllAlbums )    //show all albums
            show = true;
        else if( id == AlbumsWithCover && coverItem->hasCover() )    //show only albums with cover
            show = true;
        else if( id == AlbumsWithoutCover && !coverItem->hasCover() )//show only albums without cover
            show = true;

        if( show )
            m_coverView->insertItem( m_coverView->count() - 1, item );
    }
    m_currentView = id;
}

void CoverManager::coverFetched( const QString &artist, const QString &album ) //SLOT
{
    loadCover( artist, album );
    m_coversFetched++;
    updateStatusBar();
}


void CoverManager::coverRemoved( const QString &artist, const QString &album ) //SLOT
{
    loadCover( artist, album );
    m_coversFetched--;
    updateStatusBar();
}


void CoverManager::coverFetcherError()
{
    DEBUG_FUNC_INFO

    m_coverErrors++;
    updateStatusBar();
}


void CoverManager::stopFetching()
{
    DEBUG_FUNC_INFO

    updateStatusBar();
}

// PRIVATE

void CoverManager::loadCover( const QString &artist, const QString &album )
{
    foreach( QListWidgetItem *item, m_coverItems )
    {
        CoverViewItem *coverItem = static_cast<CoverViewItem*>(item);
        if ( album == coverItem->album() && ( artist == coverItem->artist() || ( artist.isEmpty() && coverItem->artist().isEmpty() ) ) )
        {
            coverItem->loadCover();
            return;
        }
    }
}

void CoverManager::playSelectedAlbums()
{
    Amarok::Collection *coll = CollectionManager::instance()->primaryCollection();
    QueryMaker *qm = coll->queryMaker();
    foreach( CoverViewItem *item, selectedItems() )
    {
        qm->addMatch( item->albumPtr() );
    }
    The::playlistController()->insertOptioned( qm, Playlist::AppendAndPlay );
}

QList<CoverViewItem*> CoverManager::selectedItems()
{
    QList<CoverViewItem*> selectedItems;
    foreach( QListWidgetItem *item, m_coverView->selectedItems() )
        selectedItems.append( static_cast<CoverViewItem*>(item) );

    return selectedItems;
}


void CoverManager::updateStatusBar()
{
    QString text;

    //cover fetching info
    if( m_fetchingCovers )
    {
        //update the progress bar
        m_progress->setMaximum( m_fetchingCovers );
        m_progress->setValue( m_coversFetched + m_coverErrors );
        if( m_progressBox->isHidden() )
            m_progressBox->show();

        //update the status text
        if( m_coversFetched + m_coverErrors >= m_progress->value() ) {
            //fetching finished
            text = i18nc( "The fetching is done.", "Finished." );
            if( m_coverErrors )
                text += i18np( " Cover not found", " <b>%1</b> covers not found", m_coverErrors );
            //reset counters
            m_fetchingCovers = 0;
            m_coversFetched = 0;
            m_coverErrors = 0;
            QTimer::singleShot( 2000, this, SLOT( updateStatusBar() ) );
        }

        if( m_fetchingCovers == 1 )
        {
            foreach( Meta::AlbumPtr album, m_fetchCovers )
            {
                if( album->albumArtist()->prettyName().isEmpty() )
                    text = i18n( "Fetching cover for %1..." , album->prettyName() );
                else
                    text = i18n( "Fetching cover for %1 - %2...",
                                 album->albumArtist()->prettyName(),
                                 album->prettyName() );
            }
        }
        else if( m_fetchingCovers )
        {
            text = i18np( "Fetching 1 cover: ", "Fetching <b>%1</b> covers... : ", m_fetchingCovers );
            if( m_coversFetched )
                text += i18np( "1 fetched", "%1 fetched", m_coversFetched );
            if( m_coverErrors )
            {
                if( m_coversFetched )
                    text += i18n(" - ");
                text += i18np( "1 not found", "%1 not found", m_coverErrors );
            }
            if( m_coversFetched + m_coverErrors == 0 )
                text += i18n( "Connecting..." );
        }
    }
    else
    {
        m_coversFetched = 0;
        m_coverErrors = 0;

        uint totalCounter = 0, missingCounter = 0;

        if( m_progressBox->isVisible() )
            m_progressBox->hide();

        //album info
        int i = 0;
        for( QListWidgetItem *item = m_coverView->item( i );
             i < m_coverView->count();
             item = m_coverView->item( i++ ) )
        {
            totalCounter++;
            if( !static_cast<CoverViewItem*>( item )->hasCover() )
                missingCounter++;    //counter for albums without cover
        }

        if( !m_filter.isEmpty() )
            text = i18np( "1 result for \"%2\"", "%1 results for \"%2\"", totalCounter, m_filter );
        else if( m_artistView->selectedItems().count() > 0 )
        {
            text = i18np( "1 album", "%1 albums", totalCounter );
            if( m_artistView->selectedItems().first() != m_artistView->invisibleRootItem()->child( 0 ) ) //showing albums by an artist
            {
                QString artist = m_artistView->selectedItems().first()->text(0);
                if( artist.endsWith( ", The" ) )
                    Amarok::manipulateThe( artist, false );
                text += i18n( " by " ) + artist;
            }
        }

        if( missingCounter )
            text += i18n(" - ( <b>%1</b> without cover )", missingCounter );

        m_fetchButton->setEnabled( missingCounter );
    }

    m_statusLabel->setText( text );
}

void CoverManager::setStatusText( QString text )
{
    m_oldStatusText = m_statusLabel->text();
    m_statusLabel->setText( text );
}
//////////////////////////////////////////////////////////////////////
//    CLASS CoverView
/////////////////////////////////////////////////////////////////////

CoverView::CoverView( QWidget *parent, const char *name, Qt::WFlags f )
    : QListWidget( parent )
{
    DEBUG_BLOCK

    setObjectName( name );
    setWindowFlags( f );
    setViewMode( QListView::IconMode );
    setMovement( QListView::Static );
    setResizeMode( QListView::Adjust );
    setSelectionMode( QAbstractItemView::ExtendedSelection );
    setWrapping( true );
    setSpacing( 4 );
    setWordWrap( true );
    setIconSize( QSize(100,100) );
    setTextElideMode( Qt::ElideRight );
    setMouseTracking( true );
    setContextMenuPolicy( Qt::DefaultContextMenu );

    connect( this, SIGNAL( itemEntered( QListWidgetItem * ) ), SLOT( setStatusText( QListWidgetItem * ) ) );
    connect( this, SIGNAL( viewportEntered() ), CoverManager::instance(), SLOT( updateStatusBar() ) );
}

void CoverView::contextMenuEvent( QContextMenuEvent *event )
{
    CoverViewItem* item = dynamic_cast<CoverViewItem*>( itemAt( event->pos() ) );
    if( item )
    {
        KMenu menu;
        menu.addTitle( i18n( "Cover Image" ) );

        Meta::AlbumPtr album = item->albumPtr();
        if( album )
        {
            Meta::CustomActionsCapability *cac = album->create<Meta::CustomActionsCapability>();
            if( cac )
            {
                QList<QAction *> actions = cac->customActions();

                menu.addSeparator();
                foreach( QAction *action, actions )
                    menu.addAction( action );
            }
        }
        menu.exec( event->globalPos() );
    }
    else
        QListView::contextMenuEvent( event );
    // TODO
    // Multiple selections
    // Play, Load and Append to playlist actions
    // Set custom cover action
}

void CoverView::setStatusText( QListWidgetItem *item )
{
    #define item static_cast<CoverViewItem *>( item )
    if ( !item )
        return;

    bool sampler = false;
    //compilations have valDummy for artist.  see QueryBuilder::addReturnValue(..) for explanation
    //FIXME: Don't rely on other independent code, use an sql query
    if( item->artist().isEmpty() )
        sampler = true;

    QString tipContent = i18n( "%1 - %2", sampler ? i18n("Various Artists") : item->artist() , item->album() );

    CoverManager::instance()->setStatusText( tipContent );

    #undef item
}

//////////////////////////////////////////////////////////////////////
//    CLASS CoverViewItem
/////////////////////////////////////////////////////////////////////

CoverViewItem::CoverViewItem( QListWidget *parent, Meta::AlbumPtr album )
    : QListWidgetItem( parent )
    , m_albumPtr( album)
    , m_coverPixmap()
    , m_parent( parent )
{
    m_album = album->prettyName();
    if( album->albumArtist() )
        m_artist = album->albumArtist()->prettyName();
    else
        m_artist = i18n( "No Artist" );
    setText( album->prettyName() );

    const bool isSuppressing = album->suppressImageAutoFetch();
    album->setSuppressImageAutoFetch( true );
    setIcon( album->image( 100 ) );
    album->setSuppressImageAutoFetch( isSuppressing );

    CoverManager::instance()->subscribeTo( album );
}

CoverViewItem::~CoverViewItem()
{}

bool CoverViewItem::hasCover() const
{
    return albumPtr()->hasImage();
}

void CoverViewItem::loadCover()
{
    const bool isSuppressing = m_albumPtr->suppressImageAutoFetch();
    m_albumPtr->setSuppressImageAutoFetch( true );
    m_coverPixmap = m_albumPtr->image();  //create the scaled cover
    m_albumPtr->setSuppressImageAutoFetch( isSuppressing );
    setIcon( m_coverPixmap );
}

void CoverViewItem::dragEntered()
{
    setSelected( true );
}


void CoverViewItem::dragLeft()
{
    setSelected( false );
}

#include "CoverManager.moc"

