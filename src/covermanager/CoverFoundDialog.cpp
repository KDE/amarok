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

#include "AlbumBreadcrumbWidget.h"
#include "core/support/Amarok.h"
#include "CoverViewDialog.h"
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
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QMenu>
#include <QScrollArea>
#include <QSplitter>
#include <QTabWidget>

#define DEBUG_PREFIX "CoverFoundDialog"
#include "core/support/Debug.h"

CoverFoundDialog::CoverFoundDialog( const CoverFetchUnit::Ptr unit,
                                    const QPixmap cover,
                                    const CoverFetch::Metadata data,
                                    QWidget *parent )
    : KDialog( parent )
    , m_album( unit->album() )
    , m_isSorted( false )
    , m_sortEnabled( false )
    , m_unit( unit )
    , m_queryPage( 1 )
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

    KHBox *breadcrumbBox = new KHBox( vbox );
    QLabel *breadcrumbLabel = new QLabel( i18n( "Finding cover for" ), breadcrumbBox );
    AlbumBreadcrumbWidget *breadcrumb = new AlbumBreadcrumbWidget( m_album, breadcrumbBox );

    QFont breadcrumbLabelFont;
    breadcrumbLabelFont.setBold( true );
    breadcrumbLabel->setFont( breadcrumbLabelFont );
    breadcrumbLabel->setIndent( 4 );

    connect( breadcrumb, SIGNAL(artistClicked(const QString&)), SLOT(addToCustomSearch(const QString&)) );
    connect( breadcrumb, SIGNAL(albumClicked(const QString&)), SLOT(addToCustomSearch(const QString&)) );

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

    m_sortAction = new QAction( i18n( "Sort by size" ), sourceMenu );
    m_sortAction->setCheckable( true );
    connect( m_sortAction, SIGNAL(triggered(bool)), this, SLOT(sortingTriggered(bool)) );

    QActionGroup *ag = new QActionGroup( sourceButton );
    ag->addAction( lastFmAct );
    ag->addAction( googleAct );
    ag->addAction( yahooAct );
    ag->addAction( discogsAct );
    sourceMenu->addActions( ag->actions() );
    sourceMenu->addSeparator();
    sourceMenu->addAction( m_sortAction );
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
    connect( m_view, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
             this,   SLOT(itemDoubleClicked(QListWidgetItem*)) );
    connect( m_view, SIGNAL(customContextMenuRequested(const QPoint&)),
             this,   SLOT(itemMenuRequested(const QPoint&)) );

    splitter->addWidget( m_sideBar );
    splitter->addWidget( vbox );
    setMainWidget( splitter );

    connect( m_save, SIGNAL(clicked(bool)), SLOT(saveRequested()) );

    const KConfigGroup config = Amarok::config( "Cover Fetcher" );
    const QString source = config.readEntry( "Interactive Image Source", "LastFm" );
    m_sortEnabled = config.readEntry( "Sort by Size", false );
    m_sortAction->setChecked( m_sortEnabled );
    m_isSorted = m_sortEnabled;
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
    KConfigGroup config = Amarok::config( "Cover Fetcher" );
    saveDialogSize( config );
    event->accept();
}

void CoverFoundDialog::add( const QPixmap cover,
                            const CoverFetch::Metadata metadata,
                            const CoverFetch::ImageSize imageSize )
{
    if( cover.isNull() )
        return;

    CoverFoundItem *item = new CoverFoundItem( cover, metadata, imageSize );
    connect( item, SIGNAL(pixmapChanged(const QPixmap)), m_sideBar, SLOT(setPixmap(const QPixmap)) );
    addToView( item );
}

void CoverFoundDialog::addToView( CoverFoundItem *const item )
{
    const CoverFetch::Metadata metadata = item->metadata();

    if( m_sortEnabled && metadata.contains( "width" ) && metadata.contains( "height" ) )
    {
        if( m_isSorted )
        {
            const int size = metadata.value( "width" ).toInt() * metadata.value( "height" ).toInt();
            QList< int >::iterator i = qLowerBound( m_sortSizes.begin(), m_sortSizes.end(), size );
            m_sortSizes.insert( i, size );
            const int index = m_sortSizes.count() - m_sortSizes.indexOf( size ) - 1;
            m_view->insertItem( index, item );
        }
        else
        {
            m_view->addItem( item );
            sortCoversBySize();
        }
    }
    else
    {
        m_view->addItem( item );
    }
    updateGui();
}

void CoverFoundDialog::addToCustomSearch( const QString &text )
{
    const QString &query = m_search->text();
    if( !text.isEmpty() && !query.contains( text ) )
    {
        QStringList q;
        if( !query.isEmpty() )
            q << query;
        q << text;
        const QString result = q.join( QChar( ' ' ) );
        m_search->setText( result );
    }
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
    m_sortSizes.clear();
    updateGui();
}

void CoverFoundDialog::itemSelected()
{
    CoverFoundItem *it = dynamic_cast< CoverFoundItem* >( m_view->currentItem() );
    if( it )
    {
        m_pixmap = it->hasBigPix() ? it->bigPix() : it->thumb();
        m_sideBar->setPixmap( m_pixmap, it->metadata() );
    }
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
    if( item )
    {
        bool gotBigPix( true );
        if( !item->hasBigPix() )
            gotBigPix = item->fetchBigPix();

        if( gotBigPix )
        {
            m_pixmap = item->bigPix();
            KDialog::accept();
        }
        else
        {
            m_pixmap = QPixmap();
            KDialog::reject();
        }
    }
    KDialog::reject();
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
    bool incrementPage( false );

    QString q;
    if( inputEmpty && !mQueryEmpty )
    {
        q = m_query;
        incrementPage = true;
    }
    else if( !inputEmpty || !mQueryEmpty )
    {
        q = input;
        if( m_query == input )
        {
            incrementPage = true;
        }
        else
        {
            m_query = input;
            m_queryPage = 0;
        }
    }

    if( !q.isEmpty() )
    {
        emit newCustomQuery( q, m_queryPage );
        updateSearchButton( q );
        m_queryPage++;
    }
}

void CoverFoundDialog::selectDiscogs()
{
    KConfigGroup config = Amarok::config( "Cover Fetcher" );
    config.writeEntry( "Interactive Image Source", "Discogs" );
    m_sortAction->setEnabled( true );
    m_queryPage = 0;
    processQuery();
    debug() << "Select Discogs as source";
}

void CoverFoundDialog::selectLastFm()
{
    KConfigGroup config = Amarok::config( "Cover Fetcher" );
    config.writeEntry( "Interactive Image Source", "LastFm" );
    m_sortAction->setEnabled( false );
    m_queryPage = 0;
    processQuery();
    debug() << "Select Last.fm as source";
}

void CoverFoundDialog::selectYahoo()
{
    KConfigGroup config = Amarok::config( "Cover Fetcher" );
    config.writeEntry( "Interactive Image Source", "Yahoo" );
    m_sortAction->setEnabled( true );
    m_queryPage = 0;
    processQuery();
    debug() << "Select Yahoo! as source";
}

void CoverFoundDialog::selectGoogle()
{
    KConfigGroup config = Amarok::config( "Cover Fetcher" );
    config.writeEntry( "Interactive Image Source", "Google" );
    m_sortAction->setEnabled( true );
    m_queryPage = 0;
    processQuery();
    debug() << "Select Google as source";
}

void CoverFoundDialog::sortingTriggered( bool checked )
{
    KConfigGroup config = Amarok::config( "Cover Fetcher" );
    config.writeEntry( "Sort by Size", checked );
    m_sortEnabled = checked;
    m_isSorted = false;
    if( m_sortEnabled )
        sortCoversBySize();
    debug() << "Enable sorting by size:" << checked;
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

void CoverFoundDialog::sortCoversBySize()
{
    DEBUG_BLOCK

    m_sortSizes.clear();
    QList< QListWidgetItem* > viewItems = m_view->findItems( QChar('*'), Qt::MatchWildcard );
    QMultiMap<int, CoverFoundItem*> sortItems;

    // get a list of cover items sorted (automatically by qmap) by size
    foreach( QListWidgetItem *viewItem, viewItems  )
    {
        CoverFoundItem *coverItem = dynamic_cast<CoverFoundItem*>( viewItem );
        const CoverFetch::Metadata meta = coverItem->metadata();
        const int itemSize = meta.value( "width" ).toInt() * meta.value( "height" ).toInt();
        sortItems.insert( itemSize, coverItem );
        m_sortSizes << itemSize;
    }

    // take items from the view and insert into a temp list in the sorted order
    QList<CoverFoundItem*> coverItems = sortItems.values();
    QList<CoverFoundItem*> tempItems;
    for( int i = 0, count = sortItems.count(); i < count; ++i )
    {
        CoverFoundItem *item = coverItems.value( i );
        const int itemRow = m_view->row( item );
        QListWidgetItem *itemFromRow = m_view->takeItem( itemRow );
        if( itemFromRow )
            tempItems << dynamic_cast<CoverFoundItem*>( itemFromRow );
    }

    // add the items back to the view in descending order
    foreach( CoverFoundItem* item, tempItems )
        m_view->insertItem( 0, item );

    m_isSorted = true;
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

CoverFoundSideBar::CoverFoundSideBar( const Meta::AlbumPtr album, QWidget *parent )
    : KVBox( parent )
    , m_album( album )
{
    m_cover = new QLabel( this );
    m_tabs  = new QTabWidget( this );
    m_notes = new QLabel( m_tabs );
    m_metaTable = new QWidget( m_tabs );
    m_metaTable->setLayout( new QFormLayout() );
    m_metaTable->setMinimumSize( QSize( 150, 200 ) );
    QScrollArea *metaArea = new QScrollArea( m_tabs );
    metaArea->setFrameShape( QFrame::NoFrame );
    metaArea->setWidget( m_metaTable );
    m_notes->setAlignment( Qt::AlignLeft | Qt::AlignTop );
    m_notes->setMargin( 4 );
    m_notes->setOpenExternalLinks( true );
    m_notes->setTextFormat( Qt::RichText );
    m_notes->setTextInteractionFlags( Qt::TextBrowserInteraction );
    m_notes->setWordWrap( true );
    m_cover->setAlignment( Qt::AlignCenter );
    m_tabs->addTab( metaArea, i18n( "Information" ) );
    m_tabs->addTab( m_notes, i18n( "Notes" ) );
    setMaximumWidth( 200 );
    setPixmap( m_album->image( 190 ) );
    clear();
}

CoverFoundSideBar::~CoverFoundSideBar()
{
}

void CoverFoundSideBar::clear()
{
    clearMetaTable();
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
    QStringList tags;
    tags << "artist" << "country"  << "date" << "format" << "height" << "name"
         << "type"   << "released" << "size" << "source" << "title"  << "width";

    clearMetaTable();

    QFormLayout *layout = qobject_cast< QFormLayout* >( m_metaTable->layout() );
    layout->setSizeConstraint( QLayout::SetMinAndMaxSize );

    CoverFetch::Metadata::const_iterator mit = m_metadata.constBegin();
    while( mit != m_metadata.constEnd() )
    {
        const QString tag = mit.key();
        if( tags.contains( tag ) )
        {
            const QString &value = mit.value();
            QLabel *label = new QLabel( value );
            label->setToolTip( value );
            layout->addRow( i18n( "<b>%1:</b>", tag ), label );
        }
        ++mit;
    }

    QString refUrl;
    QString refShowUrl; // only used by Yahoo atm

    const QString source = m_metadata.value( "source" );
    if( source == "Last.fm" || source == "Discogs" )
    {
        refUrl = m_metadata.value( "releaseurl" );
    }
    else if( source == "Google" )
    {
        refUrl = m_metadata.value( "imgrefurl" );
    }
    else if( source == "Yahoo!" )
    {
        refUrl = m_metadata.value( "refererclickurl" );
        refShowUrl = m_metadata.value( "refererurl" );
    }

    if( !refUrl.isEmpty() )
    {
        QFont font;
        QFontMetrics qfm( font );
        const QString &toolUrl = refShowUrl.isEmpty() ? refUrl : refShowUrl;
        const QString &tooltip = qfm.elidedText( toolUrl, Qt::ElideMiddle, 350 );
        const QString &decoded = QUrl::fromPercentEncoding( refUrl.toLocal8Bit() );
        const QString &url     = i18n( "<a href=\"%1\">link</a>", decoded );

        QLabel *label = new QLabel( url );
        label->setOpenExternalLinks( true );
        label->setTextInteractionFlags( Qt::TextBrowserInteraction );
        label->setToolTip( tooltip );
        layout->addRow( i18n( "<b>URL:</b>" ), label );
    }
}

void CoverFoundSideBar::clearMetaTable()
{
    QFormLayout *layout = qobject_cast< QFormLayout* >( m_metaTable->layout() );
    for( int i = 0, rowCount = layout->rowCount(); i < rowCount; ++i )
    {
        QLayoutItem *labelItem = layout->itemAt( i, QFormLayout::LabelRole );
        if( labelItem )
        {
            QWidget *widget = labelItem->widget();
            layout->removeItem( labelItem );
            if( widget )
            {
                layout->removeWidget( widget );
                delete widget;
            }
        }

        QLayoutItem *fieldItem = layout->itemAt( i, QFormLayout::FieldRole );
        if( fieldItem )
        {
            QWidget *widget = fieldItem->widget();
            layout->removeItem( fieldItem );
            if( widget )
            {
                layout->removeWidget( widget );
                delete widget;
            }
        }
    }

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

bool CoverFoundItem::fetchBigPix()
{
    DEBUG_BLOCK

    const KUrl url( m_metadata.value( "normalarturl" ) );
    KJob* job = KIO::storedGet( url, KIO::NoReload, KIO::HideProgressInfo );
    connect( job, SIGNAL(result(KJob*)), SLOT(slotFetchResult(KJob*)) );

    if( !url.isValid() || !job )
        return false;

    if( !m_dialog )
        m_dialog = new KDialog( listWidget() );
    m_dialog->setCaption( i18n( "Fetching Large Cover" ) );
    m_dialog->setButtons( KDialog::Cancel );
    m_dialog->setDefaultButton( KDialog::Cancel );
    m_dialog->setWindowModality( Qt::WindowModal );

    if( !m_progress )
        m_progress = new KJobProgressBar( m_dialog, job );
    m_progress->cancelButton()->hide();
    m_progress->descriptionLabel()->hide();
    connect( m_dialog, SIGNAL(cancelClicked()), m_progress, SLOT(cancel()) );
    connect( m_dialog, SIGNAL(cancelClicked()), job, SLOT(kill()) );

    m_dialog->setMainWidget( m_progress );
    return ( m_dialog->exec() == QDialog::Accepted ) ? true : false;
}

void CoverFoundItem::display()
{
    bool success( false );
    if( !hasBigPix() )
        success = fetchBigPix();
    else
        success = true;

    if( !success )
        return;

    const QPixmap pixmap = hasBigPix() ? m_bigPix : m_thumb;
    QPointer<CoverViewDialog> dlg = new CoverViewDialog( pixmap, listWidget() );
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
}

void CoverFoundItem::slotFetchResult( KJob *job )
{
    QPixmap pixmap;
    KIO::StoredTransferJob *const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    bool dataIsGood = pixmap.loadFromData( storedJob->data() );
    if( dataIsGood )
    {
        m_bigPix = pixmap;
        emit pixmapChanged( m_bigPix );
    }

    if( m_dialog )
    {
        if( dataIsGood )
            m_dialog->accept();
        else
            m_dialog->reject();

        m_progress->deleteLater();
        m_dialog->deleteLater();
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
