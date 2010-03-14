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

CoverFoundDialog::CoverFoundDialog( Meta::AlbumPtr album, 
                                    const QPixmap cover,
                                    const CoverFetch::Metadata data,
                                    QWidget *parent )
    : KDialog( parent )
    , m_album( album )
{
    setButtons( KDialog::Ok | KDialog::Cancel |
                KDialog::User1 ); // User1: clear icon view

    setButtonGuiItem( KDialog::User1, KStandardGuiItem::clear() );
    connect( button( KDialog::User1 ), SIGNAL(clicked()), SLOT(clearView()) );

    m_save = button( KDialog::Ok );

    QSplitter *splitter = new QSplitter( this );
    m_sideBar = new CoverFoundSideBar( splitter );

    KVBox *vbox = new KVBox( splitter );
    vbox->setSpacing( 4 );

    KHBox *searchBox = new KHBox( vbox );
    vbox->setSpacing( 4 );

    m_search = new KLineEdit( searchBox );
    m_search->setClearButtonShown( true );
    m_search->setClickMessage( i18n( "Enter Custom Search" ) );
    m_search->setTrapReturnKey( true );

    KCompletion *searchComp = m_search->completionObject();
    searchComp->setOrder( KCompletion::Insertion );
    searchComp->setIgnoreCase( true );

    QStringList completionNames;
    completionNames << m_album->name();
    if( m_album->hasAlbumArtist() )
        completionNames << m_album->albumArtist()->name();
    searchComp->setItems( completionNames );

    KPushButton *searchButton = new KPushButton( KStandardGuiItem::find(), searchBox );
    KPushButton *sourceButton = new KPushButton( KStandardGuiItem::configure(), searchBox );

    QMenu *sourceMenu = new QMenu( sourceButton );
    QAction *lastFmAct = new QAction( i18n( "Last.fm" ), sourceMenu );
    QAction *webSearchAct = new QAction( i18n( "Web Search" ), sourceMenu );
    lastFmAct->setCheckable( true );
    webSearchAct->setCheckable( true );
    connect( lastFmAct, SIGNAL(triggered()), this, SLOT(selectLastFmSearch()) );
    connect( webSearchAct, SIGNAL(triggered()), this, SLOT(selectWebSearch()) );

    QActionGroup *ag = new QActionGroup( sourceButton );
    ag->addAction( lastFmAct );
    ag->addAction( webSearchAct );
    sourceMenu->addActions( ag->actions() );
    sourceButton->setMenu( sourceMenu ); // TODO: link actions to choose source when implemented

    connect( m_search,   SIGNAL(returnPressed(const QString&)),
             searchComp, SLOT(addItem(const QString&)) );
    connect( m_search, SIGNAL(returnPressed(const QString&)),
             this,     SIGNAL(newCustomQuery(const QString&)) );
    connect( searchButton, SIGNAL(pressed()),
             this,         SLOT(searchButtonPressed()) );

    m_view = new KListWidget( vbox );
    m_view->setAcceptDrops( false );
    m_view->setContextMenuPolicy( Qt::CustomContextMenu );
    m_view->setDragDropMode( QAbstractItemView::NoDragDrop );
    m_view->setDragEnabled( false );
    m_view->setDropIndicatorShown( false );
    m_view->setMovement( QListView::Static );
    m_view->setGridSize( QSize( 140, 140 ) );
    m_view->setIconSize( QSize( 120, 120 ) );
    m_view->setSpacing( 4 );
    m_view->setViewMode( QListView::IconMode );
    m_view->setResizeMode( QListView::Adjust );

    connect( m_view, SIGNAL(itemSelectionChanged()),
             this,   SLOT(itemSelected()) );
    connect( m_view, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
             this,   SLOT(itemDoubleClicked(QListWidgetItem*)) );
    connect( m_view, SIGNAL(customContextMenuRequested(const QPoint&)),
             this,   SLOT(itemMenuRequested(const QPoint&)) );

    splitter->addWidget( m_sideBar );
    splitter->addWidget( vbox );
    setMainWidget( splitter );

    connect( m_save, SIGNAL(clicked()), SLOT(saveRequested()) );

    const KConfigGroup config = Amarok::config( "Cover Fetcher" );
    const QString source = config.readEntry( "Interactive Image Source", "LastFm" );
    restoreDialogSize( config ); // call this after setMainWidget()
    if( source == "LastFm" )
        lastFmAct->setChecked( true );
    else if( source == "Yahoo" )
        webSearchAct->setChecked( true );

    add( cover, data );
    m_view->setCurrentItem( m_view->item( 0 ) );
}

void CoverFoundDialog::hideEvent( QHideEvent *event )
{
    clearView();
    KConfigGroup config = Amarok::config( "Cover Fetcher" );
    saveDialogSize( config );
    event->accept();
}

void CoverFoundDialog::clearView()
{
    m_view->clear();
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
    KDialog::accept();
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

void CoverFoundDialog::searchButtonPressed()
{
    const QString text = m_search->text();
    emit newCustomQuery( text );
}

void CoverFoundDialog::selectLastFmSearch()
{
    KConfigGroup config = Amarok::config( "Cover Fetcher" );
    config.writeEntry( "Interactive Image Source", "LastFm" );
}

void CoverFoundDialog::selectWebSearch()
{
    KConfigGroup config = Amarok::config( "Cover Fetcher" );
    config.writeEntry( "Interactive Image Source", "Yahoo" );
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

    const QString src = metadata.value( "source" );
    const QString w = metadata.contains( "width" ) ? metadata.value( "width" ) : QString::number( cover.width() );
    const QString h = metadata.contains( "height" ) ? metadata.value( "height" ) : QString::number( cover.height() );
    const QString size = QString( "%1x%2" ).arg( w ).arg( h );
    const QString tip = i18n( "Size:" ) + size;
    item->setToolTip( tip );

    m_view->addItem( item );

    updateGui();
}

CoverFoundSideBar::CoverFoundSideBar( QWidget *parent )
    : KVBox( parent )
{
    m_cover     = new QLabel( this );
    m_tabs      = new QTabWidget( this );
    m_abstract  = new QLabel( m_tabs );
    m_metaTable = new QTableWidget( m_tabs );
    m_abstract->setAlignment( Qt::AlignLeft | Qt::AlignTop );
    m_abstract->setMargin( 4 );
    m_abstract->setOpenExternalLinks( true );
    m_abstract->setTextInteractionFlags( Qt::TextBrowserInteraction );
    m_abstract->setWordWrap( true );
    m_cover->setAlignment( Qt::AlignCenter );
    m_metaTable->setColumnCount( 2 );
    m_metaTable->horizontalHeader()->setVisible( false );
    m_metaTable->verticalHeader()->setVisible( false );
    m_tabs->addTab( m_metaTable, i18n( "Information" ) );
    m_tabs->addTab( m_abstract, i18n( "Abstract" ) );
    setMaximumWidth( 200 );
    setNoCover();
}

CoverFoundSideBar::~CoverFoundSideBar()
{
}

void CoverFoundSideBar::setNoCover()
{
    if( m_noCover.isNull() )
        m_noCover = noCover();

    m_cover->setPixmap( m_noCover );
    m_metadata.clear();
}

void CoverFoundSideBar::setPixmap( const QPixmap pixmap, CoverFetch::Metadata metadata )
{
    setPixmap( pixmap );
    m_metadata = metadata;
    updateMetaTable();
    updateAbstract();
}

void CoverFoundSideBar::setPixmap( const QPixmap pixmap )
{
    m_pixmap = pixmap;
    QPixmap scaledPix = pixmap.scaled( QSize( 190, 190 ), Qt::KeepAspectRatio );
    QPixmap prettyPix = The::svgHandler()->addBordersToPixmap( scaledPix, 5, QString(), true );
    m_cover->setPixmap( prettyPix );
}

void CoverFoundSideBar::updateAbstract()
{
    bool enableAbstract( false );
    if( m_metadata.contains( "abstract" ) )
    {
        const QString abstract = m_metadata.value( "abstract" );
        if( !abstract.isEmpty() )
        {
            m_abstract->setText( abstract );
            enableAbstract = true;
        }
        else
            enableAbstract = false;
    }
    else
    {
        m_abstract->clear();
        enableAbstract = false;
    }
    m_tabs->setTabEnabled( m_tabs->indexOf( m_abstract ), enableAbstract );
}

void CoverFoundSideBar::updateMetaTable()
{
    QStringList tags;
    tags << "artist" << "clickurl" << "date"  << "format" << "height"
         << "name"   << "size"     << "title" << "url"    << "width";

    m_metaTable->clear();
    m_metaTable->setRowCount( 10 );

    int row( 0 );
    foreach( const QString &tag, tags )
    {
        QTableWidgetItem *itemTag( 0 );
        QTableWidgetItem *itemVal( 0 );

        if( m_metadata.contains( tag ) )
        {
            const QString value = m_metadata.value( tag );
            itemTag = new QTableWidgetItem( i18n( tag.toAscii() ) );
            itemVal = new QTableWidgetItem( value );
        }
        else if( tag == "width" )
        {
            itemTag = new QTableWidgetItem( i18n( tag.toAscii() ) );
            itemVal = new QTableWidgetItem( QString::number( m_pixmap.width() ) );
        }
        else if( tag == "height" )
        {
            itemTag = new QTableWidgetItem( i18n( tag.toAscii() ) );
            itemVal = new QTableWidgetItem( QString::number( m_pixmap.height() ) );
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

QPixmap CoverFoundSideBar::noCover( int size )
{
    /* code from Meta::Album::image( int size ) */

    QPixmap pixmap( size, size );
    const QString sizeKey = QString::number( size ) + '@';
    const QDir cacheCoverDir = QDir( Amarok::saveLocation( "albumcovers/cache/" ) );

    if( cacheCoverDir.exists( sizeKey + "nocover.png" ) )
    {
        pixmap.load( cacheCoverDir.filePath( sizeKey + "nocover.png" ) );
    }
    else
    {
        QPixmap orgPixmap( KStandardDirs::locate( "data", "amarok/images/nocover.png" ) );
        //scaled() does not change the original image but returns a scaled copy
        pixmap = orgPixmap.scaled( size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation );
        pixmap.save( cacheCoverDir.filePath( sizeKey + "nocover.png" ), "PNG" );
    }
    return pixmap;
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
    setIcon( prettyPix );
}

CoverFoundItem::~CoverFoundItem()
{
    delete m_dialog;
    m_dialog = 0;
}

void CoverFoundItem::fetchBigPix()
{
    const KUrl url( m_metadata.value( "normalarturl" ) );
    KJob* job = KIO::storedGet( url, KIO::NoReload, KIO::HideProgressInfo );
    connect( job, SIGNAL(result(KJob*)), SLOT(slotFetchResult(KJob*)) );

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
    ( new CoverViewDialog( pixmap, QApplication::desktop()->screen( parentScreen ) ) )->exec();
}

void CoverFoundItem::slotFetchResult( KJob *job )
{
    KIO::StoredTransferJob *const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    const QByteArray data = storedJob->data();
    QPixmap pixmap;
    if( pixmap.loadFromData( data ) )
    {
        m_bigPix = pixmap;
        const QString w = QString::number( pixmap.width() );
        const QString h = QString::number( pixmap.height() );
        const QString size = QString( "%1x%2" ).arg( w ).arg( h );
        const QString tip = i18n( "Size:" ) + size;
        setToolTip( tip );
        emit pixmapChanged( m_bigPix );
    }

    if( m_dialog )
    {
        m_dialog->accept();
        delete m_dialog;
        m_dialog = 0;
    }
    storedJob->deleteLater();
}

#include "CoverFoundDialog.moc"
