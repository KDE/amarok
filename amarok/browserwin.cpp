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
#include "amarokmenu.h" //see toolbar construction
#include "browserwin.h"
#include "browserbar.h"
#include "filebrowser.h"
#include "playerapp.h"
#include "playlistwidget.h"
#include "streambrowser.h"

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



BrowserWin::BrowserWin( QWidget *parent, const char *name )
   : QWidget( parent, name, Qt::WType_TopLevel | Qt::WNoAutoErase )
   , m_pActionCollection( new KActionCollection( this ) )
{
    setCaption( "amaroK" );


    KToolBar* toolbar = new KToolBar( this );
    toolbar->setEnableContextMenu( true );

    m_browsers = new BrowserBar( this );

    QBoxLayout *layV = new QVBoxLayout( this );
    layV->addWidget( m_browsers );
    layV->addWidget( toolbar );

    QVBox *vbox = m_browsers->container(); //m_browsers provides the layout for the playlist

    m_lineEdit = new KLineEdit( vbox );
    m_playlist = new PlaylistWidget( vbox, m_pActionCollection );

    {//<ToolBar>

        KToolBarButton *addItem, *menuItem;
        KActionCollection *ac = actionCollection();

        toolbar->insertButton( "fileopen", 100, true, i18n( "Add Item" ) );
        addItem = toolbar->getButton( 100 );
        connect( addItem, SIGNAL( clicked() ), this, SLOT( slotAddLocation() ) );

        KAction *savePlaylist = KStdAction::save( this, SLOT( savePlaylist() ), ac, "save_playlist" );
        savePlaylist->setText( i18n("&Save Playlist") );
        savePlaylist->plug( toolbar );
        ac->action( "shuffle_playlist" )->plug( toolbar );
        m_playlist->m_clearButton->plug( toolbar );
        ac->action( "show_current_track" )->plug( toolbar );

        toolbar->insertLineSeparator();

        toolbar->insertWidget( 102, 32*4, new BlockAnalyzer( toolbar ) );
        toolbar->getWidget( 102 )->setBackgroundColor( backgroundColor().dark( 120 ) );

        toolbar->insertLineSeparator();

        m_playlist->m_undoButton->plug( toolbar );
        m_playlist->m_redoButton->plug( toolbar );

        toolbar->insertLineSeparator();

        ac = pApp->actionCollection();
        ac->action( "prev"  )->plug( toolbar );
        ac->action( "play"  )->plug( toolbar );
        ac->action( "pause" )->plug( toolbar );
        ac->action( "stop"  )->plug( toolbar );
        ac->action( "next"  )->plug( toolbar );

        toolbar->insertButton( QString::null, 101, true, i18n( "Menu" ) );
        menuItem = toolbar->getButton( 101 );
        menuItem->setPopup( new AmarokMenu( this ) );
        toolbar->alignItemRight( 101 );

        //TEXT ON RIGHT HACK
        //KToolBarButtons have independent settings for their appearance.
        //However these properties are set in modeChange() to follow the parent KToolBar settings
        //passing false to setIconText prevents modeChange() being called for all buttons
        toolbar->setIconText( KToolBar::IconTextRight, false );
            addItem->modeChange();
            toolbar->getButton( toolbar->idAt( 2 ) )->modeChange();
            toolbar->getButton( toolbar->idAt( 3 ) )->modeChange();
            toolbar->getButton( toolbar->idAt( 4 ) )->modeChange();
        toolbar->setIconText( KToolBar::TextOnly, false );
            menuItem->modeChange();
        toolbar->setIconText( KToolBar::IconOnly, false );

    }//</ToolBar>


    //</FileBrowser>
        m_browsers->addPage( new KDevFileSelector( 0, "FileBrowser" ), i18n( "File Browser" ), "hdd_unmount" );
    //</FileBrowser>

    //</PlaylistBrowser>
        //m_browsers->addPage( m_playlist->browser(), i18n( "Playlist Browser" ), "midi" );
    //</PlaylistBrowser>

    { //<StreamBrowser>
        QVBox   *vb = new QVBox( 0, "StreamBrowser" );
        QWidget *b  = new QPushButton( "&Fetch Stream Information", vb );
        QObject *sb = new StreamBrowser( vb );
        connect( b, SIGNAL( clicked() ), sb, SLOT( slotUpdateStations() ) );
        connect( b, SIGNAL( clicked() ),  b, SLOT( deleteLater() ) );
        m_browsers->addPage( vb, i18n( "Stream Browser" ), "network" );
    } //</StreamBrowser>


    //we intercept keyEvents to the playlist
    m_playlist->installEventFilter( this );
    //we intercept keyEvents to the lineEdit
    m_lineEdit->installEventFilter( this );


    connect( m_playlist, SIGNAL( aboutToClear() ),
             m_lineEdit,   SLOT( clear() ) );
    //FIXME you need to detect focus out from the sideBar and connect to that..
    connect( m_playlist, SIGNAL( clicked( QListViewItem * ) ),
             m_browsers,   SLOT( autoClosePages() ) );
    connect( m_lineEdit, SIGNAL( textChanged( const QString& ) ),
             m_playlist,   SLOT( slotTextChanged( const QString& ) ) );

    QToolTip::add( m_lineEdit, i18n( "Enter filter string" ) );
}


BrowserWin::~BrowserWin()
{
    //TODO save at regular intervals, (use the QWidget built in timer as they have less overhead)

    if( AmarokConfig::savePlaylist() )
        m_playlist->saveM3U( m_playlist->defaultPlaylistPath() );
}



///////// public interface

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
