/* This file is part of the KDE project
   Copyright (C) 2004 Mark Kretschmann <markey@web.de>
   Copyright (C) 2003 Alexander Dymo <cloudtemple@mksat.net>
   Copyright (C) 2003 Roberto Raggi <roberto@kdevelop.org>
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#include "amarokconfig.h"
#include "enginecontroller.h"
#include "filebrowser.h"
#include "k3bexporter.h"
#include "kbookmarkhandler.h"
#include "playlist.h"
#include "playlistloader.h"

#include <qhbox.h>
#include <qdir.h>
#include <qlabel.h>
#include <qtimer.h>
#include <qtooltip.h>

#include <kaction.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <klistview.h>   //slotViewChanged()
#include <klineedit.h>
#include <klocale.h>
#include <kpopupmenu.h>
#include <ktoolbarbutton.h> //ctor
#include <kurlcombobox.h>
#include <kurlcompletion.h>


namespace amaroK { extern KConfig *config( const QString& ); }


class MyDirLister : public KDirLister
{
    public:
        MyDirLister( bool delayedMimeTypes ) : KDirLister( delayedMimeTypes ) { }
    protected:
        virtual bool MyDirLister::matchesMimeFilter( const KFileItem *item ) const {
            return
                item->isDir() ||
                EngineController::canDecode( item->url().path() ) ||
                PlaylistLoader::isPlaylist( item->url() );
        }
};

class MyDirOperator : public KDirOperator
{
    public:
        MyDirOperator( const KURL &url, QWidget *parent ) : KDirOperator( url, parent ) {
            setDirLister( new MyDirLister( true ) );
        }
};


QColor FileBrowser::altBgColor; //FIXME should be redundant eventually!


//BEGIN Constructor/destructor

FileBrowser::FileBrowser( const char * name )
    : QVBox( 0, name )
    , m_timer( new QTimer( this ) )
{
    setSpacing( 4 );
    setMargin( 5 );

    KConfig* const config = amaroK::config( "Filebrowser" );

    m_toolbar = new FileBrowser::ToolBar( this );
    m_toolbar->setMovingEnabled(false);
    m_toolbar->setFlat(true);
    m_toolbar->setIconText( KToolBar::IconOnly );
    m_toolbar->setIconSize( 16 );
    m_toolbar->setEnableContextMenu( false );

    QString currentLocation = config->readEntry( "Location" );
    if ( !QDir( currentLocation ).exists() )
        currentLocation = QDir::homeDirPath();

    { //Search LineEdit
        QHBox *hbox; KToolBarButton *button;

        hbox         = new QHBox( this );
        button       = new KToolBarButton( "locationbar_erase", 0, hbox );
        m_filterEdit = new KLineEdit( hbox, "filter_edit" );
        m_filterEdit->installEventFilter( this );

        connect( button, SIGNAL(clicked()), this, SLOT(clearFilter()) );

        QToolTip::add( button, i18n( "Clear filter" ) );
        QToolTip::add( m_filterEdit, i18n( "Space-separated terms will be used to filter the directory-listing" ) );
    }

    //The main widget with file listings and that
    m_dir = new MyDirOperator( KURL(currentLocation), this );
    connect( m_dir, SIGNAL(urlEntered( const KURL& )), SLOT(dirUrlEntered( const KURL& )) );

    //folder selection combo box
    m_cmbPath = new KURLComboBox( KURLComboBox::Directories, true, this, "path combo" );
    m_cmbPath->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ));
    m_cmbPath->setCompletionObject( new KURLCompletion( KURLCompletion::DirCompletion ) );
    m_cmbPath->setMaxItems( 9 );
    m_cmbPath->setURLs( config->readListEntry( "Dir History" ) );
    m_cmbPath->lineEdit()->setText( currentLocation );
    setFocusProxy( m_cmbPath ); //so the dirOperator is focussed when we get focus events

    {
        //set the lineEdit to initial state
        QEvent e( QEvent::FocusOut );
        eventFilter( m_filterEdit, &e );
    }

    //insert our own actions at front of context menu
    QPopupMenu* const menu = ((KActionMenu*)actionCollection()->action("popupMenu"))->popupMenu();
    menu->clear();
    menu->insertItem( i18n( "&Append to Playlist" ), this, SLOT(addToPlaylist()), 1 );
    menu->insertItem( i18n( "&Make Playlist" ), this, SLOT(makePlaylist()), 0 );
    menu->insertSeparator();
    //TODO this has no place in the context menu, make it a toolbar button instead

    enum { BURN_DATACD = 100, BURN_AUDIOCD };
    menu->insertItem( i18n("Burn to CD as data"), this, SLOT( burnDataCd() ), 0, BURN_DATACD );
    menu->setItemEnabled( BURN_DATACD, K3bExporter::isAvailable() );
    menu->insertItem( i18n("Burn to CD as audio"), this, SLOT( burnAudioCd() ), 0, BURN_AUDIOCD );
    menu->setItemEnabled( BURN_AUDIOCD, K3bExporter::isAvailable() );

    menu->insertSeparator();
    menu->insertItem( i18n( "&Select All Files" ), this, SLOT(selectAllFiles()) );
    menu->insertSeparator();
    actionCollection()->action( "delete" )->plug( menu );
    menu->insertSeparator();
    actionCollection()->action( "properties" )->plug( menu );

    m_dir->setEnableDirHighlighting( true );
    m_dir->setMode( KFile::Mode((int)KFile::Files | (int)KFile::Directory) ); //allow selection of multiple files + dirs
    m_dir->setOnlyDoubleClickSelectsFiles( true ); //amaroK type settings
    m_dir->readConfig( config );
    m_dir->setView( KFile::Default ); //will set userconfigured view, will load URL
    setStretchFactor( m_dir, 2 );

    {
        KActionMenu *a;

        a = (KActionMenu*)actionCollection()->action( "sorting menu" );
        a->setIcon( "configure" );
        a->setDelayed( false ); //TODO should be done by KDirOperator

        actionCollection()->action( "delete" )->setShortcut( KShortcut( SHIFT + Key_Delete ) );

        a = new KActionMenu( i18n("Bookmarks"), "bookmark", actionCollection(), "bookmarks" );
        a->setDelayed( false );
        KBookmarkHandler *bookmarkHandler = new KBookmarkHandler( this, a->popupMenu() );
        connect( bookmarkHandler, SIGNAL(openURL( const QString& )), SLOT(setDir( const QString& )) );
    }

    connect( m_timer, SIGNAL( timeout() ), SLOT( slotSetFilter() ) );
    connect( m_filterEdit, SIGNAL( textChanged( const QString& ) ), SLOT( slotSetFilterTimeout() ) );
    connect( m_cmbPath, SIGNAL( urlActivated( const KURL&  )), SLOT(cmbPathActivated( const KURL& )) );
    connect( m_cmbPath, SIGNAL( returnPressed( const QString&  )), SLOT(cmbPathReturnPressed( const QString& )) );
    connect( m_dir, SIGNAL(viewChanged( KFileView* )), SLOT(slotViewChanged( KFileView* )) );
    connect( m_dir, SIGNAL(fileSelected( const KFileItem* )), SLOT(activateThis( const KFileItem* )) );

    setupToolbar();

    setMinimumWidth( m_toolbar->sizeHint().width() ); //the m_toolbar minWidth is 0!
}


FileBrowser::~FileBrowser()
{
    KConfig* const c = amaroK::config( "Filebrowser" );

    m_dir->writeConfig( c ); //uses currently set group

    c->writeEntry( "Location", m_dir->url().directory( false, false ) );
    c->writeEntry( "Dir History", m_cmbPath->urls() );
}

//END Constructor/Destructor


//BEGIN Public Methods

QString FileBrowser::location() const
{
    return m_cmbPath->currentText();
}

inline void
FileBrowser::setupToolbar()
{
    QStringList actions;
    actions << "up" << "back" << "forward" << "home" << "reload" << "short view"
            << "detailed view" << "sorting menu" << "bookmarks";

    for( QStringList::ConstIterator it = actions.constBegin(); it != actions.constEnd(); ++it )
        if ( KAction *a = m_dir->actionCollection()->action( (*it).latin1() ) )
            a->plug( m_toolbar );
}

//END Public Methods


//BEGIN Private Methods

KURL::List FileBrowser::selectedItems()
{
    KURL::List list;
    for( KFileItemListIterator it( m_dir->selectedItems()->count() ? *m_dir->selectedItems() : *m_dir->view()->items() ); *it; ++it )
        list.append( (*it)->url() );

    return list;
}

bool FileBrowser::eventFilter( QObject*, QEvent *e )
{
    //we are the only ones who use this filter.. currently

    if ( !m_dir->nameFilter().isEmpty() )
        return FALSE;

    switch( e->type() ) {
    case QEvent::FocusIn:
        m_filterEdit->setPaletteForegroundColor( colorGroup().text() );
        m_filterEdit->clear();
        m_timer->stop();
        return FALSE;

    case QEvent::FocusOut:
        m_filterEdit->setPaletteForegroundColor( palette().color( QPalette::Disabled, QColorGroup::Text ) );
        m_filterEdit->setText( i18n("Filter here..") );
        m_timer->stop();
        return FALSE;

    default:
        return FALSE;
    }
}
//END Private Methods


//BEGIN Public Slots

void FileBrowser::slotSetFilter( )
{
    const QString text = m_filterEdit->text();

    if ( text.isEmpty() )
        m_dir->clearFilter();
    else {
        QString filter;
        QStringList terms = QStringList::split( ' ', text );

        for ( QStringList::Iterator it = terms.begin(); it != terms.end(); ++it ) {
            filter += '*';
            filter += *it;
        }

        filter += '*';
        m_dir->setNameFilter( filter );
    }

    m_dir->updateDir();
}


void FileBrowser::setDir( const KURL &u )
{
    m_dir->setURL( u, true );
}

//END Public Slots


//BEGIN Private Slots

//NOTE I inline all these as they are private slots and only called from within the moc segment

inline void FileBrowser::cmbPathReturnPressed( const QString& u )
{
    QStringList urls = m_cmbPath->urls();
    urls.remove( u );
    urls.prepend( u );
    m_cmbPath->setURLs( urls, KURLComboBox::RemoveBottom );
    m_dir->setFocus();
    m_dir->setURL( KURL(u), true );
}


inline void FileBrowser::dirUrlEntered( const KURL& u )
{
    m_cmbPath->setURL( u );
}


inline void FileBrowser::slotSetFilterTimeout()
{
    if ( m_timer->isActive() ) m_timer->stop();
    m_timer->start( 180, true );
}


inline void FileBrowser::clearFilter()
{
    m_filterEdit->clear();
    m_timer->stop();
    slotSetFilter();
    if( !m_filterEdit->hasFocus() ) {
        //set the lineEdit to initial state
        QEvent e( QEvent::FocusOut );
        eventFilter( m_filterEdit, &e);
    }
}


inline void FileBrowser::slotViewChanged( KFileView *view )
{
    if( view->widget()->inherits( "KListView" ) )
    {
        static_cast<KListView*>(view->widget())->setAlternateBackground( FileBrowser::altBgColor );
    }
}


inline void FileBrowser::activateThis( const KFileItem *item )
{
    Playlist::instance()->appendMedia( item->url(), true );
}


inline void FileBrowser::makePlaylist()
{
    Playlist::instance()->clear();
    Playlist::instance()->appendMedia( selectedItems() );
}


inline void FileBrowser::addToPlaylist()
{
    Playlist::instance()->appendMedia( selectedItems() );
}


inline void FileBrowser::selectAllFiles()
{
    KFileItemList list( *m_dir->view()->items() );

    // Select all items which represent files
    for ( KFileItem* item = list.first(); item; item = list.next() )
        m_dir->view()->setSelected( item, item->isFile() );
}


inline void FileBrowser::burnDataCd() // SLOT
{
    K3bExporter::instance()->exportTracks( selectedItems(), K3bExporter::DataCD );
}


inline void FileBrowser::burnAudioCd() // SLOT
{
    K3bExporter::instance()->exportTracks( selectedItems(), K3bExporter::AudioCD );
}


//END Private Slots

#include "filebrowser.moc"
