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

#include "browserwin.h"
#include "expandbutton.h"
#include "filebrowser.h"
#include "playerapp.h"
#include "playlistwidget.h"
#include "streambrowser.h"

#include "amarokconfig.h"

#include <qcolor.h>   //setPalettes()
#include <qpalette.h> //setPalettes()
#include <qfile.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qsplitter.h>
#include <qstring.h>
#include <qtooltip.h>
#include <qvbox.h>
#include <qwidget.h>
#include <qsignalmapper.h> //PlaylistSideBar
#include <qobjectlist.h>   //setPaletteRecursively()
#include <qpainter.h>      //PlaylistSideBar::TinyButton
#include <qstyle.h>        //PlaylistSideBar::PlaylistSideBar

#include <kaction.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <klocale.h>
#include <klineedit.h>
#include <klistview.h>
#include <kmultitabbar.h>
#include <kstandarddirs.h>
#include <kurl.h>


static void setPaletteRecursively( QWidget*, const QPalette&, const QColor& );




//<mxcl>
//I avoided using KDockWidget as it is horrendously large and we only wanted a subset
//of its functionality. But this turned out to be more code than I thought.
//Maybe this was a silly decision.. change it if you like.

/**
 * A mini-button usually placed in the dockpanel.
 * @internal
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

    for( QPtrList<QWidget>::ConstIterator it = m_pages.constBegin(); *it; ++it )
        config->writeEntry( (*it)->name(), (*it)->baseSize().width() );
}

void PlaylistSideBar::addPage( QWidget *widget, const QString& icon, bool show )
{
    //hi, this function is ugly - blame the monstrosity that is KMultiTabBar

    widget->reparent( m_pageHolder, 0, QPoint() ); //FIXME
    m_pageHolder->layout()->add( widget );

    //we need to get next available id this way
    //currently it's the only way to do it coz KMultiTabBar sux0rs
    int id = m_multiTabBar->tabs()->count();
    //we exlusively use widget name to force adoption of sensible, unique widget names for the
    //purposes of this class
    QString name( widget->name() );

    m_multiTabBar->appendTab( KGlobal::iconLoader()->loadIcon( icon, KIcon::NoGroup, KIcon::SizeSmall ), id, name );
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
        m_pages.at( index ); //restore "current"
    }

    widget->hide();
    if( show ) showHidePage( id );
}

void PlaylistSideBar::showHidePage( int replaceIndex )
{
    setUpdatesEnabled( false );

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

    setUpdatesEnabled( true );
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

void PlaylistSideBar::setPageFont( const QFont &font )
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
    if( !m_stayButton->isOn() ) close();
}



// CLASS BrowserWin =====================================================================

BrowserWin::BrowserWin( QWidget *parent, const char *name )
    : QWidget( parent, name, Qt::WType_TopLevel | Qt::WPaintUnclipped )
    , m_pActionCollection( new KActionCollection( this ) )
    , m_pSplitter( new QSplitter( this ) )
    , m_pSideBar( new PlaylistSideBar( m_pSplitter ) )
{
    setCaption( kapp->makeStdCaption( i18n( "Playlist" ) ) );
    setAcceptDrops( true );
    m_pSplitter->setResizeMode( m_pSideBar, QSplitter::FollowSizeHint );
    m_pSideBar ->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );

    initChildren();

    KStdAction::undo( m_pPlaylistWidget, SLOT( doUndo() ), m_pActionCollection );
    KStdAction::redo( m_pPlaylistWidget, SLOT( doRedo() ), m_pActionCollection );
    KStdAction::copy( m_pPlaylistWidget, SLOT( copyAction() ), m_pActionCollection );

    connect( m_pPlaylistWidget, SIGNAL( sigUndoState( bool ) ),
             m_pButtonUndo, SLOT( setEnabled( bool ) ) );
    connect( m_pPlaylistWidget, SIGNAL( sigRedoState( bool ) ),
             m_pButtonRedo, SLOT( setEnabled( bool ) ) );
    connect( m_pPlaylistWidget, SIGNAL( cleared() ),
             m_pPlaylistLineEdit, SLOT( clear() ) );
    connect( m_pPlaylistLineEdit, SIGNAL( clicked() ),
             m_pSideBar, SLOT( autoClosePages() ) );
    connect( m_pPlaylistWidget, SIGNAL( clicked( QListViewItem * ) ),
             m_pSideBar, SLOT( autoClosePages() ) );


    connect( m_pButtonClear, SIGNAL( clicked() ),
             m_pPlaylistWidget, SLOT( clear() ) );

    //FIXME <mxcl> MAKE_IT_CLEAN: kaction-ify
    connect( m_pButtonShuffle,  SIGNAL( clicked() ),
             m_pPlaylistWidget, SLOT( shuffle() ) );

    //moved from playerapp
    connect( m_pButtonAdd, SIGNAL( clicked() ),
             this, SLOT( slotAddLocation() ) );

    connect( m_pButtonSave, SIGNAL( clicked() ),
             this, SLOT( savePlaylist() ) );

    connect( m_pButtonUndo, SIGNAL( clicked() ),
             m_pPlaylistWidget, SLOT( doUndo() ) );

    connect( m_pButtonRedo, SIGNAL( clicked() ),
             m_pPlaylistWidget, SLOT( doRedo() ) );
}


BrowserWin::~BrowserWin()
{
    //FIXME sucks a little to get ptr this way
    //FIXME instead force the widgets to derive from SideBarWidget or something
    // this method is good as it saves duplicate pointer to the fileBrowser
    KDevFileSelector *fileBrowser = (KDevFileSelector *)m_pSideBar->page( "FileBrowser" );

    //NOTE this doesn't seem to save anything yet..
    if( fileBrowser != NULL ) fileBrowser->writeConfig( kapp->sessionConfig(), "filebrowser" );
}

/////////////////////////////////////////////////////////////////////////////////////
// INIT
/////////////////////////////////////////////////////////////////////////////////////

void BrowserWin::initChildren()
{
    //<Buttons>
    m_pButtonAdd     = new ExpandButton( i18n( "Add Item" ), this );

    m_pButtonClear   = new ExpandButton( i18n( "Clear" ), this );
    m_pButtonShuffle = new ExpandButton( i18n( "Shuffle" ), m_pButtonClear );
    m_pButtonSave    = new ExpandButton( i18n( "Save Playlist" ), m_pButtonClear );

    m_pButtonUndo    = new ExpandButton( i18n( "Undo" ), this );
    m_pButtonRedo    = new ExpandButton( i18n( "Redo" ), this );
    m_pButtonUndo      ->  setEnabled  ( false );
    m_pButtonRedo      ->  setEnabled  ( false );

    m_pButtonPlay    = new ExpandButton( i18n( "Play" ), this );
    m_pButtonPause   = new ExpandButton( i18n( "Pause" ), m_pButtonPlay );
    m_pButtonStop    = new ExpandButton( i18n( "Stop" ), m_pButtonPlay );
    m_pButtonNext    = new ExpandButton( i18n( "Next" ), m_pButtonPlay );
    m_pButtonPrev    = new ExpandButton( i18n( "Previous" ), m_pButtonPlay );
    //</Buttons>

    { //</FileBrowser>
        KDevFileSelector *w = new KDevFileSelector( m_pSideBar, "FileBrowser" );
        w->readConfig( kapp->sessionConfig(), "filebrowser" );
        m_pSideBar->addPage( w, "hdd_unmount", true );
    } //</FileBrowser>

    { //<StreamBrowser>
        QVBox   *vb = new QVBox( m_pSideBar, "StreamBrowser" );
        QWidget *b  = new QPushButton( "&Fetch Stream Information", vb );
        QObject *sb = new StreamBrowser( vb, "KDERadioStation" );
        connect( b, SIGNAL( clicked() ), sb, SLOT( slotUpdateStations() ) );
        connect( b, SIGNAL( clicked() ),  b, SLOT( hide() ) );
        m_pSideBar->addPage( vb, "network" );
    } //</StreamBrowser>

    { //<Playlist>
        QVBox *vb = new QVBox( m_pSplitter );
        m_pPlaylistLineEdit = new KLineEdit( vb );
        m_pPlaylistWidget   = new PlaylistWidget( vb );
        connect( m_pPlaylistLineEdit, SIGNAL( textChanged( const QString& ) ),
                m_pPlaylistWidget, SLOT( slotTextChanged( const QString& ) ) );
        connect( m_pPlaylistLineEdit, SIGNAL( returnPressed() ),
                m_pPlaylistWidget, SLOT( slotReturnPressed() ) );
        m_pSplitter->setResizeMode( vb, QSplitter::Auto );
        QToolTip::add( m_pPlaylistLineEdit, i18n( "Enter filter string" ) );
    } //</Playlist>

    //<Layout>
    QBoxLayout *layV = new QVBoxLayout( this );
    layV->addWidget( m_pSplitter );

    QBoxLayout *layH = new QHBoxLayout( layV );
    layH->addWidget( m_pButtonAdd );
    layH->addWidget( m_pButtonClear );
    layH->addWidget( m_pButtonUndo );
    layH->addWidget( m_pButtonRedo );
    layH->addWidget( m_pButtonPlay );
    //</Layout>
}

/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
/////////////////////////////////////////////////////////////////////////////////////

void BrowserWin::slotUpdateFonts()
{
    QFont font;

    if ( !AmarokConfig::useCustomFonts() )    //workaround for reversed config entry
        font = AmarokConfig::browserWindowFont();

    m_pSideBar->setPageFont( font );
    m_pPlaylistWidget->setFont( font );
}


#include <kfiledialog.h>
#include <kurlrequester.h>
#include <kurlrequesterdlg.h>

void BrowserWin::savePlaylist()
{
/*    QString path = KFileDialog::getSaveFileName( m_pBrowserWidget->m_pDirLister->url().path(), "*.m3u" );

    if ( !path.isEmpty() )
    {
        if ( path.right( 4 ) != ".m3u" ) // <berkus> FIXME: 3.2 KFileDialog has a [x] Append file extension automagically, so we should obey the user choice
            path += ".m3u";

        m_pPlaylistWidget->saveM3u( path );
    }*/
}


void BrowserWin::slotAddLocation()
{
    KURLRequesterDlg dlg( QString::null, 0, 0 );
    dlg.setCaption( kapp->makeStdCaption( i18n( "Enter file or URL" ) ) );
    dlg.urlRequester()->setMode( KFile::File | KFile::ExistingOnly );
    dlg.exec();

    m_pPlaylistWidget->insertMedia( dlg.selectedURL() );
}


void BrowserWin::setPalettes( const QPalette &pal, const QColor &bgAlt )
{
    setPaletteRecursively( m_pPlaylistWidget,   pal, bgAlt );
    setPaletteRecursively( m_pPlaylistLineEdit, pal, bgAlt );
    setPaletteRecursively( m_pSideBar,          pal, bgAlt );

    update();
    m_pPlaylistWidget->triggerUpdate();
}


//Routine for setting palette recursively in a widget and all its childen
static void setPaletteRecursively( QWidget* widget, const QPalette &pal, const QColor& bgAlt )
{
    QObjectList *list = widget->queryList( "QWidget" );
    list->append( widget );

    for( QObject *obj = list->first(); obj; obj = list->next() )
    {
        static_cast<QWidget*>(obj)->setPalette( pal );
        if( obj->inherits( "QLineEdit" ) )
        {
            QLineEdit *le = dynamic_cast<QLineEdit *>(obj); //slow, but safe
            if( le ) le->setPaletteForegroundColor( Qt::white ); //FIXME don't be hard set!
        }
        else if( obj->inherits( "KListView" ) )
        {
            KListView *lv = dynamic_cast<KListView *>(obj); //slow, but safe
            if( lv ) lv->setAlternateBackground( bgAlt );
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
/////////////////////////////////////////////////////////////////////////////////////

void BrowserWin::closeEvent( QCloseEvent *e )
{
    e->accept();
    emit signalHide();
}


void BrowserWin::keyPressEvent( QKeyEvent *e )
{
  //if the keypress is given to this widget then nothing is in focus
  //if the keypress was passed here from a childWidget that couldn't handle it then
  //we should note what is in focus and not send it back!

  //FIXME WARNING! there is a substantial risk of infinite looping here if the event is ignored by child event
  //               handlers it will be passed back to this function!

  //FIXME, you managed an infinite loop here. Damn (using filebrowserlineedit)

  kdDebug() << "BrowserWin::keyPressEvent()\n";

  switch( e->key() )
  {
  case Qt::Key_Up:
  case Qt::Key_Down:
  case Qt::Key_Left:
  case Qt::Key_Right:
  case Qt::Key_Prior:
  case Qt::Key_Next:
//  case Qt::Key_Return:
//  case Qt::Key_Enter:
  case Qt::Key_Delete:
     if( !m_pPlaylistWidget->hasFocus() )
     {
        //if hasFocus() then this event came from there, and we don't want to risk an infinite loop!
        m_pPlaylistWidget->setFocus();
        QApplication::sendEvent( m_pPlaylistWidget, e );
     }
     break;
/*
  //removed as risky, although useful
  default:
     if( !m_pPlaylistLineEdit->hasFocus() )
     {
        //if hasFocus() then this event came from there (99% sure)
        m_pPlaylistLineEdit->setFocus();
        QApplication::sendEvent( m_pPlaylistLineEdit, e );
     }
*/
  }

  e->accept(); //consume the event, ALERT! keypresses won't propagate to parent (good thing)
}


#include "browserwin.moc"
