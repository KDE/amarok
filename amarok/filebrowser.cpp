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

//BEGIN Includes
#include "amarokconfig.h"
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
#include <klocale.h>
#include <kpopupmenu.h>
#include <kurlcombobox.h>
#include <kurlcompletion.h>
//END Includes


QColor FileBrowser::altBgColor; //FIXME should be redundant eventually!

//#include "amarokfileview.cpp" //FIXME


//BEGIN Constructor/destructor

FileBrowser::FileBrowser( const char * name )
  : QVBox( 0, name )
{
    KConfig* const config = kapp->config();
    config->setGroup( "Filebrowser" );

    m_actionCollection = new KActionCollection( this );

    toolbar = new KDevFileSelectorToolBar( this );
    toolbar->setMovingEnabled(false);
    toolbar->setFlat(true);
    toolbar->setIconText( KToolBar::IconOnly );
    toolbar->setIconSize( 16 );
    toolbar->setEnableContextMenu( false );

    cmbPath = new KURLComboBox( KURLComboBox::Directories, true, this, "path combo" );
    cmbPath->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ));
    cmbPath->setCompletionObject( new KURLCompletion( KURLCompletion::DirCompletion ) );
    setFocusProxy( cmbPath ); //so the dirOperator is focussed when we get focus events

    dir = new KDevDirOperator( KURL(), this, "operator" );
    dir->readConfig( config );
    dir->setView( KFile::Default ); //will set userconfigured view
    //dir->setView( new amaroK::FileView( dir ) );
    dir->setEnableDirHighlighting( true );
    dir->actionCollection()->action( "delete" )->setShortcut( KShortcut( SHIFT + Key_Delete ) );
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
    filterBox->setStretchFactor( filter, 2 );

    connect( btnFilter, SIGNAL(clicked()), this, SLOT(btnFilterClick()) );
    connect( filter, SIGNAL( activated(const QString&) ), SLOT( slotFilterChange(const QString&) ) );
    connect( filter, SIGNAL( returnPressed(const QString&) ), filter, SLOT( addToHistory(const QString&) ) );
    connect( cmbPath, SIGNAL( urlActivated( const KURL&  )), SLOT(cmbPathActivated( const KURL& )) );
    connect( cmbPath, SIGNAL( returnPressed( const QString&  )), SLOT(cmbPathReturnPressed( const QString& )) );
    connect( dir, SIGNAL(urlEntered( const KURL& )), SLOT(dirUrlEntered( const KURL& )) );
    connect( bookmarkHandler, SIGNAL( openURL( const QString& )), SLOT( setDir( const QString& ) ) );

/*
    //NOTE why bother? this just increases the binary size and we don't have whatsthis tips anywhere else
    //     also our users know how to use a file dialog so this is similar enough IMO

    QWhatsThis::add
        ( cmbPath,
                i18n("<p>Here you can enter a path for a directory to display."
                     "<p>To go to a directory previously entered, press the arrow on "
                     "the right and choose one. <p>The entry has directory "
                     "completion. Right-click to choose how completion should behave.") );
    QWhatsThis::add
        ( filter,
                i18n("<p>Here you can enter a name filter to limit which files are displayed."
                     "<p>To clear the filter, toggle off the filter button to the left."
                     "<p>To reapply the last filter used, toggle on the filter button." ) );
    QWhatsThis::add
        ( btnFilter,
                i18n("<p>This button clears the name filter when toggled off, or "
                     "reapplies the last filter used when toggled on.") );
*/
    readConfig();

    setMinimumWidth( toolbar->sizeHint().width() ); //the toolbar minWidth is 0!
}

FileBrowser::~FileBrowser()
{
    KConfig* const c = kapp->config();
    c->setGroup( "Filebrowser" );

    dir->writeConfig( c ); //uses currently set group

    //c->writeEntry( "Set Path Combo History Len", cmbPath->maxItems() );

    QStringList l;
    for( int i = 0; i < cmbPath->count(); ++i )
        l.append( cmbPath->text( i ) );
    c->writeEntry( "Dir History", l ); //NOTE KURLComboBox::urls() may suffice

    c->writeEntry( "Location", cmbPath->currentText() );
    //c->writeEntry( "Filter History Len", filter->maxCount() );
    c->writeEntry( "Filter History", filter->historyItems() );
    c->writeEntry( "Current Filter", filter->currentText() );
    c->writeEntry( "Last Filter", lastFilter );
}
//END Constructor/Destructor


//BEGIN Public Methods
QString FileBrowser::location() const
{
    return cmbPath->currentText();
}


void FileBrowser::readConfig()
{
    setupToolbar();

    KConfig* const config = kapp->config();
    config->setGroup( "Filebrowser" );

    cmbPath->setMaxItems( /*config->readNumEntry( "Pathcombo History Len", */ 9 /*)*/ );
    cmbPath->setURLs( config->readListEntry( "Dir History" ) );

    const QString configLocation = config->readEntry( "Location" );

/*    if( config->readBoolEntry( "Restore Location", true ) && !configLocation.isEmpty() )
    {*/
        setDir( configLocation );
        cmbPath->setURL( KURL( configLocation ) ); //FIXME should we use setPath() here?
//    }

    filter->setMaxCount( /*config->readNumEntry( "Filter History Len", */ 9 /*)*/ );
    filter->setHistoryItems( config->readListEntry( "Filter History" ), true );
    lastFilter = config->readEntry( "Last Filter" );

/*    QString flt;
    if ( config->readBoolEntry( "Restore Last Filter", true ) || kapp->isRestored() )
        flt = config->readEntry( "Current Filter" )*/

    const QString flt = config->readEntry( "Current Filter" );
    filter->lineEdit()->setText( flt );
    slotFilterChange( flt );
}


void FileBrowser::setupToolbar()
{
    //toolbar->clear();

    QStringList tbactions;
    tbactions << "up" << "back" << "forward" << "home" << "reload" << "short view" << "detailed view";

    KAction *ac;
    for ( QStringList::ConstIterator it = tbactions.constBegin(); it != tbactions.constEnd(); ++it )
    {
        ac = dir->actionCollection()->action( (*it).latin1() );
        if( ac ) ac->plug( toolbar );
    }
    m_actionCollection->action( "bookmarks" )->plug( toolbar );
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
    }
    else
    {
        dir->setNameFilter( f );
        lastFilter = f;
        QToolTip::add( btnFilter, i18n("Clear filter") );
    }
    btnFilter->setOn( !empty );
    dir->updateDir();
    // this will be never true after the filter has been used;)
    btnFilter->setEnabled( !( empty && lastFilter.isEmpty() ) );

}


void FileBrowser::setDir( KURL u )
{
    dir->setURL(u, true);
}

//END Public Slots


//BEGIN Private Slots
void FileBrowser::cmbPathActivated( const KURL& u )
{
    cmbPathReturnPressed( u.url() );
}


void FileBrowser::cmbPathReturnPressed( const QString& u )
{
    QStringList urls = cmbPath->urls();
    urls.remove( u );
    urls.prepend( u );
    cmbPath->setURLs( urls, KURLComboBox::RemoveBottom );
    dir->setFocus();
    dir->setURL( KURL(u), true );
}


void FileBrowser::dirUrlEntered( const KURL& u )
{
    cmbPath->setURL( u );
}


/*
   When the button in the filter box toggles:
   If off:
   If the name filer is anything but "" or "*", reset it.
   If on:
   Set last filter.
*/
void FileBrowser::btnFilterClick()
{
    if ( !btnFilter->isOn() )
    {
        slotFilterChange( QString::null );
    }
    else
    {
        filter->lineEdit()->setText( lastFilter );
        slotFilterChange( lastFilter );
    }
}
//END Private Slots

//we override this method, so that we can set the alternateBackgroundColor
#include <klistview.h>
#include <kfileview.h>
KFileView* KDevDirOperator::createView( QWidget *parent, KFile::FileView viewType )
{
    KFileView *view = KDirOperator::createView( parent, viewType );

    if( view && view->widget()->inherits( "KListView" ) )
        static_cast<KListView*>(view->widget())->setAlternateBackground( FileBrowser::altBgColor );

    return view;
}

#include "filebrowser.moc"
