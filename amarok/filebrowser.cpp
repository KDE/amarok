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
//#include "amarokfileview.cpp" //FIXME
#include "filebrowser.h"
#include "kbookmarkhandler.h"

#include <qhbox.h>
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


QColor FileBrowser::altBgColor; //FIXME should be redundant eventually!


//BEGIN Constructor/destructor

FileBrowser::FileBrowser( const char * name )
  : QVBox( 0, name )
{
    KConfig* const config = kapp->config();
    config->setGroup( "Filebrowser" );

    m_actionCollection = new KActionCollection( this );

    m_toolbar = new FileBrowser::ToolBar( this );
    m_toolbar->setMovingEnabled(false);
    m_toolbar->setFlat(true);
    m_toolbar->setIconText( KToolBar::IconOnly );
    m_toolbar->setIconSize( 16 );
    m_toolbar->setEnableContextMenu( false );

    cmbPath = new KURLComboBox( KURLComboBox::Directories, true, this, "path combo" );
    cmbPath->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ));
    cmbPath->setCompletionObject( new KURLCompletion( KURLCompletion::DirCompletion ) );
    cmbPath->setMaxItems( 9 );
    cmbPath->setURLs( config->readListEntry( "Dir History" ) );
    setFocusProxy( cmbPath ); //so the dirOperator is focussed when we get focus events

    dir = new KDirOperator( KURL( config->readEntry( "Location" ) ), this );
    connect( dir, SIGNAL(urlEntered( const KURL& )), SLOT(dirUrlEntered( const KURL& )) );
    dir->setEnableDirHighlighting( true );
    dir->setMode( KFile::Files ); //enables multi selection mode
    dir->setOnlyDoubleClickSelectsFiles( true ); //amaroK type settings
    dir->actionCollection()->action( "delete" )->setShortcut( KShortcut( SHIFT + Key_Delete ) );
    dir->readConfig( config );
    dir->setView( KFile::Default ); //will set userconfigured view, will load URL
    //dir->setView( new amaroK::FileView( dir ) );
    setStretchFactor( dir, 2 );

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
    KConfig* const c = kapp->config();
    c->setGroup( "Filebrowser" );

    dir->writeConfig( c ); //uses currently set group

    QStringList l;
    for( int i = 0; i < cmbPath->count(); ++i ) l.append( cmbPath->text( i ) );
    c->writeEntry( "Dir History", l ); //NOTE KURLComboBox::urls() may be necessary

    c->writeEntry( "Location", cmbPath->currentText() ); //FIXME is not a properly encoded URL
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
        QToolTip::add( btnFilter, QString( i18n("Apply last filter (\"%1\")") ).arg( lastFilter ) );

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


inline void FileBrowser::activateThis( const KFileItem *item )
{
    emit activated( item->url() );
}

//END Private Slots

#include "filebrowser.moc"
