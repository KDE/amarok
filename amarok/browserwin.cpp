/***************************************************************************
                        browserwin.cpp  -  description
                           -------------------
  begin                : Fre Nov 15 2002
  copyright            : (C) 2002 by Mark Kretschmann
  email                : markey@web.de
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "amarokconfig.h"
#include "browserwin.h"
#include "expandbutton.h"
#include "filebrowser.h"
#include "playerapp.h"
#include "playlistwidget.h"
#include "streambrowser.h"

//PlaylistSideBar includes
#include <qpainter.h>      //PlaylistSideBar::TinyButton
#include <qpixmap.h>       //TinyButtons
#include <qsignalmapper.h> //m_mapper
#include <qstyle.h>        //PlaylistSideBar::PlaylistSideBar
#include <qtimer.h>        //autoClose()

#include <kiconloader.h>   //multiTabBar icons
#include <kmultitabbar.h>  //m_multiTabBar

//BrowserWin includes
#include <qcolor.h>        //setPalettes()
#include <qevent.h>        //eventFilter()
#include <qlayout.h>
#include <qobjectlist.h>   //setPaletteRecursively()
#include <qpalette.h>      //setPalettes()
#include <qsplitter.h>     //m_splitter
#include <qtooltip.h>      //QToolTip::add()
#include <qvbox.h>         //contains the playlist

#include <kaction.h>       //m_actionCollection
#include <kapplication.h>  //kapp
#include <kcursor.h>       //customEvent()
#include <kdebug.h>
#include <kglobal.h>
#include <kfiledialog.h>   //savePlaylist()
#include <klineedit.h>     //m_lineEdit
#include <klocale.h>
#include <kstandarddirs.h> //KGlobal::dirs()
#include <kurldrag.h>      //eventFilter()
#include <kurlrequester.h>    //slotAddLocation()
#include <kurlrequesterdlg.h> //slotAddLocation()


//Routine for setting palette recursively in a widget and all its childen
//NOTE I didn't make this a member as there was no need and we may like to move it at some point
static void setPaletteRecursively( QWidget* widget, const QPalette &pal, const QColor& bgAlt )
{
    QObjectList *list = widget->queryList( "QWidget" );
    list->append( widget );

    for( QObject *obj = list->first(); obj; obj = list->next() )
    {
        static_cast<QWidget*>(obj)->setPalette( pal );

        if( obj->inherits( "KListView" ) )
        {
            KListView *lv = dynamic_cast<KListView *>(obj); //slow, but safe
            if( lv ) lv->setAlternateBackground( bgAlt );
        }
    }
}


//<mxcl>
//I avoided using KDockWidget as it is horrendously large and we only wanted a subset
//of its functionality. But this turned out to be more code than I thought.
//Maybe this was a silly decision.. change it if you like.

/**
 * A mini-button usually placed in the dockpanel.
 *
 * @author Max Judin, Max Howell.
*/

PlaylistSideBar::TinyButton::TinyButton( QWidget *parent )
  : QPushButton( parent )
  , m_mouseOver( false )
{
    setFocusPolicy( NoFocus );
}

void PlaylistSideBar::TinyButton::drawButton( QPainter* p )
{
    p->fillRect( rect(), colorGroup().brush( QColorGroup::Button ) );
    p->drawPixmap( (width() - pixmap()->width()) / 2, (height() - pixmap()->height()) / 2, *pixmap() );

    //this is from kdockwidget_private.cpp and is now better code
    //TODO commit improvements

    if( isOn() || isDown() ) { //draw buttonPressed state
        p->setPen( colorGroup().dark() );
        p->moveTo( 0, height() - 1 );
        p->lineTo( 0, 0 );
        p->lineTo( width() - 1, 0 );

        p->setPen( colorGroup().light() );
        p->lineTo( width() - 1, height() - 1 );
        p->lineTo( 0, height() - 1 );
    }
    else if( m_mouseOver ) { //draw mouseOver state
        p->setPen( colorGroup().light() );
        p->moveTo( 0, height() - 1 );
        p->lineTo( 0, 0 );
        p->lineTo( width() - 1, 0 );

        p->setPen( colorGroup().dark() );
        p->lineTo( width() - 1, height() - 1 );
        p->lineTo( 0, height() - 1 );
    }
}

void PlaylistSideBar::TinyButton::enterEvent( QEvent * )
{
    m_mouseOver = true;
    repaint();
}

void PlaylistSideBar::TinyButton::leaveEvent( QEvent * )
{
    m_mouseOver = false;
    repaint();
}



/**
 * The SideBbar/MultTab/Tray all-in-one spectacular!
 *
 * @author Max Howell.
*/

//NOTE I use the "current" property of the QPtrList, m_widgets, to represent
//the currently visible widget. This had a number of advantages, one of
//which is not clarity of source! Apologies.
//NOTE I use the baseSize().width() property to remember page widths as it
//an otherwise unused property and this saves memory

//NOTE you can obtain pointers to the widgets you insert with addPage() using: page( "name" );

PlaylistSideBar::PlaylistSideBar( QWidget *parent )
    : QHBox( parent )
    , m_multiTabBar( new KMultiTabBar( KMultiTabBar::Vertical, this ) )
    , m_pageHolder( new QWidget( this ) ) //FIXME making this a layout would save mem
    , m_stayButton( 0 )
    , m_mapper( new QSignalMapper( this ) )
    , m_pages()
{
    m_multiTabBar->setStyle( KMultiTabBar::VSNET );
    m_multiTabBar->setPosition( KMultiTabBar::Left );
    m_multiTabBar->showActiveTabTexts( true );

    m_stayButton = new TinyButton( m_pageHolder );
    m_stayButton->setToggleButton( true );
    m_stayButton->setPixmap( const_cast< const char** >(not_close_xpm) );
    QToolTip::add( m_stayButton, i18n( "The tray stays open", "Stay" ) );

    QPushButton *closeButton = new TinyButton( m_pageHolder );
    closeButton->setPixmap( style().stylePixmap( QStyle::SP_TitleBarCloseButton , this) );
    QToolTip::add( closeButton, i18n( "Close" ) );
    connect( closeButton, SIGNAL( clicked() ), SLOT( close() ) );

    QSize buttonSize = closeButton->pixmap()->size();
    closeButton->setFixedSize( buttonSize );
    m_stayButton->setFixedSize( buttonSize );

    QVBoxLayout *mainLayout = new QVBoxLayout( m_pageHolder );
    QHBoxLayout *tinyLayout = new QHBoxLayout();
    tinyLayout->setAlignment( Qt::AlignRight );
    tinyLayout->addWidget( m_stayButton );
    tinyLayout->addWidget( closeButton );
    mainLayout->addLayout( tinyLayout );

    //set it to a default state of closed();
    m_pageHolder->hide();
    setMaximumWidth( m_multiTabBar->sizeHint().width() );

    KConfig *config = kapp->config();
    config->setGroup( "PlaylistSideBar" );
    m_stayButton->setOn( config->readBoolEntry( "Stay", true ) );

    connect( m_mapper, SIGNAL( mapped( int ) ), SLOT( showHidePage( int ) ) );
}

PlaylistSideBar::~PlaylistSideBar()
{
    KConfig *config = kapp->config();
    config->setGroup( "PlaylistSideBar" );
    config->writeEntry( "Stay", m_stayButton->isOn() );
    config->writeEntry( "CurrentPane", m_pages.current() ? m_pages.current()->name() : "" );

    for( QPtrList<QWidget>::ConstIterator it = m_pages.constBegin(); *it; ++it )
        config->writeEntry( (*it)->name(), (*it)->baseSize().width() );
}

void PlaylistSideBar::addPage( QWidget *widget, const QString &title, const QString& icon )
{
    //hi, this function is ugly - blame the monstrosity that is KMultiTabBar

    //determine next available id
    int id = m_multiTabBar->tabs()->count();
    QString name( widget->name() );

    widget->reparent( m_pageHolder, QPoint(), false );
    m_pageHolder->layout()->add( widget );
    widget->hide();

    m_multiTabBar->appendTab( KGlobal::iconLoader()->loadIcon( icon, KIcon::NoGroup, KIcon::SizeSmall ), id, title );
    QWidget *tab = m_multiTabBar->tab( id );
    tab->setFocusPolicy( QWidget::NoFocus ); //FIXME currently if you tab to these widgets, they respond to no input!

    //we use a SignalMapper to show/hide the corresponding page when tabs are clicked
    connect( tab, SIGNAL( clicked() ), m_mapper, SLOT( map() ) );
    m_mapper->setMapping( tab, id );

    KConfig *config = kapp->config();
    config->setGroup( "PlaylistSideBar" );
    widget->setBaseSize( config->readNumEntry( name, widget->sizeHint().width() ), DefaultHeight );
    {
        //FIXME what if there's no correlation between QPtrList index and multiTabBar id?
        //we need some kind of sensible behavior that doesn't require much code
        int index = m_pages.at();
        m_pages.append( widget );
        //FIXME this sucks plenty!
        if( index == -1 ) { m_pages.last(); m_pages.next(); }
        else m_pages.at( index ); //restore "current"
    }

    if( config->readEntry( "CurrentPane" ) == name )
        showHidePage( id );
}

void PlaylistSideBar::showHidePage( int replaceIndex )
{
    int currentIndex = m_pages.at(); //doesn't set "current"
    QWidget *current = m_pages.current();
    QWidget *replace = m_pages.at( replaceIndex ); //sets "current" to replaceIndex

    if( replace != NULL ) //just in case
    {
        //FIXME make me pretty!

        if( current != NULL && current != replace )
        {
            //we need to close the open page
            current->hide();
            m_multiTabBar->tab( currentIndex )->setState( false );
        }

        bool isHidden = replace->isHidden();

        m_multiTabBar->tab( replaceIndex )->setState( isHidden );
        if( isHidden ) { replace->show(); m_pageHolder->show(); setMaximumWidth( 2000 ); }
        else           { m_pages.last(); m_pages.next(); //quickly sets "current" to NULL
                         replace->hide(); m_pageHolder->hide(); setMaximumWidth( m_multiTabBar->width() ); }
    }
}

QSize PlaylistSideBar::sizeHint() const
{
    //return a sizeHint that will make the splitter space our pages as the user expects
    //note you can't just return width() that wouldn't work unfortunately
    return ( m_pages.current() ) ? m_pages.current()->baseSize() : m_multiTabBar->size();
}

void PlaylistSideBar::resizeEvent( QResizeEvent *e )
{
    if( m_pages.current() )
    {
        m_pages.current()->setBaseSize( e->size().width(), DefaultHeight );
    }
}

QWidget *PlaylistSideBar::page( const QString &widgetName )
{
    for( QPtrList<QWidget>::ConstIterator it = m_pages.constBegin(); *it; ++it )
        if( widgetName == (*it)->name() )
            return *it;

    return 0;
}

void PlaylistSideBar::setFont( const QFont &font )
{
    //Hi! are you here because this function doesn't work?
    //Please consider not changing the fonts for the browsers as they require a
    //readable font, and the user already set a readable font - his default font
    //thanks! - mxcl

    //note - untested because font system was broken when I made this
    //FIXME you'll need to iterate over the children or something else..
/*    for( uint i = 0; i < m_pages.size(); ++i )
        if( m_pages[i] )
            m_pages[i]->setFont( font );*/
}

inline void PlaylistSideBar::close() //SLOT
{
    //this works even if "current" is NULL
    showHidePage( m_pages.at() );
}

inline void PlaylistSideBar::autoClosePages() //SLOT
{
    if( !m_stayButton->isOn() )
    {
        QTimer::singleShot( QApplication::doubleClickInterval(), this, SLOT( close() ) );
    }
}




// CLASS BrowserWin =====================================================================

BrowserWin::BrowserWin( QWidget *parent, const char *name )
   : QWidget( parent, name, Qt::WType_TopLevel | Qt::WPaintUnclipped )
   , m_pActionCollection( new KActionCollection( this ) )
   , m_splitter( new QSplitter( this ) )
   , m_sideBar( new PlaylistSideBar( m_splitter ) )
   , m_playlist( 0 )
   , m_lineEdit( 0 )
{
    setCaption( kapp->makeStdCaption( i18n( "Playlist" ) ) );

    //TODO pass it an engine pointer and it'll connect up various signals
    //this is cool because Qt is cool and not compile check is neccessary!

    /*
    QToolButton *clearButton = new QToolButton( this );
    //KApplication::reverseLayout() ? "clear_left" : "locationbar_erase"
    clearButton->setIconSet( SmallIconSet( "locationbar_erase" ) );
    connect( clearButton, SIGNAL( clicked() ), m_lineEdit, SLOT( clear() ) );
    */

    QBoxLayout *layV = new QVBoxLayout( this );
    layV->addWidget( m_splitter );

    QVBox *box = new QVBox( m_splitter );
    m_lineEdit = new KLineEdit( box );
    m_playlist = new PlaylistWidget( box );
    m_splitter->setResizeMode( m_sideBar, QSplitter::FollowSizeHint );
    m_splitter->setResizeMode( box,       QSplitter::Auto );
    m_sideBar->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );


    {
        ExpandButton *add =
        new ExpandButton( i18n( "&Add Item..." ), this, this, SLOT( slotAddLocation() ) );

        ExpandButton *play =
        new ExpandButton( i18n( "&Play" ),    this, pApp, SLOT( slotPlay() ) );
        new ExpandButton( i18n( "Pause" ),    play, pApp, SLOT( slotPause() ) );
        new ExpandButton( i18n( "Stop" ),     play, pApp, SLOT( slotStop() ) );
        new ExpandButton( i18n( "Next" ),     play, pApp, SLOT( slotNext() ) );
        new ExpandButton( i18n( "Previous" ), play, pApp, SLOT( slotPrev() ) );

        ExpandButton *clear =
        new ExpandButton( i18n( "&Clear" ), this, m_playlist, SLOT( clear() ) );
        new ExpandButton( i18n( "Shuffle" ), clear, m_playlist, SLOT( shuffle() ) );
        new ExpandButton( i18n( "Save Playlist..." ), clear, this, SLOT( savePlaylist() ) );

        m_playlist->m_undoButton->reparent( this, QPoint(), false );
        m_playlist->m_redoButton->reparent( this, QPoint(), false );

        QBoxLayout *layH = new QHBoxLayout( layV );
        layH->addWidget( add );
        layH->addWidget( clear );
        layH->addWidget( m_playlist->m_undoButton );
        layH->addWidget( m_playlist->m_redoButton );
        layH->addWidget( play );
    }

    //</FileBrowser>
        m_sideBar->addPage( new KDevFileSelector( 0, "FileBrowser" ), i18n( "File Browser" ), "hdd_unmount" );
    //</FileBrowser>

    { //<StreamBrowser>
        QVBox   *vb = new QVBox( 0, "StreamBrowser" );
        QWidget *b  = new QPushButton( "&Fetch Stream Information", vb );
        QObject *sb = new StreamBrowser( vb );
        connect( b, SIGNAL( clicked() ), sb, SLOT( slotUpdateStations() ) );
        connect( b, SIGNAL( clicked() ),  b, SLOT( deleteLater() ) );
        m_sideBar->addPage( vb, i18n( "Stream Browser" ), "network" );
    } //</StreamBrowser>


    //load previous playlist
    insertMedia( defaultPlaylistPath() );

    //we intercept keyEvents to the playlist
    m_playlist->installEventFilter( this );
    //we intercept keyEvents to the lineEdit
    m_lineEdit->installEventFilter( this );


    connect( m_playlist,   SIGNAL( aboutToClear() ),
             m_lineEdit,     SLOT( clear() ) );
    //FIXME you need to detect focus out from the sideBar and connect to that..
    connect( m_playlist,   SIGNAL( clicked( QListViewItem * ) ),
             m_sideBar,      SLOT( autoClosePages() ) );
    connect( m_lineEdit,   SIGNAL( textChanged( const QString& ) ),
             m_playlist,     SLOT( slotTextChanged( const QString& ) ) );

    //TODO KStdAction::copy( m_playlist, SLOT( copyAction() ), m_actionCollection );

    QToolTip::add( m_lineEdit, i18n( "Enter filter string" ) );
}


BrowserWin::~BrowserWin()
{
    //TODO save at regular intervals, (use the QWidget built in timer as they have less overhead)

    if( AmarokConfig::savePlaylist() )
        m_playlist->saveM3u( defaultPlaylistPath() );
}



///////// public interface

void BrowserWin::insertMedia( const KURL::List &list, bool b )
{
    if( b ) m_playlist->clear();
    m_playlist->insertMedia( list );
}


bool BrowserWin::isAnotherTrack() const
{
    return m_playlist->isAnotherTrack();
}


void BrowserWin::setColors( const QPalette &pal, const QColor &bgAlt )
{
    m_lineEdit->setPalette( pal );
    m_playlist->setColors( pal, bgAlt ); //due to private inheritance nasty
    setPaletteRecursively( m_sideBar, pal, bgAlt );

    //update()
    //m_playlist->triggerUpdate();

    //TODO perhaps this should be a global member of some singleton (I mean bgAlt not just the filebrowser bgAlt!)
    KDevFileSelector::altBgColor = bgAlt;
}


void BrowserWin::setFont( const QFont &newFont )
{
    m_sideBar ->setFont( newFont );
    m_playlist->setFont( newFont );
}


void BrowserWin::saveConfig()
{
    //FIXME sucks a little to get ptr this way
    //FIXME instead force the widgets to derive from SideBarWidget or something
    // this method is good as it saves having duplicate pointers to the fileBrowser
    KDevFileSelector *fileBrowser = (KDevFileSelector *)m_sideBar->page( "FileBrowser" );
    fileBrowser->writeConfig();
}


inline
QString BrowserWin::defaultPlaylistPath() const
{
    return KGlobal::dirs()->saveLocation( "data", kapp->instanceName() + "/" ) + "current.m3u";
}



//////// private interface

bool BrowserWin::eventFilter( QObject *o, QEvent *e )
{
    //filters events for a few of the widgets we are parent to
    //this makes life easier since we have more useful functions available from here

    switch( e->type() )
    {
    case QEvent::KeyPress:
        #define e static_cast<QKeyEvent*>(e)
        if( o == m_lineEdit )
        {
            //FIXME inefficient to always construct this
            QListViewItemIterator it( m_playlist, QListViewItemIterator::Visible );
            if( 0 == it.current() ) return FALSE;

            switch( e->key() )
            {
            case Key_Down:
                m_playlist->setFocus();
                m_playlist->setCurrentItem( it.current() );
                it.current()->setSelected( true ); //FIXME why doesn't it do this for us?
                return TRUE;

            case Key_Return:
            case Key_Enter:
                m_lineEdit->clear();
                m_playlist->activate( it.current() );
                m_playlist->ensureItemVisible( it.current() );
                return TRUE;

            default:
                return FALSE;
            }
        }

        if( e->key() == Key_Up && m_playlist->currentItem()->itemAbove() == 0 )
        {
            m_playlist->currentItem()->setSelected( false );
            m_lineEdit->setFocus();
            return TRUE;
        }

        if( e->key() == Key_Delete )
        {
            m_playlist->removeSelectedItems();
            return TRUE;
        }

        if( ( e->key() >= Key_0 && e->key() <= Key_Z ) || e->key() == Key_Backspace )
        {
            m_lineEdit->setFocus();
            QApplication::sendEvent( m_lineEdit, e );
            return TRUE;
        }
        #undef e

    default:
        return FALSE;
    }
}


void BrowserWin::savePlaylist() const //SLOT
{
    QWidget *fb = m_sideBar->page( "FileBrowser" );
    QString path = fb ? static_cast<KDevFileSelector *>(fb)->location() : "~";

    path = KFileDialog::getSaveFileName( path, "*.m3u" );

    if( !path.isEmpty() )
    {
        m_playlist->saveM3u( path );
    }
}


void BrowserWin::slotAddLocation() //SLOT
{
    KURLRequesterDlg dlg( QString::null, 0, 0 );
    dlg.setCaption( kapp->makeStdCaption( i18n( "Enter file or URL" ) ) );
    dlg.urlRequester()->setMode( KFile::File | KFile::ExistingOnly );
    dlg.exec();

    insertMedia( dlg.selectedURL() );
}

#include "browserwin.moc"
