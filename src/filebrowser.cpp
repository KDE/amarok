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
#include "app.h"
#include "amarokconfig.h"
#include "filebrowser.h"
#include "kbookmarkhandler.h"
#include "playlist.h"

#include <qhbox.h>
#include <qdir.h>
#include <qlabel.h>
#include <qtoolbutton.h>
#include <qtooltip.h>

#include <kaction.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <klistview.h>   //slotViewChanged()
#include <klocale.h>
#include <kpopupmenu.h>
#include <kurlcombobox.h>
#include <kurlcompletion.h>


namespace amaroK { extern KConfig *config( const QString& ); }


QColor FileBrowser::altBgColor; //FIXME should be redundant eventually!


//BEGIN Constructor/destructor

FileBrowser::FileBrowser( const char * name )
  : QVBox( 0, name )
{
    setSpacing( 4 );
    setMargin( 5 );

    KConfig* const config = amaroK::config( "Filebrowser" );

    m_actionCollection = new KActionCollection( this );

    m_toolbar = new FileBrowser::ToolBar( this );
    m_toolbar->setMovingEnabled(false);
    m_toolbar->setFlat(true);
    m_toolbar->setIconText( KToolBar::IconOnly );
    m_toolbar->setIconSize( 16 );
    m_toolbar->setEnableContextMenu( false );

    QString currentLocation = config->readEntry( "Location" );
    QDir currentDir( currentLocation );
    if ( !currentDir.exists() )
        currentLocation = QDir::homeDirPath();

    cmbPath = new KURLComboBox( KURLComboBox::Directories, true, this, "path combo" );
    cmbPath->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ));
    cmbPath->setCompletionObject( new KURLCompletion( KURLCompletion::DirCompletion ) );
    cmbPath->setMaxItems( 9 );
    cmbPath->setURLs( config->readListEntry( "Dir History" ) );
    cmbPath->lineEdit()->setText( currentLocation );
    setFocusProxy( cmbPath ); //so the dirOperator is focussed when we get focus events
  
    dir = new KDirOperator( KURL( currentLocation ), this );
    connect( dir, SIGNAL(urlEntered( const KURL& )), SLOT(dirUrlEntered( const KURL& )) );
    ((KActionMenu *)dir->actionCollection()->action("popupMenu"))->popupMenu ()->insertItem(i18n("Make Playlist"),this,SLOT(makePlaylist()));
    ((KActionMenu *)dir->actionCollection()->action("popupMenu"))->popupMenu ()->insertItem(i18n("Add to Playlist"),this,SLOT(addToPlaylist()));
 
    dir->setEnableDirHighlighting( true );
    dir->setMode( KFile::Mode((int)KFile::Files | (int)KFile::Directory) ); //allow selection of multiple files + dirs
    dir->setOnlyDoubleClickSelectsFiles( true ); //amaroK type settings
    dir->actionCollection()->action( "delete" )->setShortcut( KShortcut( SHIFT + Key_Delete ) );
    dir->readConfig( config );
    dir->setView( KFile::Default ); //will set userconfigured view, will load URL
//    dir->layout()->setMargin( 2 );
    //dir->setView( new amaroK::FileView( dir ) );
    setStretchFactor( dir, 2 );

    //TODO enable drag from playlist, then give a konqi like popupmenu allowing copy/move/link and cancel
    //TODO if dragged to tab it should open, so you need to make tabs accept drops if browser author wants it
    //     and auto-expand if they are shut
    //dir->setAcceptDrops( true ); FIXME I think the KDirOperator won't translate from KURL to KFileItem. BAH!
    //dir->setDropOptions( KFileView::AutoOpenDirs );



    KActionMenu *acmBookmarks = new KActionMenu( i18n("Bookmarks"), "bookmark", m_actionCollection, "bookmarks" );
    acmBookmarks->setDelayed( false );
    bookmarkHandler = new KBookmarkHandler( this, acmBookmarks->popupMenu() );

    QHBox *filterBox = new QHBox( this );
    btnFilter = new QToolButton( filterBox );
    btnFilter->setIconSet( SmallIconSet( "filter" ) );
    btnFilter->setToggleButton( true );
    filter = new KHistoryCombo( true, filterBox, "filter");
    filter->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ));
    filter->setMaxCount( 9 );
    filter->setHistoryItems( config->readListEntry( "Filter History" ), true );
    filterBox->setStretchFactor( filter, 2 );

    const QString flt = config->readEntry( "Current Filter" );
    lastFilter = config->readEntry( "Last Filter" );
    filter->lineEdit()->setText( flt ); //slotFilterChange doesn't set the text
    slotFilterChange( flt );

    connect( btnFilter, SIGNAL(clicked()), this, SLOT(btnFilterClick()) );
    connect( filter, SIGNAL( activated(const QString&) ), SLOT( slotFilterChange(const QString&) ) );
    connect( filter, SIGNAL( returnPressed(const QString&) ), filter, SLOT( addToHistory(const QString&) ) );
    connect( cmbPath, SIGNAL( urlActivated( const KURL&  )), SLOT(cmbPathActivated( const KURL& )) );
    connect( cmbPath, SIGNAL( returnPressed( const QString&  )), SLOT(cmbPathReturnPressed( const QString& )) );
    connect( bookmarkHandler, SIGNAL(openURL( const QString& )), SLOT(setDir( const QString& )) );
    connect( dir, SIGNAL(viewChanged( KFileView* )), SLOT(slotViewChanged( KFileView* )) );
    connect( dir, SIGNAL(fileSelected( const KFileItem* )), SLOT(activateThis( const KFileItem* )) );

    setupToolbar();

    setMinimumWidth( m_toolbar->sizeHint().width() ); //the m_toolbar minWidth is 0!
}


FileBrowser::~FileBrowser()
{
    KConfig* const c = amaroK::config( "Filebrowser" );

    dir->writeConfig( c ); //uses currently set group

    c->writeEntry( "Location", dir->url().directory( false, false ) );
    c->writeEntry( "Dir History", cmbPath->urls() );
    c->writeEntry( "Filter History", filter->historyItems() );
    c->writeEntry( "Current Filter", filter->currentText() );
    c->writeEntry( "Last Filter", lastFilter );

    //c->writeEntry( "Filter History Len", filter->maxCount() );
    //c->writeEntry( "Set Path Combo History Len", cmbPath->maxItems() );
}

//END Constructor/Destructor


//BEGIN Public Methods

QString FileBrowser::location() const
{
    return cmbPath->currentText();
}


void FileBrowser::setupToolbar()
{
    QStringList actions;
    actions << "up" << "back" << "forward" << "home" << "reload" << "short view" << "detailed view";

    KAction *ac;
    for( QStringList::ConstIterator it = actions.constBegin(); it != actions.constEnd(); ++it )
    {
        ac = dir->actionCollection()->action( (*it).latin1() );
        if( ac ) ac->plug( m_toolbar );
    }
    m_actionCollection->action( "bookmarks" )->plug( m_toolbar );
}

//END Public Methods


//BEGIN Public Slots

void FileBrowser::slotFilterChange( const QString & nf )
{
    const QString f = nf.stripWhiteSpace();
    const bool empty = f.isEmpty() || f == "*";

    if ( empty )
    {
        dir->clearFilter();
        filter->lineEdit()->setText( QString::null );
        QToolTip::add( btnFilter, i18n("Apply last filter (\"%1\")").arg( lastFilter ) );

    } else {

        dir->setNameFilter( f );
        lastFilter = f;
        QToolTip::add( btnFilter, i18n("Clear filter") );
    }

    btnFilter->setOn( !empty );
    dir->updateDir();
    // this will be never true after the filter has been used;)
    btnFilter->setEnabled( !( empty && lastFilter.isEmpty() ) ); //FIXME can only be true in ctor, move there
}


void FileBrowser::setDir( const KURL &u )
{
    dir->setURL( u, true );
}

//END Public Slots


//BEGIN Private Slots

//NOTE I inline all these as they are private slots and only called from within the moc segment

inline void FileBrowser::cmbPathReturnPressed( const QString& u )
{
    QStringList urls = cmbPath->urls();
    urls.remove( u );
    urls.prepend( u );
    cmbPath->setURLs( urls, KURLComboBox::RemoveBottom );
    dir->setFocus();
    dir->setURL( KURL(u), true );
}


inline void FileBrowser::dirUrlEntered( const KURL& u )
{
    cmbPath->setURL( u );
}


inline void FileBrowser::btnFilterClick()
{
    if( btnFilter->isOn() )
    {
        filter->lineEdit()->setText( lastFilter );
        slotFilterChange( lastFilter );

    } else {

        slotFilterChange( QString::null );
    }
}


inline void FileBrowser::slotViewChanged( KFileView *view )
{
    if( view->widget()->inherits( "KListView" ) )
    {
        static_cast<KListView*>(view->widget())->setAlternateBackground( FileBrowser::altBgColor );
    }
}

inline void FileBrowser::makePlaylist()
{
       
      pApp->actionCollection()->action( "playlist_clear" )->activate();
      KFileItemListIterator selected( * dir->selectedItems() );
      KURL::List list;
      for ( ; selected.current(); ++selected ) {
       list.append( (*selected)->url());
      }
     pApp->playlist()->appendMedia( list, true, true );
}
inline void FileBrowser::addToPlaylist()
{
    KFileItemListIterator selected( * dir->selectedItems() );
      KURL::List list;
      for ( ; selected.current(); ++selected ) {
       list.append( (*selected)->url());
      }
     pApp->playlist()->appendMedia( list, false, true );
}

inline void FileBrowser::activateThis( const KFileItem *item )
{
    emit activated( item->url() );
}

//END Private Slots

#include "filebrowser.moc"
