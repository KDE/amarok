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

#include <config.h>        //for HAVE_SQLITE check

#include "amarokconfig.h"
#include "amarokmenu.h"    //see toolbar construction
#include "browserwin.h"
#include "browserbar.h"
#include "collectionbrowser.h"
#include "filebrowser.h"
#include "playerapp.h"
#include "playlistwidget.h"
#include "streambrowser.h"
#include "searchbrowser.h"

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
#include <klineedit.h>     //m_lineEdit
#include <klocale.h>
#include <ktoolbar.h>
#include <ktoolbarbutton.h>
#include <kurlrequester.h>    //slotAddLocation()
#include <kurlrequesterdlg.h> //slotAddLocation()
#include <kxmlguifactory.h> //XMLGUI
#include <kxmlguibuilder.h> //XMLGUI


#include <blockanalyzer.h> //FIXME make an action or someting like that


//Routine for setting palette recursively in a widget and all its childen
//TODO make available globally sometime, maybe through an extern in PlayerApp.h
void setPaletteRecursively( QObjectList *list, const QPalette &pal, const QColor& bgAlt )
{
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

inline void setPaletteRecursively( QWidget* widget, const QPalette &pal, const QColor& bgAlt )
{
    setPaletteRecursively( widget->queryList( "QWidget" ), pal, bgAlt );
}


#include <qobjectlist.h>
BrowserWin::BrowserWin( QWidget *parent, const char *name )
   : QWidget( parent, name, Qt::WType_TopLevel | Qt::WNoAutoErase )
   , KXMLGUIClient()
//   , m_pActionCollection( new KActionCollection( this ) )
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;

    setCaption( "amaroK" );

    m_browsers = new BrowserBar( this );

    QVBox *vbox = m_browsers->container(); //m_browsers provides the layout for the playlist

    m_lineEdit = new KLineEdit( vbox );
    m_playlist = new PlaylistWidget( vbox, actionCollection() ); //FIXME some actions are created in here

    KStdAction::open( this, SLOT(slotAddLocation()), actionCollection(), "playlist_add" )->setText( i18n("&Add Media") );
    KStdAction::save( this, SLOT(savePlaylist()), actionCollection(), "playlist_save" )->setText( i18n("&Save Playlist") );

    new amaroK::MenuAction( actionCollection() );
    new amaroK::PlayPauseAction( actionCollection() );

    actionCollection()->addDocCollection( pApp->actionCollection() ); //FIXME this sucks

    //<XMLGUI>
        setXMLFile( "amarokui.rc" );
        KToolBar *toolbar = createGUI(); //NOTE we implement this
    //</XMLGUI>

    kdDebug() << "Created XMLGUI\n";

    QBoxLayout *layV = new QVBoxLayout( this );
    layV->addWidget( m_browsers );
    layV->addWidget( toolbar );


//         toolbar->insertWidget( 102, 32*4, new BlockAnalyzer( toolbar ) );
//         toolbar->getWidget( 102 )->setBackgroundColor( backgroundColor().dark( 120 ) );

//         toolbar->insertButton( QString::null, 101, true, i18n( "Menu" ) );
//         menuItem = toolbar->getButton( 101 );
//         menuItem->setPopup( new amaroK::Menu( this ) );
//         toolbar->alignItemRight( 101 );

    //TEXT ON RIGHT HACK
    //KToolBarButtons have independent settings for their appearance.
    //However these properties are set in modeChange() to follow the parent KToolBar settings
    //passing false to setIconText prevents modeChange() being called for all buttons

    //<FileBrowser>
        m_browsers->addPage( new KDevFileSelector( 0, "FileBrowser" ), i18n( "File Browser" ), "hdd_unmount" );
    //</FileBrowser>

    { //<SearchBrowser>
        m_browsers->addPage( new SearchBrowser( 0, "SearchBrowser" ), i18n( "Search Browser" ), "SearchBrowser" );
    } //</SearchBrowser>

    //<PlaylistBrowser>
        //m_browsers->addPage( m_playlist->browser(), i18n( "Playlist Browser" ), "midi" );
    //</PlaylistBrowser>

#ifdef HAVE_SQLITE
    //<CollectionBrowser>
        m_browsers->addPage( new CollectionBrowser( "CollectionBrowser" ), i18n( "Collection Browser" ), "contents" );
    //</CollectionBrowser>
#endif

    { //<StreamBrowser>
        QVBox   *vb = new QVBox( 0, "StreamBrowser" );
        QWidget *b  = new QPushButton( "&Fetch Stream Information", vb );
        QObject *sb = new StreamBrowser( vb );
        connect( b, SIGNAL( clicked() ), sb, SLOT( slotUpdateStations() ) );
        connect( b, SIGNAL( clicked() ),  b, SLOT( deleteLater() ) );
        m_browsers->addPage( vb, i18n( "Stream Browser" ), "network" );
    } //</StreamBrowser>


    m_playlist->installEventFilter( this ); //we intercept keyEvents
    m_lineEdit->installEventFilter( this ); //we intercept keyEvents


    connect( m_playlist, SIGNAL( aboutToClear() ),
             m_lineEdit,   SLOT( clear() ) );
    //FIXME you need to detect focus out from the sideBar and connect to that..
    connect( m_playlist, SIGNAL( clicked( QListViewItem * ) ),
             m_browsers,   SLOT( autoClosePages() ) );
    connect( m_lineEdit, SIGNAL( textChanged( const QString& ) ),
             m_playlist,   SLOT( slotTextChanged( const QString& ) ) );

    QToolTip::add( m_lineEdit, i18n( "Enter filter string" ) );
    kdDebug() << "END " << k_funcinfo << endl;
}

///////// public interface

KToolBar *BrowserWin::createGUI()
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;

    //if we have been called from the toolBarEdit Dialog
    //we need to clear the toolbar first
    //otherwise we haven't created the toolbar yet
    //NOTE KXMLGUI is not very flexible, we'd happily create the toolbar ourselves, it's less code if we did!

    //TODO create toolbar ourselves

    KToolBar *toolbar = (KToolBar*)child( "playlist_toolbar" );
    if( toolbar ) toolbar->clear();

    KXMLGUIBuilder *builder = new KXMLGUIBuilder( this );
    kdDebug() << "Instantiated builder.\n";

    KXMLGUIFactory *factory = new KXMLGUIFactory( builder, this );
    kdDebug() << "Instantiated factory.\n";

    //build Toolbar, plug actions
    factory->addClient( this );
    kdDebug() << "Added client.\n";

    if( !toolbar ) toolbar = (KToolBar*)child( "playlist_toolbar" );

    KToolBarButton *b;
    toolbar->setIconText( KToolBar::IconTextRight, false );
        if((b = static_cast<KToolBarButton*>(toolbar->child( "toolbutton_playlist_add" )))) b->modeChange();
        if((b = static_cast<KToolBarButton*>(toolbar->child( "toolbutton_playlist_clear" )))) b->modeChange();
        if((b = static_cast<KToolBarButton*>(toolbar->child( "toolbutton_playlist_shuffle" )))) b->modeChange();
        if((b = static_cast<KToolBarButton*>(toolbar->child( "toolbutton_playlist_show" )))) b->modeChange();
    toolbar->setIconText( KToolBar::TextOnly, false );
        if((b = static_cast<KToolBarButton*>(toolbar->child( "toolbutton_amarok_menu" )))) b->modeChange();
    toolbar->setIconText( KToolBar::IconOnly, false );

    kdDebug() << "END " << k_funcinfo << endl;

    delete factory;
    delete builder;
    return toolbar;
    //TODO conserve memory? (delete XML QDom)
}


void BrowserWin::insertMedia( const KURL::List &list, bool clearList, bool directPlay )
{
    if( clearList ) m_playlist->clear(); //FIXME clear currently is not 100% bug free, it might not work as expected

    m_playlist->insertMedia( list, directPlay );
}


void BrowserWin::restoreSessionPlaylist()
{
    insertMedia( m_playlist->defaultPlaylistPath() );
}


bool BrowserWin::isAnotherTrack() const
{
    return m_playlist->isTrackAfter();
}


void BrowserWin::setColors( const QPalette &pal, const QColor &bgAlt )
{
    //these widgets are actually children of splitter now
    //list.append( m_lineEdit );
    //list.append( m_playlist ); //we are friend
    setPaletteRecursively( m_browsers->queryList( "QWidget" ), pal, bgAlt );

    //TODO perhaps this should be a global member of some singleton (I mean bgAlt not just the filebrowser bgAlt!)
    KDevFileSelector::altBgColor = bgAlt;
}


void BrowserWin::setFont( const QFont &newFont )
{
    m_browsers->setFont( newFont );
    m_playlist->setFont( newFont );
}


void BrowserWin::saveConfig()
{
    //FIXME sucks a little to get ptr this way
    //FIXME instead force the widgets to derive from SideBarWidget or something
    // this method is good as it saves having duplicate pointers to the fileBrowser
    KDevFileSelector *fileBrowser = (KDevFileSelector *)m_browsers->page( "FileBrowser" );
    fileBrowser->writeConfig();
}




//////// private interface

bool BrowserWin::eventFilter( QObject *o, QEvent *e )
{
    //filters events for a few of the widgets we are parent to
    //this makes life easier since we have more useful functions available from here

    switch( e->type() )
    {
    case QEvent::KeyPress:

        //there are a few keypresses that we override

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


void BrowserWin::savePlaylist() const //SLOT
{
    QWidget *fb = m_browsers->page( "FileBrowser" );
    QString path = fb ? static_cast<KDevFileSelector *>(fb)->location() : "~";

    path = KFileDialog::getSaveFileName( path, "*.m3u" );

    if( !path.isEmpty() ) //FIXME unecessary check
    {
        m_playlist->saveM3U( path );
    }
}


void BrowserWin::slotAddLocation() //SLOT
{
    KURLRequesterDlg dlg( QString::null, 0, 0 );
    dlg.setCaption( kapp->makeStdCaption( i18n( "Enter File, URL or Directory" ) ) );
    dlg.urlRequester()->setMode( KFile::File | KFile::ExistingOnly );
    dlg.exec();

    insertMedia( dlg.selectedURL() );
}


#include "browserwin.moc"
