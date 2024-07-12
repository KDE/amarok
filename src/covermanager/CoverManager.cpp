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

#define DEBUG_PREFIX "CoverManager"

#include "CoverManager.h"

#include "amarokconfig.h"
#include <config.h>
#include "core/capabilities/ActionsCapability.h"
#include "core/collections/Collection.h"
#include "core/collections/QueryMaker.h"
#include "core/meta/Meta.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "covermanager/CoverFetchingActions.h"
#include "covermanager/CoverViewDialog.h"
#include "playlist/PlaylistController.h"
#include "statusbar/CompoundProgressBar.h"
#include "widgets/LineEdit.h"
#include "widgets/PixmapViewer.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QDialogButtonBox>
#include <QMenu>    //showCoverMenu()
#include <QProgressBar>
#include <QPushButton>
#include <QScreen>
#include <QSplitter>
#include <QStatusBar>
#include <QStringList>
#include <QTimer>    //search filter timer
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include <KConfigGroup>
#include <KIconLoader>
#include <KLocalizedString>
#include <KSqueezedTextLabel> //status label
#include <KToolBar>

static QString artistToSelectInInitFunction;
CoverManager *CoverManager::s_instance = nullptr;

class ArtistItem : public QTreeWidgetItem
{
    public:
        ArtistItem( QTreeWidget *parent, Meta::ArtistPtr artist )
            : QTreeWidgetItem( parent )
            , m_artist( artist )
        {
            setText( 0, artist->prettyName() );
        }

        ArtistItem( const QString &text, QTreeWidget *parent = nullptr )
            : QTreeWidgetItem( parent )
            , m_artist( nullptr )
        {
            setText( 0, text );
        }

        Meta::ArtistPtr artist() const { return m_artist; }

    private:
        Meta::ArtistPtr m_artist;
};


CoverManager::CoverManager( QWidget *parent )
        : QDialog( parent )
        , m_currentView( AllAlbums )
        , m_timer( new QTimer( this ) )    //search filter timer
        , m_fetchingCovers( false )
        , m_coversFetched( 0 )
        , m_coverErrors( 0 )
        , m_isLoadingCancelled( false )
{
    DEBUG_BLOCK

    setObjectName( QStringLiteral("TheCoverManager") );

    s_instance = this;

    // Sets caption and icon correctly (needed e.g. for GNOME)
    //kapp->setTopWidget( this );

    QDialogButtonBox *buttonBox = new QDialogButtonBox( QDialogButtonBox::Close, this );
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &CoverManager::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &CoverManager::reject);
    setWindowTitle( i18n("Cover Manager") );
    setAttribute( Qt::WA_DeleteOnClose );

    // TODO: There is no hidden signal in QDialog. Needs porting to QT5.
//     connect( this, &CoverManager::hidden, this, &CoverManager::delayedDestruct );
    connect( buttonBox->button(QDialogButtonBox::Close), &QAbstractButton::clicked, this, &CoverManager::delayedDestruct );

    m_splitter = new QSplitter( this );
    mainLayout->addWidget(m_splitter);
    mainLayout->addWidget(buttonBox);

    //artist listview
    m_artistView = new QTreeWidget( m_splitter );
    m_artistView->setHeaderLabel( i18n( "Albums By" ) );
    m_artistView->setSortingEnabled( false );
    m_artistView->setTextElideMode( Qt::ElideRight );
    m_artistView->setMinimumWidth( 200 );
    m_artistView->setColumnCount( 1 );
    m_artistView->setAlternatingRowColors( true );
    m_artistView->setUniformRowHeights( true );
    m_artistView->setSelectionMode( QAbstractItemView::ExtendedSelection );

    ArtistItem *item = nullptr;
    item = new ArtistItem( i18n( "All Artists" ) );
    item->setIcon(0, QIcon::fromTheme( QStringLiteral("media-optical-audio-amarok") ) );
    m_items.append( item );

    Collections::Collection *coll = CollectionManager::instance()->primaryCollection();
    Collections::QueryMaker *qm = coll->queryMaker();
    qm->setAutoDelete( true );
    qm->setQueryType( Collections::QueryMaker::Artist );
    qm->setAlbumQueryMode( Collections::QueryMaker::OnlyNormalAlbums );
    qm->orderBy( Meta::valArtist );

    connect( qm, &Collections::QueryMaker::newArtistsReady,
             this, &CoverManager::slotArtistQueryResult );

    connect( qm, &Collections::QueryMaker::queryDone, this, &CoverManager::slotContinueConstruction );

    qm->run();
}

void
CoverManager::slotArtistQueryResult( Meta::ArtistList artists ) //SLOT
{
    DEBUG_BLOCK
    for( Meta::ArtistPtr artist : artists )
        m_artistList << artist;
}

void
CoverManager::slotContinueConstruction() //SLOT
{
    DEBUG_BLOCK
    for( Meta::ArtistPtr artist : m_artistList )
    {
        ArtistItem* item = new ArtistItem( m_artistView, artist );
        item->setIcon( 0, QIcon::fromTheme( QStringLiteral("view-media-artist-amarok") ) );
        m_items.append( item );
    }
    m_artistView->insertTopLevelItems( 0, m_items );

    BoxWidget *vbox = new BoxWidget( true, m_splitter );
    BoxWidget *hbox = new BoxWidget( false, vbox );
    vbox->layout()->setSpacing( 4 );
    hbox->layout()->setSpacing( 4 );

    { //<Search LineEdit>
        m_searchEdit = new Amarok::LineEdit( hbox );
        m_searchEdit->setPlaceholderText( i18n( "Enter search terms here" ) );
        m_searchEdit->setFrame( true );

        m_searchEdit->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Minimum);
        m_searchEdit->setClearButtonEnabled( true );

        static_cast<QHBoxLayout*>( hbox->layout() )->setStretchFactor( m_searchEdit, 1 );
    } //</Search LineEdit>

    // view menu
    m_viewButton = new QPushButton( hbox );

    m_viewMenu = new QMenu( m_viewButton );
    m_selectAllAlbums          = m_viewMenu->addAction( i18n("All Albums"),           this, &CoverManager::slotShowAllAlbums );
    m_selectAlbumsWithCover    = m_viewMenu->addAction( i18n("Albums With Cover"),    this, &CoverManager::slotShowAlbumsWithCover );
    m_selectAlbumsWithoutCover = m_viewMenu->addAction( i18n("Albums Without Cover"), this, &CoverManager::slotShowAlbumsWithoutCover );

    QActionGroup *viewGroup = new QActionGroup( m_viewButton );
    viewGroup->setExclusive( true );
    viewGroup->addAction( m_selectAllAlbums );
    viewGroup->addAction( m_selectAlbumsWithCover );
    viewGroup->addAction( m_selectAlbumsWithoutCover );

    m_viewButton->setMenu( m_viewMenu );
    m_viewButton->setIcon( QIcon::fromTheme( QStringLiteral("filename-album-amarok") ) );
    connect( m_viewMenu, &QMenu::triggered, this, &CoverManager::slotAlbumFilterTriggered );

    //fetch missing covers button
    m_fetchButton = new QPushButton( QIcon::fromTheme( QStringLiteral("get-hot-new-stuff-amarok") ), i18n("Fetch Missing Covers"), hbox );
    connect( m_fetchButton, &QAbstractButton::clicked, this, &CoverManager::fetchMissingCovers );

    m_selectAllAlbums->setChecked( true );
    m_selectAllAlbums->trigger();

    //cover view
    m_coverView = new CoverView( vbox );
    m_coverViewSpacer = new CoverView( vbox );
    m_coverViewSpacer->hide();

    //status bar
    QStatusBar *statusBar = new QStatusBar( vbox );

    m_statusLabel = new KSqueezedTextLabel( statusBar );
    m_statusLabel->setIndent( 3 );
    m_progress = new CompoundProgressBar( statusBar );

    statusBar->addWidget( m_statusLabel, 4 );
    statusBar->addPermanentWidget( m_progress, 1 );

    connect( m_progress, &CompoundProgressBar::allDone, this, &CoverManager::progressAllDone );

    QSize size = this->screen()->size() / 1.5;
    QSize sz = Amarok::config( QStringLiteral("Cover Manager") ).readEntry( "Window Size", size );
    resize( sz.width(), sz.height() );

    m_splitter->setStretchFactor( m_splitter->indexOf( m_artistView ), 1 );
    m_splitter->setStretchFactor( m_splitter->indexOf( vbox ), 4 );

    m_fetcher = The::coverFetcher();

    QTreeWidgetItem *item = nullptr;
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

    // signals and slots connections
    connect( m_artistView, &QTreeWidget::itemSelectionChanged,
             this, &CoverManager::slotArtistSelected );
    connect( m_coverView, &CoverView::itemActivated,
             this, &CoverManager::coverItemClicked );
    connect( m_timer, &QTimer::timeout,
             this, &CoverManager::slotSetFilter );
    connect( m_searchEdit, &Amarok::LineEdit::textChanged,
             this, &CoverManager::slotSetFilterTimeout );

    if( item == nullptr )
        item = m_artistView->invisibleRootItem()->child( 0 );

    item->setSelected( true );
    show();
}

CoverManager::~CoverManager()
{
    Amarok::config( QStringLiteral("Cover Manager") ).writeEntry( "Window Size", size() );
    qDeleteAll( m_coverItems );
    delete m_coverView;
    m_coverView = nullptr;
    s_instance = nullptr;
}

void
CoverManager::viewCover( const Meta::AlbumPtr &album, QWidget *parent ) //static
{
    //QDialog means "escape" works as expected
    QDialog *dialog = new CoverViewDialog( album, parent );
    dialog->show();
}

void
CoverManager::metadataChanged(const Meta::AlbumPtr &album )
{
    const QString albumName = album->name();
    for( CoverViewItem *item : m_coverItems )
    {
        if( albumName == item->albumPtr()->name() )
            item->loadCover();
    }
    updateStatusBar();
}

void
CoverManager::fetchMissingCovers() //SLOT
{
    m_fetchCovers.clear();
    for( int i = 0, coverCount = m_coverView->count(); i < coverCount; ++i )
    {
        QListWidgetItem *item = m_coverView->item( i );
        CoverViewItem *coverItem = static_cast<CoverViewItem*>( item );
        if( !coverItem->hasCover() )
            m_fetchCovers += coverItem->albumPtr();
    }

    debug() << QStringLiteral( "Fetching %1 missing covers" ).arg( m_fetchCovers.size() );

    ProgressBar *fetchProgressBar = new ProgressBar( this );
    fetchProgressBar->setDescription( i18n( "Fetching" ) );
    fetchProgressBar->setMaximum( m_fetchCovers.size() );
    m_progress->addProgressBar( fetchProgressBar, m_fetcher );
    m_progress->show();

    m_fetcher->queueAlbums( m_fetchCovers );
    m_fetchingCovers = true;

    updateStatusBar();
    m_fetchButton->setEnabled( false );
    connect( m_fetcher, &CoverFetcher::finishedSingle, this, &CoverManager::updateFetchingProgress );
}

void
CoverManager::showOnce( const QString &artist, QWidget* parent )
{
    if( !s_instance )
    {
        artistToSelectInInitFunction = artist;
        new CoverManager( parent );
    }
    else
    {
        s_instance->activateWindow();
        s_instance->raise();
    }
}

void
CoverManager::slotArtistSelected() //SLOT
{
    DEBUG_BLOCK

    // delete cover items before clearing cover view
    qDeleteAll( m_coverItems );
    m_coverItems.clear();
    m_coverView->clear();

    //this can be a bit slow
    QApplication::setOverrideCursor( Qt::WaitCursor );

    Collections::Collection *coll = CollectionManager::instance()->primaryCollection();

    Collections::QueryMaker *qm = coll->queryMaker();
    qm->setAutoDelete( true );
    qm->setQueryType( Collections::QueryMaker::Album );
    qm->orderBy( Meta::valAlbum );

    qm->beginOr();
    const QList< QTreeWidgetItem* > items = m_artistView->selectedItems();
    for( const QTreeWidgetItem *item : items )
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

    connect( qm, &Collections::QueryMaker::newAlbumsReady,
             this, &CoverManager::slotAlbumQueryResult );

    connect( qm, &Collections::QueryMaker::queryDone, this, &CoverManager::slotArtistQueryDone );

    qm->run();
}

void
CoverManager::slotAlbumQueryResult( const Meta::AlbumList &albums ) //SLOT
{
    m_albumList = albums;
}

void
CoverManager::slotAlbumFilterTriggered( QAction *action ) //SLOT
{
    m_viewButton->setText( action->text() );
}

void
CoverManager::slotArtistQueryDone() //SLOT
{
    DEBUG_BLOCK

    QApplication::restoreOverrideCursor();

    const int albumCount = m_albumList.count();

    ProgressBar *coverLoadProgressBar = new ProgressBar( this );
    coverLoadProgressBar->setDescription( i18n( "Loading" ) );
    coverLoadProgressBar->setMaximum( albumCount );
    connect( coverLoadProgressBar, &ProgressBar::cancelled,
             this, &CoverManager::cancelCoverViewLoading );

    m_progress->addProgressBar( coverLoadProgressBar, m_coverView );
    m_progress->show();

    uint x = 0;
    debug() << "Loading covers for selected artist(s)";

    //the process events calls below causes massive flickering in the m_albumList
    //so we hide this view and only show it when all items has been inserted. This
    //also provides quite a massive speed improvement when loading covers.
    m_coverView->hide();
    m_coverViewSpacer->show();
    for( const Meta::AlbumPtr &album : m_albumList )
    {
        qApp->processEvents( QEventLoop::ExcludeSocketNotifiers );
        if( isHidden() )
        {
            m_progress->endProgressOperation( m_coverView );
            return;
        }
        /*
         * Loading is stopped if cancelled by the user, or the number of albums
         * has changed. The latter occurs when the artist selection changes.
         */
        if( m_isLoadingCancelled || albumCount != m_albumList.count() )
        {
            m_isLoadingCancelled = false;
            break;
        }

        CoverViewItem *item = new CoverViewItem( m_coverView, album );
        m_coverItems.append( item );

        if( ++x % 10 == 0 )
        {
            m_progress->setProgress( m_coverView, x );
        }
    }
    m_progress->endProgressOperation( m_coverView );

    // makes sure View is retained when artist selection changes
    changeView( m_currentView, true );

    m_coverViewSpacer->hide();
    m_coverView->show();
    updateStatusBar();
}

void
CoverManager::cancelCoverViewLoading()
{
    m_isLoadingCancelled = true;
}

// called when a cover item is clicked
void
CoverManager::coverItemClicked( QListWidgetItem *item ) //SLOT
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


void
CoverManager::slotSetFilter() //SLOT
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

    for( QListWidgetItem *item : m_coverItems )
    {
        CoverViewItem *coverItem = static_cast<CoverViewItem*>(item);
        if( coverItem->album().contains( m_filter, Qt::CaseInsensitive ) || coverItem->artist().contains( m_filter, Qt::CaseInsensitive ) )
            m_coverView->insertItem( m_coverView->count() -  1, item );
    }

    // makes sure View is retained when filter text has changed
    changeView( m_currentView, true );
    updateStatusBar();
}


void
CoverManager::slotSetFilterTimeout() //SLOT
{
    if ( m_timer->isActive() ) m_timer->stop();
    m_timer->setSingleShot( true );
    m_timer->start( 180 );
}

void
CoverManager::changeView( CoverManager::View id, bool force ) //SLOT
{
    if( !force && m_currentView == id )
        return;

    //clear the iconview without deleting items
    m_coverView->clearSelection();
    int itemsCount = m_coverView->count();
    while( itemsCount-- > 0 )
       m_coverView->takeItem( 0 );

    for( QListWidgetItem *item : m_coverItems )
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

void
CoverManager::updateFetchingProgress( int state )
{
    switch( static_cast< CoverFetcher::FinishState >( state ) )
    {
    case CoverFetcher::Success:
        m_coversFetched++;
        break;

    case CoverFetcher::Cancelled:
    case CoverFetcher::Error:
    case CoverFetcher::NotFound:
    default:
        m_coverErrors++;
        break;
    }
    m_progress->incrementProgress( m_fetcher );
    updateStatusBar();
}

void
CoverManager::stopFetching()
{
    DEBUG_FUNC_INFO

    m_fetchCovers.clear();
    m_fetchingCovers = false;
    m_progress->endProgressOperation( m_fetcher );
    updateStatusBar();
}

void
CoverManager::loadCover( const QString &artist, const QString &album )
{
    for( QListWidgetItem *item : m_coverItems )
    {
        CoverViewItem *coverItem = static_cast<CoverViewItem*>(item);
        if ( album == coverItem->album() && ( artist == coverItem->artist() || ( artist.isEmpty() && coverItem->artist().isEmpty() ) ) )
        {
            coverItem->loadCover();
            return;
        }
    }
}

void
CoverManager::progressAllDone()
{
    m_progress->hide();
}

void
CoverManager::updateStatusBar()
{
    QString text;

    //cover fetching info
    if( m_fetchingCovers )
    {
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
            m_progress->endProgressOperation( m_fetcher );

            disconnect( m_fetcher, &CoverFetcher::finishedSingle, this, &CoverManager::updateFetchingProgress );
            QTimer::singleShot( 2000, this, &CoverManager::updateStatusBar );
        }

        if( m_fetchCovers.size() == 1 )
        {
            for( Meta::AlbumPtr album : m_fetchCovers )
            {
                if( album->hasAlbumArtist() && !album->albumArtist()->prettyName().isEmpty() )
                {
                    text = i18n( "Fetching cover for %1 - %2...",
                                 album->albumArtist()->prettyName(),
                                 album->prettyName() );
                }
                else
                {
                    text = i18n( "Fetching cover for %1..." , album->prettyName() );
                }
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
                for( const QTreeWidgetItem *item : selected )
                {
                    QString artist = item->text( 0 );
                    Amarok::manipulateThe( artist, false );
                    artists.append( artist );
                }
                text = i18n( "%1 by %2", text, artists.join( i18nc("Separator for artists", ", ")) );
            }
        }

        if( missingCounter )
            text = i18np("%2 - ( <b>1</b> without cover )", "%2 - ( <b>%1</b> without cover )",
                         missingCounter, text );

        m_fetchButton->setEnabled( missingCounter );
    }

    m_statusLabel->setText( text );
}

void
CoverManager::delayedDestruct()
{
    if ( isVisible() )
        hide();

    deleteLater();
}

void
CoverManager::setStatusText( const QString &text )
{
    m_oldStatusText = m_statusLabel->text();
    m_statusLabel->setText( text );
}

//////////////////////////////////////////////////////////////////////
//    CLASS CoverView
/////////////////////////////////////////////////////////////////////

CoverView::CoverView( QWidget *parent, const char *name, Qt::WindowFlags f )
    : QListWidget( parent )
{
    DEBUG_BLOCK

    setObjectName( QLatin1String(name) );
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
    setContextMenuPolicy( Qt::DefaultContextMenu );
    setMouseTracking( true ); // required for setting status text when itemEntered signal is emitted

    connect( this, &CoverView::itemEntered, this, &CoverView::setStatusText );
    connect( this, &CoverView::viewportEntered, CoverManager::instance(), &CoverManager::updateStatusBar );
}

void
CoverView::contextMenuEvent( QContextMenuEvent *event )
{
    QList<QListWidgetItem*> items = selectedItems();
    const int itemsCount = items.count();

    QMenu menu;
    menu.addSection( i18n( "Cover Image" ) );

    if( itemsCount == 1 )
    {
        // only one item selected: get all custom actions this album is capable of.
        CoverViewItem *item = dynamic_cast<CoverViewItem*>( items.first() );
        QList<QAction *> actions;
        Meta::AlbumPtr album = item->albumPtr();
        if( album )
        {
            QScopedPointer<Capabilities::ActionsCapability> ac( album->create<Capabilities::ActionsCapability>() );
            if( ac )
            {
                actions = ac->actions();
                for( QAction *action : actions )
                    menu.addAction( action );
            }
        }
        menu.exec( event->globalPos() );
    }
    else if( itemsCount > 1 )
    {
        // multiple albums selected: only unset cover and fetch cover actions
        // make sense here, and perhaps (un)setting compilation flag (TODO).

        Meta::AlbumList unsetAlbums;
        Meta::AlbumList fetchAlbums;

        for( QListWidgetItem *item : items )
        {
            CoverViewItem *cvItem = dynamic_cast<CoverViewItem*>(item);
            Meta::AlbumPtr album = cvItem->albumPtr();
            if( album )
            {
                QScopedPointer<Capabilities::ActionsCapability> ac( album->create<Capabilities::ActionsCapability>() );
                if( ac )
                {
                    QList<QAction *> actions = ac->actions();
                    for( QAction *action : actions )
                    {
                        if( qobject_cast<FetchCoverAction*>(action) )
                            fetchAlbums << album;
                        else if( qobject_cast<UnsetCoverAction*>(action) )
                            unsetAlbums << album;
                    }
                }
            }
        }

        if( itemsCount == fetchAlbums.count() )
        {
            FetchCoverAction *fetchAction = new FetchCoverAction( this, fetchAlbums );
            menu.addAction( fetchAction );
        }
        if( itemsCount == unsetAlbums.count() )
        {
            UnsetCoverAction *unsetAction = new UnsetCoverAction( this, unsetAlbums );
            menu.addAction( unsetAction );
        }
        menu.exec( event->globalPos() );
    }
    else
        QListView::contextMenuEvent( event );

    // TODO: Play, Load and Append to playlist actions
}

void
CoverView::setStatusText( QListWidgetItem *item )
{
    #define itemmacro static_cast<CoverViewItem *>( item )
    if ( !itemmacro )
        return;

    const QString artist = itemmacro->albumPtr()->isCompilation() ? i18n( "Various Artists" ) : itemmacro->artist();
    const QString tipContent = i18n( "%1 - %2", artist , itemmacro->album() );
    CoverManager::instance()->setStatusText( tipContent );
    #undef item
}

//////////////////////////////////////////////////////////////////////
//    CLASS CoverViewItem
/////////////////////////////////////////////////////////////////////

CoverViewItem::CoverViewItem( QListWidget *parent, Meta::AlbumPtr album )
    : QListWidgetItem( parent )
    , m_albumPtr( album)
{
    m_album = album->prettyName();
    if( album->hasAlbumArtist() )
        m_artist = album->albumArtist()->prettyName();
    else
        m_artist = i18n( "No Artist" );
    setText( album->prettyName() );

    loadCover();

    CoverManager::instance()->subscribeTo( album );
}

CoverViewItem::~CoverViewItem()
{}

bool
CoverViewItem::hasCover() const
{
    return albumPtr()->hasImage();
}

void
CoverViewItem::loadCover()
{
    const bool isSuppressing = m_albumPtr->suppressImageAutoFetch();
    m_albumPtr->setSuppressImageAutoFetch( true );
    setIcon( QPixmap::fromImage( m_albumPtr->image( 100 ) ) );
    m_albumPtr->setSuppressImageAutoFetch( isSuppressing );
}

void
CoverViewItem::dragEntered()
{
    setSelected( true );
}


void
CoverViewItem::dragLeft()
{
    setSelected( false );
}


