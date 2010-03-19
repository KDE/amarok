/****************************************************************************************
 * Copyright (c) 2004 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2004 Stefan Bogner <bochi@online.ms>                                   *
 * Copyright (c) 2004 Max Howell <max.howell@methylblue.com>                            *
 * Copyright (c) 2007 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
 * Copyright (c) 2009 Martin Sandsmark <sandsmark@samfundet.no>                         *
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

#include "CoverFoundDialog.h"

#include "Amarok.h"
#include "CoverViewDialog.h"
#include "Debug.h"
#include "PixmapViewer.h"
#include "statusbar/KJobProgressBar.h"
#include "SvgHandler.h"

#include <KConfigGroup>
#include <KIO/Job>
#include <KLineEdit>
#include <KListWidget>
#include <KPushButton>
#include <KStandardDirs>

#include <QCloseEvent>
#include <QDir>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QMenu>
#include <QSplitter>
#include <QTabWidget>
#include <QTableWidget>

#define DEBUG_PREFIX "CoverFoundDialog"

CoverFoundDialog::CoverFoundDialog( const CoverFetchUnit::Ptr unit,
                                    const QPixmap cover,
                                    const CoverFetch::Metadata data,
                                    QWidget *parent )
    : KDialog( parent )
    , m_queryPage( 1 )
    , m_unit( unit )
    , m_album( unit->album() )
{
    setButtons( KDialog::Ok | KDialog::Cancel |
                KDialog::User1 ); // User1: clear icon view

    setButtonGuiItem( KDialog::User1, KStandardGuiItem::clear() );
    connect( button( KDialog::User1 ), SIGNAL(clicked()), SLOT(clearView()) );

    m_save = button( KDialog::Ok );

    QSplitter *splitter = new QSplitter( this );
    m_sideBar = new CoverFoundSideBar( m_album, splitter );

    KVBox *vbox = new KVBox( splitter );
    vbox->setSpacing( 4 );

    KHBox *searchBox = new KHBox( vbox );
    vbox->setSpacing( 4 );

    m_search = new KLineEdit( searchBox );
    m_search->setClearButtonShown( true );
    m_search->setClickMessage( i18n( "Enter Custom Search" ) );
    m_search->setTrapReturnKey( true );
    setupSearchToolTip();

    KCompletion *searchComp = m_search->completionObject();
    searchComp->setOrder( KCompletion::Insertion );
    searchComp->setIgnoreCase( true );

    QStringList completionNames;
    QString firstRunQuery( m_album->name() );
    completionNames << firstRunQuery;
    if( m_album->hasAlbumArtist() )
    {
        const QString &name = m_album->albumArtist()->name();
        completionNames << name;
        firstRunQuery += ' ' + name;
    }
    m_query = firstRunQuery;
    searchComp->setItems( completionNames );
    m_album->setSuppressImageAutoFetch( true );

    m_searchButton = new KPushButton( KStandardGuiItem::find(), searchBox );
    KPushButton *sourceButton = new KPushButton( KStandardGuiItem::configure(), searchBox );
    updateSearchButton( firstRunQuery );

    QMenu *sourceMenu = new QMenu( sourceButton );
    QAction *lastFmAct = new QAction( i18n( "Last.fm" ), sourceMenu );
    QAction *googleAct = new QAction( i18n( "Google" ), sourceMenu );
    QAction *yahooAct = new QAction( i18n( "Yahoo!" ), sourceMenu );
    QAction *discogsAct = new QAction( i18n( "Discogs" ), sourceMenu );
    lastFmAct->setCheckable( true );
    googleAct->setCheckable( true );
    yahooAct->setCheckable( true );
    discogsAct->setCheckable( true );
    connect( lastFmAct, SIGNAL(triggered()), this, SLOT(selectLastFm()) );
    connect( googleAct, SIGNAL(triggered()), this, SLOT(selectGoogle()) );
    connect( yahooAct, SIGNAL(triggered()), this, SLOT(selectYahoo()) );
    connect( discogsAct, SIGNAL(triggered()), this, SLOT(selectDiscogs()) );

    QActionGroup *ag = new QActionGroup( sourceButton );
    ag->addAction( lastFmAct );
    ag->addAction( googleAct );
    ag->addAction( yahooAct );
    ag->addAction( discogsAct );
    sourceMenu->addActions( ag->actions() );
    sourceButton->setMenu( sourceMenu );

    connect( m_search, SIGNAL(returnPressed(const QString&)), searchComp, SLOT(addItem(const QString&)) );
    connect( m_search, SIGNAL(returnPressed(const QString&)), SLOT(processQuery(const QString&)) );
    connect( m_search, SIGNAL(returnPressed(const QString&)), SLOT(updateSearchButton(const QString&)) );
    connect( m_search, SIGNAL(textChanged(const QString&)), SLOT(updateSearchButton(const QString&)) );
    connect( m_search, SIGNAL(clearButtonClicked()), SLOT(clearQueryButtonClicked()));
    connect( m_searchButton, SIGNAL(pressed()), SLOT(processQuery()) );

    m_view = new KListWidget( vbox );
    m_view->setAcceptDrops( false );
    m_view->setContextMenuPolicy( Qt::CustomContextMenu );
    m_view->setDragDropMode( QAbstractItemView::NoDragDrop );
    m_view->setDragEnabled( false );
    m_view->setDropIndicatorShown( false );
    m_view->setMovement( QListView::Static );
    m_view->setGridSize( QSize( 140, 150 ) );
    m_view->setIconSize( QSize( 120, 120 ) );
    m_view->setSpacing( 4 );
    m_view->setViewMode( QListView::IconMode );
    m_view->setResizeMode( QListView::Adjust );

    connect( m_view, SIGNAL(itemSelectionChanged()),
             this,   SLOT(itemSelected()) );
    /* // FIXME: Double clicking on an item crashes Amarok, seems to be a qt bug.
    connect( m_view, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
             this,   SLOT(itemDoubleClicked(QListWidgetItem*)) );*/
    connect( m_view, SIGNAL(customContextMenuRequested(const QPoint&)),
             this,   SLOT(itemMenuRequested(const QPoint&)) );

    splitter->addWidget( m_sideBar );
    splitter->addWidget( vbox );
    setMainWidget( splitter );

    connect( m_save, SIGNAL(released()), SLOT(saveRequested()) );

    const KConfigGroup config = Amarok::config( "Cover Fetcher" );
    const QString source = config.readEntry( "Interactive Image Source", "LastFm" );
    restoreDialogSize( config ); // call this after setMainWidget()

    if( source == "LastFm" )
        lastFmAct->setChecked( true );
    else if( source == "Yahoo" )
        yahooAct->setChecked( true );
    else if( source == "Discogs" )
        discogsAct->setChecked( true );
    else
        googleAct->setChecked( true );

    typedef CoverFetchArtPayload CFAP;
    const CFAP *payload = dynamic_cast< const CFAP* >( unit->payload() );
    add( cover, data, payload->imageSize() );
    m_view->setCurrentItem( m_view->item( 0 ) );
}

CoverFoundDialog::~CoverFoundDialog()
{
    m_album->setSuppressImageAutoFetch( false );
}

void CoverFoundDialog::hideEvent( QHideEvent *event )
{
    clearView();
    KConfigGroup config = Amarok::config( "Cover Fetcher" );
    saveDialogSize( config );
    event->accept();
}

void CoverFoundDialog::clearQueryButtonClicked()
{
    m_query = QString();
    m_queryPage = 0;
    updateGui();
}

void CoverFoundDialog::clearView()
{
    m_view->clear();
    m_sideBar->clear();
    updateGui();
}

void CoverFoundDialog::itemSelected()
{
    CoverFoundItem *it = dynamic_cast< CoverFoundItem* >( m_view->currentItem() );
    m_pixmap = it->hasBigPix() ? it->bigPix() : it->thumb();
    m_sideBar->setPixmap( m_pixmap, it->metadata() );
}


void CoverFoundDialog::itemDoubleClicked( QListWidgetItem *item )
{
    Q_UNUSED( item )
    saveRequested();
}

void CoverFoundDialog::itemMenuRequested( const QPoint &pos )
{
    const QPoint globalPos = m_view->mapToGlobal( pos );
    QModelIndex index = m_view->indexAt( pos );

    if( !index.isValid() )
        return;

    CoverFoundItem *item = dynamic_cast< CoverFoundItem* >( m_view->item( index.row() ) );
    item->setSelected( true );

    QMenu menu( this );
    QAction *display = new QAction( KIcon("zoom-original"), i18n("Display Cover"), &menu );
    connect( display, SIGNAL(triggered()), item, SLOT(display()) );

    menu.addAction( display );
    menu.exec( globalPos );
}

void CoverFoundDialog::saveRequested()
{
    CoverFoundItem *item = dynamic_cast< CoverFoundItem* >( m_view->currentItem() );
    if( item && !item->hasBigPix() )
    {
        item->fetchBigPix();
        m_pixmap = item->bigPix();
    }
    KDialog::accept();
}

void CoverFoundDialog::processQuery()
{
    const QString text = m_search->text();
    processQuery( text );
}

void CoverFoundDialog::processQuery( const QString &input )
{
    const bool inputEmpty( input.isEmpty() );
    const bool mQueryEmpty( m_query.isEmpty() );

    QString q;
    if( inputEmpty && !mQueryEmpty )
    {
        q = m_query;
        m_queryPage++;
    }
    else if( !inputEmpty || !mQueryEmpty )
    {
        q = input;
        if( m_query == input )
        {
            m_queryPage++;
        }
        else
        {
            m_query = input;
            m_queryPage = 0;
        }
    }

    if( !q.isEmpty() )
        emit newCustomQuery( q, m_queryPage );
}

void CoverFoundDialog::selectDiscogs()
{
    KConfigGroup config = Amarok::config( "Cover Fetcher" );
    config.writeEntry( "Interactive Image Source", "Discogs" );
    m_queryPage = 0;
}

void CoverFoundDialog::selectLastFm()
{
    KConfigGroup config = Amarok::config( "Cover Fetcher" );
    config.writeEntry( "Interactive Image Source", "LastFm" );
    m_queryPage = 0;
}

void CoverFoundDialog::selectYahoo()
{
    KConfigGroup config = Amarok::config( "Cover Fetcher" );
    config.writeEntry( "Interactive Image Source", "Yahoo" );
    m_queryPage = 0;
}

void CoverFoundDialog::selectGoogle()
{
    KConfigGroup config = Amarok::config( "Cover Fetcher" );
    config.writeEntry( "Interactive Image Source", "Google" );
    m_queryPage = 0;
}

void CoverFoundDialog::setupSearchToolTip()
{
    const KShortcut textShortcut = KStandardShortcut::completion();
    const KShortcut nextShortcut = KStandardShortcut::nextCompletion();
    const KShortcut prevShortcut = KStandardShortcut::prevCompletion();
    const KShortcut subShortcut  = KStandardShortcut::substringCompletion();

    const QString &textKey = textShortcut.toString( QKeySequence::NativeText );
    const QString &nextKey = nextShortcut.toString( QKeySequence::NativeText );
    const QString &prevKey = prevShortcut.toString( QKeySequence::NativeText );
    const QString &subKey  = subShortcut.toString( QKeySequence::NativeText );

    const QString tt = i18n( "<b>Useful text completion shortcuts:</b><br> %1 (%2)<br> %3 (%4)<br> %5 (%6)<br> %7 (%8)",
                             i18n( "Text completion" ), textKey,
                             i18n( "Switch to previous completion" ), nextKey,
                             i18n( "Switch to next completion" ), prevKey,
                             i18n( "Substring completion" ), subKey
                             );

    m_search->setToolTip( tt );
}

void CoverFoundDialog::updateSearchButton( const QString &text )
{
    const bool isNewSearch = ( text != m_query ) ? true : false;
    m_searchButton->setGuiItem( isNewSearch ? KStandardGuiItem::find() : KStandardGuiItem::cont() );
    m_searchButton->setToolTip( isNewSearch ? i18n( "Search" ) : i18n( "Search For More Results" ) );
}

void CoverFoundDialog::updateGui()
{
    updateTitle();

    if( !m_search->hasFocus() )
        setButtonFocus( KDialog::Ok );
    update();
}

void CoverFoundDialog::updateTitle()
{
    const int itemCount = m_view->count();
    const QString caption = ( itemCount == 0 )
                          ? i18n( "No Images Found" )
                          : i18np( "1 Image Found", "%1 Images Found", itemCount );
    setCaption( caption );
}

void CoverFoundDialog::add( const QPixmap cover,
                            const CoverFetch::Metadata metadata,
                            const CoverFetch::ImageSize imageSize )
{
    if( cover.isNull() )
        return;

    CoverFoundItem *item = new CoverFoundItem( cover, metadata, imageSize );
    connect( item, SIGNAL(pixmapChanged(const QPixmap)), m_sideBar, SLOT(setPixmap(const QPixmap)) );
    m_view->addItem( item );
    updateGui();
}

CoverFoundSideBar::CoverFoundSideBar( const Meta::AlbumPtr album, QWidget *parent )
    : KVBox( parent )
    , m_album( album )
{
    m_cover = new QLabel( this );
    m_tabs  = new QTabWidget( this );
    m_notes = new QLabel( m_tabs );
    m_metaTable = new QTableWidget( m_tabs );
    m_notes->setAlignment( Qt::AlignLeft | Qt::AlignTop );
    m_notes->setMargin( 4 );
    m_notes->setOpenExternalLinks( true );
    m_notes->setTextFormat( Qt::RichText );
    m_notes->setTextInteractionFlags( Qt::TextBrowserInteraction );
    m_notes->setWordWrap( true );
    m_cover->setAlignment( Qt::AlignCenter );
    m_metaTable->setColumnCount( 2 );
    m_metaTable->horizontalHeader()->setVisible( false );
    m_metaTable->verticalHeader()->setVisible( false );
    m_tabs->addTab( m_metaTable, i18n( "Information" ) );
    m_tabs->addTab( m_notes, i18n( "Notes" ) );
    setMaximumWidth( 200 );
    setPixmap( m_album->image() );
    clear();
}

CoverFoundSideBar::~CoverFoundSideBar()
{
}

void CoverFoundSideBar::clear()
{
    m_metaTable->clear();
    m_notes->clear();
    m_metadata.clear();
}

void CoverFoundSideBar::setPixmap( const QPixmap pixmap, CoverFetch::Metadata metadata )
{
    m_metadata = metadata;
    updateNotes();
    setPixmap( pixmap );
}

void CoverFoundSideBar::setPixmap( const QPixmap pixmap )
{
    m_pixmap = pixmap;
    QPixmap scaledPix = pixmap.scaled( QSize( 190, 190 ), Qt::KeepAspectRatio );
    QPixmap prettyPix = The::svgHandler()->addBordersToPixmap( scaledPix, 5, QString(), true );
    m_cover->setPixmap( prettyPix );
    updateMetaTable();
}

void CoverFoundSideBar::updateNotes()
{
    bool enableNotes( false );
    if( m_metadata.contains( "notes" ) )
    {
        const QString notes = m_metadata.value( "notes" );
        if( !notes.isEmpty() )
        {
            m_notes->setText( notes );
            enableNotes = true;
        }
        else
            enableNotes = false;
    }
    else
    {
        m_notes->clear();
        enableNotes = false;
    }
    m_tabs->setTabEnabled( m_tabs->indexOf( m_notes ), enableNotes );
}

void CoverFoundSideBar::updateMetaTable()
{
    // TODO: clean up tags displayed when the sidebar info area is improved
    QStringList tags;
    tags << "artist"    << "clickurl"  << "country" << "date" << "format"
         << "height"    << "imgrefurl" << "name"    << "type" << "released"
         << "releaseid" << "size"      << "title"   << "url"  << "width";

    m_metaTable->clear();
    m_metaTable->setRowCount( tags.size() );

    int row( 0 );
    foreach( const QString &tag, tags )
    {
        QTableWidgetItem *itemTag( 0 );
        QTableWidgetItem *itemVal( 0 );

        if( m_metadata.contains( tag ) )
        {
            const QString value = m_metadata.value( tag );
            if( value.isEmpty() )
                continue;

            itemTag = new QTableWidgetItem( i18n( tag.toAscii() ) );
            itemVal = new QTableWidgetItem( value );
        }
        else if( tag == "width" )
        {
            itemTag = new QTableWidgetItem( i18n( "width" ) );
            itemVal = new QTableWidgetItem( QString::number( m_pixmap.width() ) );
        }
        else if( tag == "height" )
        {
            itemTag = new QTableWidgetItem( i18n( "height" ) );
            itemVal = new QTableWidgetItem( QString::number( m_pixmap.height() ) );
        }
        else
        {
            continue;
        }

        if( itemTag && itemVal )
        {
            m_metaTable->setItem( row, 0, itemTag );
            m_metaTable->setItem( row, 1, itemVal );
            row++;
        }
    }
    m_metaTable->setRowCount( row );
    m_metaTable->sortItems( 0 );
    m_metaTable->resizeColumnsToContents();
    m_metaTable->resizeRowsToContents();
}

CoverFoundItem::CoverFoundItem( const QPixmap cover,
                                const CoverFetch::Metadata data,
                                const CoverFetch::ImageSize imageSize,
                                QListWidget *parent )
    : QListWidgetItem( parent )
    , m_metadata( data )
    , m_dialog( 0 )
    , m_progress( 0 )
{
    switch( imageSize )
    {
    default:
    case CoverFetch::NormalSize:
        m_bigPix = cover;
        break;
    case CoverFetch::ThumbSize:
        m_thumb = cover;
        break;
    }

    QPixmap scaledPix = cover.scaled( QSize( 120, 120 ), Qt::KeepAspectRatio );
    QPixmap prettyPix = The::svgHandler()->addBordersToPixmap( scaledPix, 5, QString(), true );
    setSizeHint( QSize( 140, 150 ) );
    setIcon( prettyPix );
    setCaption();
    setFont( KGlobalSettings::smallestReadableFont() );
    setTextAlignment( Qt::AlignHCenter | Qt::AlignTop );
}

CoverFoundItem::~CoverFoundItem()
{
    delete m_progress;
    m_progress = 0;
    delete m_dialog;
    m_dialog = 0;
}

void CoverFoundItem::fetchBigPix()
{
    const KUrl url( m_metadata.value( "normalarturl" ) );
    KJob* job = KIO::storedGet( url, KIO::NoReload, KIO::HideProgressInfo );
    connect( job, SIGNAL(result(KJob*)), SLOT(slotFetchResult(KJob*)) );

    if( !url.isValid() || !job )
        return;

    if( !m_dialog )
        m_dialog = new KDialog( listWidget() );
    m_dialog->setCaption( i18n( "Fetching Large Cover" ) );
    m_dialog->setButtons( KDialog::Cancel );
    m_dialog->setDefaultButton( KDialog::Cancel );

    if( !m_progress )
        m_progress = new KJobProgressBar( m_dialog, job );
    m_progress->cancelButton()->hide();
    m_progress->descriptionLabel()->hide();
    connect( m_dialog, SIGNAL(cancelClicked()), m_progress, SLOT(cancel()) );
    connect( m_dialog, SIGNAL(cancelClicked()), job, SLOT(kill()) );

    m_dialog->setMainWidget( m_progress );
    m_dialog->exec();
}

void CoverFoundItem::display()
{
    if( !hasBigPix() )
        fetchBigPix();

    QWidget *p = dynamic_cast<QWidget*>( parent() );
    int parentScreen = KApplication::desktop()->screenNumber( p );

    const QPixmap pixmap = hasBigPix() ? m_bigPix : m_thumb;
    ( new CoverViewDialog( pixmap, QApplication::desktop()->screen( parentScreen ) ) )->show();
}

void CoverFoundItem::slotFetchResult( KJob *job )
{
    KIO::StoredTransferJob *const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    const QByteArray data = storedJob->data();
    QPixmap pixmap;
    if( pixmap.loadFromData( data ) )
    {
        m_bigPix = pixmap;
        emit pixmapChanged( m_bigPix );
    }

    if( m_dialog )
    {
        m_dialog->accept();
        delete m_progress;
        m_progress = 0;
        delete m_dialog;
        m_dialog = 0;
    }
    storedJob->deleteLater();
}

void CoverFoundItem::setCaption()
{
    QStringList captions;
    const QString width = m_metadata.value( "width" );
    const QString height = m_metadata.value( "height" );
    if( !width.isEmpty() && !height.isEmpty() )
        captions << QString( "%1 x %2" ).arg( width ).arg( height );

    int size = m_metadata.value( "size" ).toInt();
    if( size )
    {
        const QString source = m_metadata.value( "source" );
        if( source == "Yahoo!" )
            size /= 1024;

        captions << ( QString::number( size ) + 'k' );
    }

    if( !captions.isEmpty() )
        setText( captions.join( QString( " - " ) ) );
}

#include "CoverFoundDialog.moc"
