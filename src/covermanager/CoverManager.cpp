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
#include <QToolButton>
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
        , m_currentView( AllAlbums )
        , m_timer( new QTimer( this ) )    //search filter timer
        , m_fetchingCovers( false )
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
    m_artistView->setMinimumWidth( 200 );
    m_artistView->setColumnCount( 1 );
    m_artistView->setAlternatingRowColors( true );
    m_artistView->setUniformRowHeights( true );
    m_artistView->setSelectionMode( QAbstractItemView::ExtendedSelection );

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

        hbox->setStretchFactor( m_searchEdit, 1 );
    } //</Search LineEdit>

    // view menu
    m_viewButton = new KPushButton( hbox );

    m_viewMenu = new KMenu( m_viewButton );
    m_selectAllAlbums          = m_viewMenu->addAction( i18n("All Albums"),           this, SLOT( slotShowAllAlbums() ) );
    m_selectAlbumsWithCover    = m_viewMenu->addAction( i18n("Albums With Cover"),    this, SLOT( slotShowAlbumsWithCover() ) );
    m_selectAlbumsWithoutCover = m_viewMenu->addAction( i18n("Albums Without Cover"), this, SLOT( slotShowAlbumsWithoutCover() ) );

    QActionGroup *viewGroup = new QActionGroup( m_viewButton );
    viewGroup->setExclusive( true );
    viewGroup->addAction( m_selectAllAlbums );
    viewGroup->addAction( m_selectAlbumsWithCover );
    viewGroup->addAction( m_selectAlbumsWithoutCover );

    m_viewButton->setMenu( m_viewMenu );
    m_viewButton->setIcon( KIcon( "filename-album-amarok" ) );
    connect( m_viewMenu, SIGNAL(triggered(QAction*)), this, SLOT(slotAlbumFilterTriggered(QAction*)) );

    //fetch missing covers button
    m_fetchButton = new KPushButton( KGuiItem( i18n("Fetch Missing Covers"), "get-hot-new-stuff-amarok" ), hbox );
    connect( m_fetchButton, SIGNAL(clicked()), SLOT(fetchMissingCovers()) );

    m_selectAllAlbums->setChecked( true );
    m_selectAllAlbums->trigger();

    //cover view
    m_coverView = new CoverView( vbox );
    m_coverViewSpacer = new CoverView( vbox );
    m_coverViewSpacer->hide();

    //status bar
    KStatusBar *m_statusBar = new KStatusBar( vbox );

    m_statusLabel = new KSqueezedTextLabel( m_statusBar );
    m_statusLabel->setIndent( 3 );
    m_progressBox = new KHBox( m_statusBar );

    m_statusBar->addWidget( m_statusLabel, 4 );
    m_statusBar->addPermanentWidget( m_progressBox, 1 );

    QToolButton *stopButton = new QToolButton( m_progressBox );
    stopButton->setIcon( KIcon( "dialog-cancel-amarok" ) );
    stopButton->setToolTip( i18n( "Abort" ) );
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

    QSize size = QApplication::desktop()->screenGeometry( this ).size() / 1.5;
    QSize sz = Amarok::config( "Cover Manager" ).readEntry( "Window Size", size );
    resize( sz.width(), sz.height() );

    setStretchFactor( indexOf( m_artistView ), 1 );
    setStretchFactor( indexOf( vbox ), 4 );

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

    const QString albumName = album->name();
    foreach( CoverViewItem *item, m_coverItems )
    {
        if( albumName == item->albumPtr()->name() )
            item->loadCover();
    }
    updateStatusBar();
}

void CoverManager::fetchMissingCovers() //SLOT
{
    DEBUG_BLOCK

    m_fetchCovers.clear();
    for( int i = 0, coverCount = m_coverView->count(); i < coverCount; ++i )
    {
        QListWidgetItem *item = m_coverView->item( i );
        CoverViewItem *coverItem = static_cast<CoverViewItem*>( item );
        if( !coverItem->hasCover() ) {
            m_fetchCovers += coverItem->albumPtr();
        }
    }

    m_progress->setMaximum( m_fetchCovers.size() );
    m_fetcher->queueAlbums( m_fetchCovers );
    m_fetchingCovers = true;

    updateStatusBar();
    m_fetchButton->setEnabled( false );
    connect( m_fetcher, SIGNAL(finishedSingle(int)), SLOT(updateFetchingProgress(int)) );

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

    //Extra time for the sake of init() and aesthetics
    QTimer::singleShot( 0, this, SLOT( slotArtistSelectedContinue() ) );
}


void CoverManager::slotArtistSelectedContinue() //SLOT
{
    DEBUG_BLOCK

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

    qm->beginOr();
    const QList< QTreeWidgetItem* > items = m_artistView->selectedItems();
    foreach( const QTreeWidgetItem *item, items )
    {
        const ArtistItem *artistItem = static_cast< const ArtistItem* >( item );
        if( artistItem != m_artistView->invisibleRootItem()->child( 0 ) )
            qm->addFilter( Meta::valArtist, artistItem->artist()->name(), true, true );
        else
            qm->excludeFilter( Meta::valAlbum, QString(), true, true );
    }
    qm->endAndOr();

    // do not show albums with no name, i.e. tracks not belonging to any album
    qm->beginAnd();
    qm->excludeFilter( Meta::valAlbum, QString(), true, true );
    qm->endAndOr();

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


void
CoverManager::slotAlbumFilterTriggered( QAction *action ) //SLOT
{
    m_viewButton->setText( action->text() );
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

    //the process events calls below causes massive flickering in the m_albumList
    //so we hide this view and only show it when all items has been inserted. This
    //also provides quite a massive speed improvement when loading covers.
    m_coverView->hide();
    m_coverViewSpacer->show();
    foreach( Meta::AlbumPtr album, m_albumList )
    {
        CoverViewItem *item = new CoverViewItem( m_coverView, album );
        m_coverItems.append( item );

        kapp->processEvents(); // QProgressDialog also calls this, but not always due to Qt bug!

         //only worth testing for after processEvents() is called
        if( m_progressDialog->wasCanceled() )
        {
            break;
        }
        
        if ( ++x % 10 == 0 )
        {
            m_progressDialog->setValue( x );
        }
    }

    // makes sure View is retained when artist selection changes
    changeView( m_currentView, true );

    m_coverViewSpacer->hide();
    m_coverView->show();
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

    // makes sure View is retained when filter text has changed
    changeView( m_currentView, true );
    updateStatusBar();
}


void CoverManager::slotSetFilterTimeout() //SLOT
{
    if ( m_timer->isActive() ) m_timer->stop();
    m_timer->setSingleShot( true );
    m_timer->start( 180 );
}

void CoverManager::changeView( CoverManager::View id, bool force ) //SLOT
{
    DEBUG_BLOCK

    if( !force && m_currentView == id )
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

void CoverManager::updateFetchingProgress( int state )
{
    switch( static_cast< CoverFetcher::FinishState >( state ) )
    {
    case CoverFetcher::Success:
        m_coversFetched++;
        break;

    case CoverFetcher::Error:
    case CoverFetcher::NotFound:
    default:
        m_coverErrors++;
        break;
    }
    m_progress->setValue( m_coversFetched + m_coverErrors );
}

void CoverManager::stopFetching()
{
    DEBUG_FUNC_INFO

    m_fetchCovers.clear();
    m_fetchingCovers = false;
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
        if( m_progressBox->isHidden() )
            m_progressBox->show();

        //update the status text
        if( m_coversFetched + m_coverErrors >= m_fetchCovers.size() )
        {
            //fetching finished
            text = i18nc( "The fetching is done.", "Finished." );
            if( m_coverErrors )
                text += i18np( " Cover not found", " <b>%1</b> covers not found", m_coverErrors );
            //reset counters
            m_coversFetched = 0;
            m_coverErrors = 0;
            m_fetchCovers.clear();
            m_fetchingCovers = false;

            disconnect( m_fetcher, SIGNAL(finishedSingle(int)), this, SLOT(updateFetchingProgress(int)) );
            QTimer::singleShot( 2000, this, SLOT( updateStatusBar() ) );
        }

        if( m_fetchCovers.size() == 1 )
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
        else
        {
            text = i18np( "Fetching 1 cover: ", "Fetching <b>%1</b> covers... : ", m_fetchCovers.size() );
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
        for( int i = 0, coverCount = m_coverView->count(); i < coverCount; ++i )
        {
            totalCounter++;
            QListWidgetItem *item = m_coverView->item( i );
            if( !static_cast<CoverViewItem*>( item )->hasCover() )
                missingCounter++;    //counter for albums without cover
        }

        const QList< QTreeWidgetItem* > selected = m_artistView->selectedItems();

        if( !m_filter.isEmpty() )
        {
            text = i18np( "1 result for \"%2\"", "%1 results for \"%2\"", totalCounter, m_filter );
        }
        else if( selected.count() > 0 )
        {
            text = i18np( "1 album", "%1 albums", totalCounter );

            // showing albums by selected artist(s)
            if( selected.first() != m_artistView->invisibleRootItem()->child( 0 ) )
            {
                QStringList artists;
                foreach( const QTreeWidgetItem *item, selected )
                {
                    QString artist = item->text( 0 );
                    Amarok::manipulateThe( artist, false );
                    artists.append( artist );
                }
                text += i18n( " by " ) + artists.join( ", " );
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
    setWordWrap( true );
    setIconSize( QSize(100, 100) );
    setGridSize( QSize(120, 160) );
    setTextElideMode( Qt::ElideRight );
    setMouseTracking( true );
    setContextMenuPolicy( Qt::DefaultContextMenu );

    connect( this, SIGNAL( itemEntered( QListWidgetItem * ) ), SLOT( setStatusText( QListWidgetItem * ) ) );
    connect( this, SIGNAL( viewportEntered() ), CoverManager::instance(), SLOT( updateStatusBar() ) );
}

void CoverView::contextMenuEvent( QContextMenuEvent *event )
{
    QList<QListWidgetItem*> items = selectedItems();

    if( items.count() > 0 )
    {
        QMap<QString,QList<QAction *> >  cacs;
        KMenu menu;
        menu.addTitle( i18n( "Cover Image" ) );
        
        for (int x = 0; x < items.count(); ++x)
        {
            CoverViewItem *item = dynamic_cast<CoverViewItem*>(items.value(x));
            Meta::AlbumPtr album = item->albumPtr();
            if( album )
            {
                Meta::CustomActionsCapability *cac = album->create<Meta::CustomActionsCapability>();
                if( cac )
                {
                    QList<QAction *> actions = cac->customActions();

                    foreach( QAction *action, actions )
                    {
                        if( !action->text().isEmpty() )
                        {
                            cacs[action->text()].append(action);
                        }
                    }
                }
            }
        }

        if( cacs.count() > 0 )
        {
            menu.addSeparator();

            foreach ( QList<QAction *> actionList, cacs )
            {
                if( actionList.count() == items.count() )
                {
                    MultipleAction *maction = new MultipleAction( this, actionList );
                    menu.addAction( maction );
                }
            }
        }
        menu.exec( event->globalPos() );
    }
    else
        QListView::contextMenuEvent( event );
    // TODO
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
    setIcon( m_albumPtr->image( 100 ) );
    m_albumPtr->setSuppressImageAutoFetch( isSuppressing );
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

