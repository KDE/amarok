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
#include <KHBox>
#include <KIO/Job>
#include <KLineEdit>
#include <KListWidget>
#include <KPushButton>
#include <KStandardDirs>
#include <KVBox>

#include <QCloseEvent>
#include <QDir>
#include <QFrame>
#include <QGridLayout>
#include <QMenu>

#define DEBUG_PREFIX "CoverFoundDialog"

CoverFoundDialog::CoverFoundDialog( Meta::AlbumPtr album, 
                                    const QPixmap cover,
                                    const CoverFetch::Metadata data,
                                    QWidget *parent )
    : KDialog( parent )
    , m_album( album )
{
    setButtons( KDialog::Ok | KDialog::Details | KDialog::Cancel |
                KDialog::User1 ); // User1: clear icon view

    setButtonGuiItem( KDialog::User1, KStandardGuiItem::clear() );
    connect( button( KDialog::User1 ), SIGNAL(clicked()), SLOT(clearView()) );

    m_save = button( KDialog::Ok );

    KVBox *box = new KVBox( this );
    box->setSpacing( 4 );

    KHBox *searchBox = new KHBox( box );
    box->setSpacing( 4 );

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

    m_view = new KListWidget( box );
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

    connect( m_view, SIGNAL(itemClicked(QListWidgetItem*)),
             this,   SLOT(itemClicked(QListWidgetItem*)) );
    connect( m_view, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
             this,   SLOT(itemDoubleClicked(QListWidgetItem*)) );
    connect( m_view, SIGNAL(customContextMenuRequested(const QPoint&)),
             this,   SLOT(itemMenuRequested(const QPoint&)) );

    QFrame *m_details = new QFrame( this );
    m_details->setFrameShadow( QFrame::Plain );
    m_details->setFrameShape( QFrame::Box );

    QLabel *artistLabel = new QLabel( "<b>" + i18n( "Artist" ) + "</b>", m_details );
    QLabel *albumLabel  = new QLabel( "<b>" + i18n( "Album"  ) + "</b>", m_details );
    QLabel *urlLabel    = new QLabel( "<b>" + i18n( "URL"    ) + "</b>", m_details );
    QLabel *artistText  = new QLabel( m_details );
    QLabel *albumText   = new QLabel( m_details );
    QLabel *urlText     = new QLabel( m_details );

    artistLabel->setAlignment( Qt::AlignRight );
    albumLabel->setAlignment( Qt::AlignRight );
    urlLabel->setAlignment( Qt::AlignRight );
    artistText->setTextInteractionFlags( Qt::TextBrowserInteraction );
    albumText->setTextInteractionFlags( Qt::TextBrowserInteraction );
    urlText->setTextInteractionFlags( Qt::TextBrowserInteraction );
    urlText->setOpenExternalLinks( true );

    m_detailsLayout = new QGridLayout( m_details );
    m_detailsLayout->addWidget( artistLabel, 0, 0 );
    m_detailsLayout->addWidget( albumLabel,  1, 0 );
    m_detailsLayout->addWidget( urlLabel,  2, 0 );
    m_detailsLayout->addWidget( artistText, 0, 1 );
    m_detailsLayout->addWidget( albumText, 1, 1 );
    m_detailsLayout->addWidget( urlText, 2, 1 );
    m_detailsLayout->setColumnStretch( 1, 1 );

    setMainWidget( box );
    setDetailsWidget( m_details );

    connect( m_save, SIGNAL(clicked()), SLOT(saveRequested()) );

    const KConfigGroup config = Amarok::config( "Cover Fetcher" );
    const QString source = config.readEntry( "Interactive Image Source", "LastFm" );
    restoreDialogSize( config ); // call this after setMainWidget()
    if( source == "LastFm" )
        lastFmAct->setChecked( true );
    else if( source == "Yahoo" )
        webSearchAct->setChecked( true );

    add( cover, data );
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

void CoverFoundDialog::itemClicked( QListWidgetItem *item )
{
    const CoverFoundItem *it = dynamic_cast< CoverFoundItem* >( item );
    m_pixmap = it->hasBigPix() ? it->bigPix() : it->thumb();
    updateDetails();
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
    updateDetails();

    if( !m_search->hasFocus() )
        setButtonFocus( KDialog::Ok );
    update();
}

void CoverFoundDialog::updateDetails()
{
    const CoverFoundItem *item = dynamic_cast< CoverFoundItem* >( m_view->currentItem() );
    if( !item )
        return;

    const CoverFetch::Metadata meta = item->metadata();
    QLabel *artistName = qobject_cast< QLabel * >( m_detailsLayout->itemAtPosition( 0, 1 )->widget() );
    QLabel *albumName  = qobject_cast< QLabel * >( m_detailsLayout->itemAtPosition( 1, 1 )->widget() );
    QLabel *urlName    = qobject_cast< QLabel * >( m_detailsLayout->itemAtPosition( 2, 1 )->widget() );

    artistName->setText( meta.value( "artist" ) );
    albumName->setText( meta.value( "name" ) );
    const QString urlText = QString( "<a href=\"%1\">source</a>, <a href=\"%2\">image</a>, <a href=\"%3\">thumb</a>" )
                                .arg( KUrl( meta.value( "url" ) ).url() )
                                .arg( KUrl( meta.value( "normalarturl" ) ).url() )
                                .arg( KUrl( meta.value( "thumbarturl" ) ).url() );
    urlName->setText( urlText );
}

void CoverFoundDialog::updateTitle()
{
    const int itemCount = m_view->count();
    const QString caption = ( itemCount == 0 )
                          ? i18n( "Cover Not Found" )
                          : i18np( "1 Cover Found", "%1 Covers Found", itemCount );
    setCaption( caption );
}

void CoverFoundDialog::add( const QPixmap cover,
                            const CoverFetch::Metadata metadata,
                            const CoverFetch::ImageSize imageSize )
{
    if( cover.isNull() )
        return;

    CoverFoundItem *item = new CoverFoundItem( cover, metadata, imageSize );

    const QString size = QString( "%1x%2" )
        .arg( QString::number( cover.width() ) )
        .arg( QString::number( cover.height() ) );
    const QString tip = i18n( "Size:" ) + size;
    item->setToolTip( tip );

    m_view->addItem( item );

    updateGui();
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
