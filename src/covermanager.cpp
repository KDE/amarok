// (c) Pierpaolo Di Panfilo 2004
// See COPYING file for licensing information

#include "covermanager.h"
#include "collectiondb.h"

#include <qfile.h>
#include <qfontmetrics.h>    //paintItem()
#include <qlabel.h>
#include <qlayout.h>
#include <qpainter.h>    //paintItem()
#include <qpalette.h>    //paintItem()
#include <qpixmap.h>
#include <qpoint.h>
#include <qrect.h>
#include <qsplitter.h>
#include <qstringlist.h>
#include <qtimer.h>    //search filter timer
#include <qtoolbutton.h>

#include <kapplication.h>
#include <kdebug.h>
#include <kfileitem.h>
#include <kiconloader.h>
#include <klineedit.h>    //search filter
#include <klistview.h>
#include <klocale.h>
#include <kmessagebox.h>    //showCoverMenu()
#include <kpopupmenu.h>    //showCoverMenu()
#include <kpushbutton.h>
#include <kstandarddirs.h>   //KGlobal::dirs()
#include <kurl.h>



CoverManager::CoverManager( QWidget *parent, const char *name )
    : QWidget( parent, name, WDestructiveClose )
    , m_db( new CollectionDB() )
    , m_filter( 0 )
    , m_previewJob( 0 )
{
    setCaption( i18n("Cover Manager") );
    
    QVBoxLayout *vbox = new QVBoxLayout( this );
    QSplitter *splitter = new QSplitter( this );
    
    //artist listview
    m_artistView = new KListView( splitter );
    m_artistView->addColumn("Albums By");
    m_artistView->setFullWidth( true );
    m_artistView->setRootIsDecorated( true );
    m_artistView->setSorting( -1 );    //no sort
    m_artistView->setMinimumWidth( 180 );
    
    KListViewItem *item = new KListViewItem( m_artistView, "All Artists" );
    item->setExpandable( true );
    item->setPixmap( 0, SmallIcon("personal") );
    
    //load artists from the collection db
    QStringList values;
    QStringList names;
    m_db->execSql("SELECT name FROM artist ORDER BY name;", &values, &names);
    
    if( !values.isEmpty() ) {
        for( uint i=0; i < values.count(); i++ )  {
            item = new KListViewItem( m_artistView, item, values[i] );
            item->setExpandable( true );
            item->setPixmap( 0, SmallIcon("personal") );
        }
    }
    
    
    QWidget *coverWidget = new QWidget( splitter );
    QVBoxLayout *viewBox = new QVBoxLayout( coverWidget );
    viewBox->setMargin(4);
    viewBox->setSpacing(4);
    QHBoxLayout *hbox = new QHBoxLayout( viewBox->layout() );
    
    //search line edit
    m_searchEdit = new KLineEdit( coverWidget );
    m_searchEdit->setPaletteForegroundColor( colorGroup().mid() );
    m_searchEdit->setText("Search here...");
    m_searchEdit->installEventFilter( this );
    m_timer = new QTimer( this );    //search filter timer

    //view tool button
    QToolButton *viewButton = new QToolButton(coverWidget);
    viewButton->setText("View");
    viewButton->setAutoRaise( true );
    // view menu
    m_viewMenu = new KPopupMenu( viewButton);
    m_viewMenu->insertItem( i18n("All albums"), AllAlbums );
    m_viewMenu->insertItem( i18n("Albums with cover"), AlbumsWithCover );
    m_viewMenu->insertItem( i18n("Albums without cover"), AlbumsWithoutCover );
    m_viewMenu->setItemChecked( AllAlbums, true );
    connect( m_viewMenu, SIGNAL( activated(int) ), SLOT( changeView(int) ) );
    viewButton->setPopup(m_viewMenu);
    viewButton->setPopupDelay(0);
    
    //fetch missing covers button
    m_fetchButton = new KPushButton( SmallIconSet("cdrom_unmount"), "Fetch missing covers", coverWidget );
    connect( m_fetchButton, SIGNAL(clicked()), SLOT(fetchMissingCovers()) );

    hbox->addWidget( m_searchEdit );
    hbox->addWidget(viewButton);
    hbox->addStretch();
    hbox->addWidget(m_fetchButton);
    
    //cover view
    m_coverView = new KIconView( coverWidget );
    m_coverView->setArrangement( QIconView::LeftToRight );
    m_coverView->setResizeMode( QIconView::Adjust );
    m_coverView->setSelectionMode( QIconView::Extended );
    m_coverView->arrangeItemsInGrid();
    m_coverView->setAutoArrange( TRUE );
    m_coverView->setItemsMovable( FALSE );
    m_coverView->setMode( KIconView::Select );
    
    //counter label
    m_counterLabel = new QLabel( coverWidget );
    m_counterLabel->setAlignment( AlignRight | SingleLine );
    m_counterLabel->setFrameStyle( QFrame::Panel | QFrame::Sunken );
    
    viewBox->addWidget( m_coverView );
    viewBox->addWidget( m_counterLabel );
    
    vbox->addWidget( splitter );
    
    // signals and slots connections
    connect( m_artistView, SIGNAL( expanded(QListViewItem *) ), SLOT( expandeItem(QListViewItem *) ) );
    connect( m_artistView, SIGNAL( collapsed(QListViewItem *) ), SLOT( collapseItem(QListViewItem *) ) );
    connect( m_artistView, SIGNAL( selectionChanged( QListViewItem * ) ), SLOT( slotArtistSelected( QListViewItem * ) ) );
    connect( m_coverView, SIGNAL( rightButtonPressed( QIconViewItem *, const QPoint & ) ), 
                SLOT(showCoverMenu(QIconViewItem *, const QPoint &)) );
    connect( m_coverView, SIGNAL( doubleClicked( QIconViewItem * ) ), 
                SLOT( coverItemDoubleClicked( QIconViewItem * ) ) );                  
    connect( m_timer, SIGNAL( timeout() ), SLOT( slotSetFilter() ) );
    connect( m_searchEdit, SIGNAL( textChanged( const QString& ) ), SLOT( slotSetFilterTimeout() ) );
    connect( m_db, SIGNAL( coverFetched(const QString &) ), SLOT( coverFetched(const QString &) ) );

    m_currentView = AllAlbums;
    m_artistView->setSelected( m_artistView->firstChild(), true );
    
    resize(610, 380);
}


CoverManager::~CoverManager()
{
    delete m_db;
    delete m_timer;
    if( m_previewJob )
        m_previewJob->kill();
        
    //TODO save window size
    //save view settings
    
}


void CoverManager::fetchMissingCovers()
{
    for( QIconViewItem *item = m_coverView->firstItem(); item; item = item->nextItem() ) {
        CoverViewItem *coverItem = static_cast<CoverViewItem*>(item);
        if( !coverItem->hasCover() )
            m_db->fetchCover( this, coverItem->artist() + " - " + coverItem->album() );
    }
}


void CoverManager::expandeItem( QListViewItem *item ) //SLOT
{
    if(!item) return;
    
    QStringList values;
    QStringList names;
    QString id;
    
    if( item == m_artistView->firstChild() ) {//All Artists
        id = "artist.id";
    } else
        id = QString::number( m_db->getValueID( "artist", item->text(0) ) );
    
    m_db->execSql("SELECT DISTINCT album.name FROM album, artist, tags "
                            "WHERE tags.album=album.id AND tags.artist="+id+" "
                            "ORDER BY album.name;"
                            , &values, &names );
     
    if( !values.isEmpty() ) {
        KListViewItem *after = 0;
        for( uint i=0; i < values.count(); i++ )  {
            if( !values[i].isEmpty() ) {
                after = new KListViewItem( item, after, values[i] );
                after->setPixmap( 0, SmallIcon("cdrom_unmount") );
            }
        }
    }
    
}


void CoverManager::collapseItem( QListViewItem *item ) //SLOT
{
    QListViewItem* child = item->firstChild();
    QListViewItem* childTmp;
     //delete all children
     while ( child ) {
        childTmp = child;
        child = child->nextSibling();
        delete childTmp;
    }       
}


void CoverManager::slotArtistSelected( QListViewItem *item ) //SLOT
{
    m_coverView->clear();
    m_coverItems.clear();
    if( m_previewJob ) {
        m_previewJob->kill();
        m_previewJob = 0;
    }

    QStringList values;
    QStringList names;

    bool allAlbums = (item == m_artistView->firstChild());
    QString command;
    if( allAlbums ) {
        command = "select DISTINCT artist.name, album.name FROM album,artist,tags "
                             "where tags.album=album.id AND tags.artist=artist.id ORDER BY album.name;";
                              
    } else {
        QString id = QString::number( m_db->getValueID( "artist", item->text(0) ) );
        command = "select DISTINCT album.name, '' FROM album,tags "
                             "where tags.album=album.id AND tags.artist="+id+" ORDER BY album.name;";
    }
    
    m_db->execSql(command, &values, &names);
                
    if( !values.isEmpty() ) {
        QPtrList<KFileItem> fileList;
        
        for( uint i=0; i < values.count();  i+=2 )  {
            if( !values[allAlbums ? i+1 : i].isEmpty() ) {   
                CoverViewItem *coverItem = new CoverViewItem( m_coverView, m_coverView->lastItem(), 
                                                                             allAlbums ? values[i] : item->text(0), values[ allAlbums ? i+1 : i ] );
                m_coverItems.append( coverItem );
                
                if( coverItem->hasCover() ) {
                    KURL url( coverItem->albumPath() );
                    KFileItem *file = new KFileItem( url, "image/png", 0 );
                    fileList.append( file );
                }
            }
        }
        
        if( fileList.count() ) {
            m_previewJob = new KIO::PreviewJob( fileList, 80, 80, 0, 0, true, true, 0 );
            connect(m_previewJob, SIGNAL( gotPreview(const KFileItem *, const QPixmap &) ),
                SLOT(slotGotPreview(const KFileItem *, const QPixmap &) ) );
            connect( m_previewJob, SIGNAL( result( KIO::Job* ) ), SLOT( previewJobFinished() ) );
        }
        
        updateCounter();
    }
    
}


void CoverManager::showCoverMenu( QIconViewItem *item, const QPoint &p ) //SLOT
{
    #define item static_cast<CoverViewItem*>(item)
    if( !item ) return;
    
    enum Id { FETCH, DELETE };
    
    KPopupMenu menu( this );
    menu.insertItem( i18n("Fetch cover"), FETCH );
    menu.insertSeparator();
    menu.insertItem( SmallIcon("editdelete"), i18n("Delete cover"), DELETE );
    
    if( !item->hasCover() )
        menu.setItemEnabled( false, DELETE);
        
    switch( menu.exec(p) ) {
        case FETCH:
            m_db->fetchCover( this, item->artist() + " - " + item->album() );
            break;
            
        case DELETE: {
            
            int button = KMessageBox::warningContinueCancel( this, 
                                i18n("Are you sure do you want to delete this cover?" ),
                                QString::null,
                                i18n("&Delete Cover") );

            if ( button == KMessageBox::Continue )
            {
                KURL url( item->albumPath() );
                KIO::DeleteJob* job = KIO::del( url );
                connect( job, SIGNAL( result( KIO::Job* ) ), SLOT( slotCoverDeleted() ) );
            }
            break;
        }
        default: ;
    }
    
    #undef item
}


void CoverManager::coverItemDoubleClicked( QIconViewItem *item ) //SLOT
{
    #define item static_cast<CoverViewItem*>(item)
    if( !item ) return;
    
    if( item->hasCover() ) {
    
        QWidget *widget = new QWidget( 0, 0, WDestructiveClose );
        widget->setCaption( item->album() );
        QPixmap coverPix( item->albumPath() );
        widget->setPaletteBackgroundPixmap( coverPix );
        widget->setMinimumSize( coverPix.size() );
        widget->setFixedSize( coverPix.size() );
        widget->show();
        
    }
    else
        m_db->fetchCover( this, item->artist() + " - " + item->album() );
    
    #undef item
}


void CoverManager::slotSetFilter() //SLOT
{
    m_filter = m_searchEdit->text();
    
    m_coverView->selectAll( false);
    QIconViewItem *item = m_coverView->firstItem();
    while ( item ) {
        QIconViewItem *tmp = item->nextItem();
        m_coverView->takeItem( item );
        item = tmp;
    }
    
    m_coverView->setAutoArrange( false );
    for( QIconViewItem *item = m_coverItems.first(); item; item = m_coverItems.next() ) {
        CoverViewItem *coverItem = static_cast<CoverViewItem*>(item);
        if( coverItem->album().contains( m_filter, FALSE ) || coverItem->artist().contains( m_filter, FALSE ) )
            m_coverView->insertItem( item, m_coverView->lastItem() );
    }
    m_coverView->setAutoArrange( true );
    
    m_coverView->arrangeItemsInGrid();
    updateCounter();
}


void CoverManager::slotSetFilterTimeout() //SLOT
{
    if ( m_timer->isActive() ) m_timer->stop();
    m_timer->start( 180, true );
}


void CoverManager::changeView( int id  ) //SLOT
{
    if( m_currentView == id ) return;
    
    //clear the iconview without deleting items
    m_coverView->selectAll( false);
    QIconViewItem *item = m_coverView->firstItem();
    while ( item ) {
        QIconViewItem *tmp = item->nextItem();
        m_coverView->takeItem( item );
        item = tmp;
    }
    
    m_coverView->setAutoArrange(false );
    for( QIconViewItem *item = m_coverItems.first(); item; item = m_coverItems.next() ) {
        bool show = false;
        CoverViewItem *coverItem = static_cast<CoverViewItem*>(item);
        if( !m_filter.isEmpty() ) {
            if( !coverItem->album().contains( m_filter, FALSE ) && !coverItem->artist().contains( m_filter, FALSE ) )
                continue;
        }
        
        if( id == AllAlbums )    //show all albums
            show = true;     
        else if( id == AlbumsWithCover && coverItem->hasCover() )    //show only albums with cover
            show = true;
        else if( id == AlbumsWithoutCover && !coverItem->hasCover() )   //show only albums without cover
            show = true;
            
        if( show )    m_coverView->insertItem( item, m_coverView->lastItem() );
    }
    m_coverView->setAutoArrange( true );
    
    m_viewMenu->setItemChecked( m_currentView, false );
    m_viewMenu->setItemChecked( id, true );

    m_coverView->arrangeItemsInGrid();
    m_currentView = id;
}


void CoverManager::slotGotPreview(const KFileItem *item, const QPixmap &pixmap) //SLOT
{
    QString path = item->url().path();
    kdDebug() << "got preview " << path << endl;
    
    for( QIconViewItem *item = m_coverItems.first(); item; item = m_coverItems.next() ) {
        CoverViewItem *coverItem = static_cast<CoverViewItem*>(item);
        if( coverItem->albumPath() == path )
            coverItem->updateCover( pixmap );
    }
}


void CoverManager::previewJobFinished()
{
    m_previewJob = 0;
}


void CoverManager::coverFetched( const QString &key )
{
    for( QIconViewItem *item = m_coverItems.first(); item; item = m_coverItems.next() ) {
        CoverViewItem *coverItem = static_cast<CoverViewItem*>(item);
        if( key == coverItem->artist() + " - " + coverItem->album() ) {
            QPtrList<KFileItem> fileList; 
            KFileItem *file = new KFileItem( KURL( coverItem->albumPath() ), "image/png", 0 );
            fileList.append( file );
            
            KIO::PreviewJob *job = new KIO::PreviewJob( fileList, 80, 80, 0, 0, true, true, 0 );
            connect(job, SIGNAL( gotPreview(const KFileItem *, const QPixmap &) ),
                SLOT(slotGotPreview(const KFileItem *, const QPixmap &) ) );
            return;
        }
    }
}


void CoverManager::slotCoverDeleted()
{
    CoverViewItem *item = static_cast<CoverViewItem*>(m_coverView->currentItem());
    if( !item ) return;
    
    item->updateCover( QPixmap() );
}


void CoverManager::updateCounter()
{
    int totalCounter = 0, missingCounter = 0;
    for( QIconViewItem *item = m_coverView->firstItem(); item; item = item->nextItem() ) {
        totalCounter++;
        if( !((CoverViewItem*)item)->hasCover() )
            missingCounter++;    //counter for albums without albums
    }
    
    QString text = QString::number( totalCounter );
    if( !m_filter.isEmpty() )
        text += i18n(" results for ") + "\"" + m_filter + "\"";
    else if( m_artistView->selectedItem() )
        text += i18n(" albums by ") + m_artistView->selectedItem()->text(0);
    
    if( missingCounter ) 
        text += i18n(" - ( <b>%3</b> without cover )" ).arg( missingCounter );
                
    m_counterLabel->setText( text );
    m_fetchButton->setEnabled( missingCounter != 0 );
}


bool CoverManager::eventFilter( QObject *o, QEvent *e )
{
    if( o == m_searchEdit ) {
        switch( e->type() ) {
           case QEvent::FocusIn:
               if( m_filter.isEmpty() ) {
                   m_searchEdit->clear();
                   m_timer->stop();
                   m_searchEdit->setPaletteForegroundColor( colorGroup().text() );
                   return FALSE;
               }
      
            case QEvent::FocusOut:
                if( m_filter.isEmpty() ) {
                    m_searchEdit->setPaletteForegroundColor( colorGroup().mid() );
                    m_searchEdit->setText( i18n("Search here...") );
                    m_timer->stop();
                    return FALSE;
                }
                
            default:
                return FALSE;
        };
    }
    
    return FALSE;
}



//////////////////////////////////////////////////////////////////////
//    CLASS CoverViewItem
/////////////////////////////////////////////////////////////////////

CoverViewItem::CoverViewItem( QIconView *parent, QIconViewItem *after, QString artist, QString album )
    : KIconViewItem( parent, after, album )
    , m_artist( artist )
    , m_album( album )
    , m_hasCover( false )
{
    calcRect();

    QFile file( albumPath() );
    if( file.exists() )
        m_hasCover = true;

    setPixmap( SmallIcon("image") );
}


CoverViewItem::~CoverViewItem()
{
}


void CoverViewItem::updateCover( const QPixmap &cover )
{
    m_hasCover = !cover.isNull();
    setPixmap( cover.isNull() ? SmallIcon("image") : cover );
}


QString CoverViewItem::albumPath()
{
    QString fileName = QString( "%1 - %2.png" ).arg(m_artist).arg(m_album);
    fileName.replace( "/", "_" );
    return KGlobal::dirs()->saveLocation( "data", kapp->instanceName() + '/' ) + "albumcovers/large/"+fileName;
}


void CoverViewItem::calcRect( const QString& )
{
    int thumbHeight = 80, thumbWidth=80;
    
    QFontMetrics fm = iconView()->fontMetrics();
    QRect itemPixmapRect( 5, 1, thumbWidth, thumbHeight );
    QRect itemRect = rect();
    itemRect.setWidth( thumbWidth + 10 );
    itemRect.setHeight( thumbHeight + fm.lineSpacing() + 2 );
    QRect itemTextRect( 0, thumbHeight+2, itemRect.width(), fm.lineSpacing() );
    
    setPixmapRect( itemPixmapRect );
    setTextRect( itemTextRect );
    setItemRect( itemRect );
}

 
void CoverViewItem::paintItem(QPainter* p, const QColorGroup& cg)
{
    QRect itemRect = rect();
    
    static QPixmap buffer;
    buffer.resize( itemRect.width(), itemRect.height() );

    if( buffer.isNull() )
    {
        QIconViewItem::paintItem(p, cg);
        return;
    }

    QPainter pBuf( &buffer, true );
    pBuf.fillRect( buffer.rect(), cg.base() );
    // draw the border
    pBuf.setPen( cg.mid() );
    pBuf.drawRect( 0, 0, itemRect.width(), pixmapRect().height()+2 );
    // draw the cover image
    QPixmap *cover = pixmap();
    pBuf.drawPixmap( pixmapRect().x() + (pixmapRect().width() - cover->width())/2, 
                                       pixmapRect().y() + (pixmapRect().height() - cover->height())/2, *cover );
    
    //justify the album name
    QString str = text();
    QFontMetrics fm = p->fontMetrics();
    int nameWidth = fm.width( str );
    if( nameWidth > textRect().width() )
    {
        QString nameJustify = "...";
        int i = 0;
        while ( fm.width( nameJustify + str[ i ] ) < textRect().width() )
            nameJustify += str[ i++ ];
        nameJustify.remove( 0, 3 );
        if ( nameJustify.isEmpty() )
            nameJustify = str.left( 1 );
        nameJustify += "...";
        str = nameJustify;
    }
    pBuf.setPen( cg.text() );
    pBuf.drawText( textRect(), Qt::AlignCenter, str );
    
    if( isSelected() ) {
       pBuf.setPen( cg.highlight() );
       pBuf.drawRect( pixmapRect() );
       pBuf.drawRect( pixmapRect().left()+1, pixmapRect().top()+1, pixmapRect().width()-2, pixmapRect().height()-2);
       pBuf.drawRect( pixmapRect().left()+2, pixmapRect().top()+2, pixmapRect().width()-4, pixmapRect().height()-4);
    }
    
    pBuf.end();
    
    p->drawPixmap( itemRect, buffer );
}
 
 
#include "covermanager.moc"
