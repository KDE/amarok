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
#include "actionclasses.h"    //see toolbar construction
#include "playlistwindow.h"
#include "browserbar.h"
#include "collectionbrowser.h"
#include "enginecontroller.h" //for actions in ctor
#include "filebrowser.h"
#include "app.h"
#include "playlist.h"
#include "streambrowser.h"
#include "searchbrowser.h"
#include "statusbar.h"

#include <qcolor.h>        //setPalettes()
#include <qevent.h>        //eventFilter()
#include <qlayout.h>
#include <qobjectlist.h>   //setPaletteRecursively()
#include <qpalette.h>      //setPalettes()
#include <qtooltip.h>      //QToolTip::add()
#include <qvbox.h>         //contains the playlist

#include <kaction.h>       //m_actionCollection
#include <kapplication.h>  //kapp
#include <kdebug.h>
#include <kglobal.h>
#include <kfiledialog.h>   //savePlaylist()
#include <kiconloader.h>   //ClearFilter button
#include <klineedit.h>     //m_lineEdit
#include <klocale.h>
#include <ktoolbar.h>
#include <ktoolbarbutton.h>   //createGUI()
#include <kurlrequester.h>    //slotAddLocation()
#include <kurlrequesterdlg.h> //slotAddLocation()
#include <kxmlguifactory.h>   //XMLGUI
#include <kxmlguibuilder.h>   //XMLGUI



PlaylistWindow::PlaylistWindow( QWidget *parent, const char *name )
   : QWidget( parent, name, Qt::WType_TopLevel | Qt::WNoAutoErase )
   , KXMLGUIClient()
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;

    setCaption( "amaroK" );

    pApp->m_pActionCollection = actionCollection(); //FIXME sucks


    //<actions>
        KActionCollection* const ac = actionCollection();
        const EngineController* const ec = EngineController::instance();

        KStdAction::configureToolbars( pApp, SLOT( slotConfigToolBars() ), ac );
        KStdAction::keyBindings( pApp, SLOT( slotConfigShortcuts() ), ac );
        KStdAction::keyBindings( pApp, SLOT( slotConfigGlobalShortcuts() ), ac, "options_configure_globals" );
        KStdAction::preferences( pApp, SLOT( slotShowOptions() ), ac );
        KStdAction::quit( pApp, SLOT( quit() ), ac );
        KStdAction::open( this, SLOT(slotAddLocation()), ac, "playlist_add" )->setText( i18n("&Add Media") );
        KStdAction::save( this, SLOT(savePlaylist()), ac, "playlist_save" )->setText( i18n("&Save Playlist") );

        ac->action( "options_configure_globals" )->setText( i18n( "Configure &Global Shortcuts..." ) );

        new KAction( i18n( "Previous Track" ), "player_start", 0, ec, SLOT( previous() ), ac, "prev" );
        new KAction( i18n( "Play" ), "player_play", 0, ec, SLOT( play() ), ac, "play" );
        new KAction( i18n( "Stop" ), "player_stop", 0, ec, SLOT( stop() ), ac, "stop" );
        new KAction( i18n( "Pause" ), "player_pause", 0, ec, SLOT( pause() ), ac, "pause" );
        new KAction( i18n( "Next Track" ), "player_end", 0, ec, SLOT( next() ), ac, "next" );

        new amaroK::MenuAction( ac );
        new amaroK::PlayPauseAction( ac );
        new amaroK::AnalyzerAction( ac );
        new amaroK::RepeatTrackAction( ac );
        new amaroK::RepeatPlaylistAction( ac );
        new amaroK::RandomAction( ac );

        ac->action( "stop" )->setEnabled( false );
        ac->action( "pause" )->setEnabled( false );
        ac->action( "prev" )->setEnabled( false );
        ac->action( "next" )->setEnabled( false );
    //</actions>


    m_browsers = new BrowserBar( this );


    { //<Search LineEdit>
        QHBox *hbox; QToolButton *button;

        hbox       = new QHBox( m_browsers->container() );
        button     = new QToolButton( hbox );
        m_lineEdit = new KLineEdit( hbox );

        hbox->setMargin( 4 );
        button->setIconSet( SmallIconSet( "locationbar_erase.png" ) );
        m_lineEdit->setFrame( QFrame::Sunken );
        m_lineEdit->installEventFilter( this ); //we intercept keyEvents

        connect( button, SIGNAL(clicked()), m_lineEdit, SLOT(clear()) );

        QToolTip::add( button, i18n( "Clear filter" ) );
        QToolTip::add( m_lineEdit, i18n( "Enter space-separated terms to filter playlist" ) );
    } //</Search LineEdit>


    KStatusBar *statusbar;

    m_toolbar  = new KToolBar( this, "playlist_toolbar" );
    statusbar  = new amaroK::StatusBar( this );
    m_playlist = new Playlist( m_browsers->container(), ac );

    QToolTip::add( m_lineEdit, i18n( "Enter filter string" ) );

    QBoxLayout *layV = new QVBoxLayout( this );
    layV->addWidget( m_browsers, 2 );
    layV->addWidget( m_toolbar );
    layV->addWidget( statusbar );

    m_playlist->setMargin( 2 );
    m_playlist->installEventFilter( this ); //we intercept keyEvents


    //<XMLGUI>
        setXMLFile( "amarokui.rc" );
        createGUI(); //NOTE we implement this
    //</XMLGUI>


    //<FileBrowser>
        QWidget *fb = new FileBrowser( "FileBrowser" );
        m_browsers->addBrowser( fb, i18n( "Files" ), "hdd_unmount" );
        connect( fb, SIGNAL(activated( const KURL& )), m_playlist, SLOT(insertMedia( const KURL& )) );
    //</FileBrowser>

    //<SearchBrowser>
        m_browsers->addBrowser( new SearchBrowser( "SearchBrowser" ), i18n( "Search" ), "find" );
    //</SearchBrowser>

    //<PlaylistBrowser>
    #ifdef PLAYLIST_BROWSER
        m_browsers->addBrowser( m_playlist->browser(), i18n( "Playlist" ), "midi" );
    #endif
    //</PlaylistBrowser>

    //<CollectionBrowser>
        m_browsers->addBrowser( new CollectionBrowser( "CollectionBrowser" ), i18n( "Collection" ), "contents" );
    //</CollectionBrowser>

    { //<StreamBrowser>
        QVBox   *vb = new QVBox( 0, "StreamBrowser" );
        QWidget *b  = new QPushButton( "&Fetch Stream Information", vb );
        QWidget *sb = new StreamBrowser( vb );
        vb->setSpacing( 3 );
        vb->setMargin( 5 );
        vb->setFocusProxy( sb );
        connect( b, SIGNAL( clicked() ), sb, SLOT( slotUpdateStations() ) );
        connect( b, SIGNAL( clicked() ),  b, SLOT( deleteLater() ) );
        m_browsers->addBrowser( vb, i18n( "Streams" ), "network" );
    } //</StreamBrowser>


    connect( m_playlist, SIGNAL(itemCountChanged( int )),
             statusbar,    SLOT(slotItemCountChanged( int )) );
    connect( m_playlist, SIGNAL(aboutToClear()),
             m_lineEdit,   SLOT(clear()) );
    connect( m_lineEdit, SIGNAL(textChanged( const QString& )),
             m_playlist,   SLOT(slotTextChanged( const QString& )) );


    kdDebug() << "END " << k_funcinfo << endl;
}

PlaylistWindow::~PlaylistWindow()
{
    AmarokConfig::setBrowserWinPos( pos() );  //TODO no need to be XT'd
    AmarokConfig::setBrowserWinSize( size() ); //TODO no need to be XT'd
}


///////// public interface

void PlaylistWindow::createGUI()
{
    setUpdatesEnabled( false );

    m_toolbar->clear(); //is necessary

    KXMLGUIBuilder builder( this );
    KXMLGUIFactory factory( &builder, this );

    //build Toolbar, plug actions
    factory.addClient( this );

    //TEXT ON RIGHT HACK
    //KToolBarButtons have independent settings for their appearance.
    //KToolBarButton::modeChange() causes that button to set its mode to that of its parent KToolBar
    //KToolBar::setIconText() calls modeChange() for children, unless 2nd param is false

    QStringList list;
    list << "toolbutton_playlist_add"
//         << "toolbutton_playlist_clear"
//         << "toolbutton_playlist_shuffle"
         << "toolbutton_playlist_show"
         << "toolbutton_amarok_menu";

    m_toolbar->setIconText( KToolBar::IconTextRight, false ); //we want some buttons to have text on right

    const QStringList::ConstIterator end  = list.constEnd();
    const QStringList::ConstIterator last = list.fromLast();
    for( QStringList::ConstIterator it = list.constBegin(); it != end; ++it )
    {
        if( it == last ) m_toolbar->setIconText( KToolBar::TextOnly, false );

        KToolBarButton* const button = (KToolBarButton*)m_toolbar->child( (*it).latin1() );
        if( button ) button->modeChange();
    }

    m_toolbar->setIconText( KToolBar::IconOnly, false ); //default appearance

    conserveMemory();

    setUpdatesEnabled( true );
}


void PlaylistWindow::insertMedia( const KURL::List &list, bool clearList, bool directPlay )
{
    if( clearList ) m_playlist->clear(); //FIXME clear currently is not 100% bug free, it might not work as expected

    m_playlist->insertMedia( list, directPlay );
}


void PlaylistWindow::restoreSessionPlaylist()
{
    insertMedia( m_playlist->defaultPlaylistPath() );
}


bool PlaylistWindow::isAnotherTrack() const
{
    return m_playlist->isTrackAfter();
}


void PlaylistWindow::setColors( const QPalette &pal, const QColor &bgAlt )
{
    //TODO optimise bearing in mind ownPalette property and unsetPalette()
    //TODO this doesn't work well with the select your own colours options. SIGH. Is it worth the trouble?

    //this updates all children's palettes recursively (thanks Qt!)
    m_browsers->setPalette( pal );

    const bool schemeAmarok = !AmarokConfig::schemeKDE();
    QObjectList* const list = m_browsers->queryList( "QWidget" );

    //now we need to search for KListViews so we can set the alternative colours
    //also amaroK's colour scheme has a few issues
    for( QObject *obj = list->first(); obj; obj = list->next() )
    {
        #define widget static_cast<QWidget*>(obj)

        if( obj->inherits("KListView") )
        {
            KListView *lv = dynamic_cast<KListView *>(obj); //slow, but safe
            if( lv ) lv->setAlternateBackground( bgAlt );
        }
        else if( schemeAmarok )
        {
            if( obj->inherits("QLabel") || obj->inherits("QToolBar") )
            {
                widget->setPaletteForegroundColor( Qt::white );
            }
            else if( obj->inherits("QMenuBar") || obj->parent()->isA("QSplitter") )
            {
                //I don't understand the QSplitter one, I got it to work by trial and error

                widget->setPalette( QApplication::palette() );
            }
        }

        #undef widget
    }

    delete list; //heap allocated! Naughty Qt! Should use a QValueList<QObject*> no? (ValueLists are shared)

    //TODO perhaps this should be a global member of some singleton (I mean bgAlt not just the filebrowser bgAlt!)
    FileBrowser::altBgColor = bgAlt;
}


void PlaylistWindow::setFont( const QFont &newFont )
{
    m_browsers->setFont( newFont );
    m_playlist->setFont( newFont );
}


bool PlaylistWindow::eventFilter( QObject *o, QEvent *e )
{
    //here we filter some events for the Playlist Search LineEdit and the Playlist
    //this makes life easier since we have more useful functions available from this class

    switch( e->type() )
    {
    case QEvent::FocusIn:
        m_browsers->autoCloseBrowsers();
        break;

    case QEvent::KeyPress:

        //there are a few keypresses that we intercept

        #define e static_cast<QKeyEvent*>(e)

        if( e->key() == Key_F2 )
        {
            QListViewItem* item = reinterpret_cast<QListViewItem*>(m_playlist->currentTrack());

            // we set it to currentItem if currentTrack() is no good, currentItem is ALWAYS visible.
            if( !( item && item->isVisible() ) ) item = m_playlist->currentItem();

            // intercept F2 for inline tag renaming
            // NOTE: tab will move to the next tag
            // NOTE: if item is still null don't select first item in playlist, user wouldn't want that. It's silly.
            // TODO: berkus has solved the "inability to cancel" issue with KListView, but it's not in kdelibs yet..

            // item may still be null, but this is safe
            // NOTE: column 0 cannot be edited currently
            m_playlist->rename( item, 1 ); //TODO what if this column is hidden?

            return TRUE;
        }

        if( o == m_lineEdit ) //the search lineedit
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

            case Key_PageDown:
            case Key_PageUp:
                QApplication::sendEvent( m_playlist, e );
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

        //following are for m_playlist only
        //we don't handle these in the playlist because often we manipulate the lineEdit too

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
        break;

    default:
        break;
    }

    return FALSE;
}


void PlaylistWindow::savePlaylist() const //SLOT
{
    QWidget *fb = m_browsers->browser( "FileBrowser" );
    QString path = fb ? static_cast<FileBrowser *>(fb)->location() : "~";

    path = KFileDialog::getSaveFileName( path, "*.m3u" );

    if( !path.isEmpty() ) //FIXME unecessary check?
    {
        m_playlist->saveM3U( path );
    }
}


void PlaylistWindow::slotAddLocation() //SLOT
{
    KURLRequesterDlg dlg( QString::null, 0, 0 );
    dlg.setCaption( kapp->makeStdCaption( i18n( "Enter File, URL or Directory" ) ) );
    dlg.urlRequester()->setMode( KFile::File | KFile::ExistingOnly );
    dlg.exec();

    insertMedia( dlg.selectedURL() );
}

#include "playlistwindow.moc"
