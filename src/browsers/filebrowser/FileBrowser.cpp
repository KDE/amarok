/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2007 Mirko Stocker <me@misto.ch>
   Copyright (C) 2008-2009 Mark Kretschmann <kretschmann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "FileBrowser.h"

#include "Debug.h"
#include "collection/CollectionManager.h"
#include "browsers/filebrowser/MyDirOperator.h"
#include "browsers/filebrowser/kbookmarkhandler.h"

#include <QDir>
#include <QLineEdit>
#include <QListWidget>
#include <QToolButton>

#include <KActionCollection>
#include <KActionMenu>
#include <KApplication>
#include <KConfig>
#include <KConfigGroup>
#include <KGlobal>
#include <KHistoryComboBox>
#include <KLocale>
#include <KUrlCompletion>
#include <KUrlNavigator>
#include <kfileplacesmodel.h>


//BEGIN Toolbar
// from kfiledialog.cpp - avoid qt warning in STDERR (~/.xsessionerrors)
static void silenceQToolBar( QtMsgType, const char * ) {}


// helper classes to be able to have a toolbar without move handle
FileBrowser::ToolBar::ToolBar( QWidget *parent )
    : KToolBar( parent, "Kate FileSelector Toolbar", true )
{
    setMinimumWidth( 10 );
}

FileBrowser::ToolBar::~ToolBar()
{}


FileBrowser::Widget::Widget( const char * name , QWidget *parent )
    : KVBox( parent )
    , m_bookmarkHandler( 0 )
{
    DEBUG_BLOCK

    setObjectName( name );
    m_actionCollection = new KActionCollection( this );

    QtMsgHandler oldHandler = qInstallMsgHandler( silenceQToolBar );

    m_toolbar = new FileBrowser::ToolBar( this );
    m_toolbar->setMovable( false );
    qInstallMsgHandler( oldHandler );

    m_filePlacesModel = new KFilePlacesModel( this );
    m_urlNav = new KUrlNavigator( m_filePlacesModel, KUrl( QDir::home().path() ), this );

    setContentsMargins( 0, 0, 0, 0 );

    setFrameShape( QFrame::StyledPanel );
    setFrameShadow( QFrame::Sunken );

    KHBox* filterBox = new KHBox( this );
    m_btnFilter = new QToolButton( filterBox );
    m_btnFilter->setIcon( KIcon( "view-filter" ) );
    m_btnFilter->setCheckable( true );
    m_filter = new KHistoryComboBox( true, filterBox );
    m_filter->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );
    filterBox->setStretchFactor( m_filter, 2 );
    connect( m_btnFilter, SIGNAL( clicked() ), this, SLOT( btnFilterClick() ) );

    connect( m_filter, SIGNAL( activated( const QString& ) ), SLOT( slotFilterChange( const QString& ) ) );
    connect( m_filter, SIGNAL( editTextChanged( const QString& ) ), SLOT( slotFilterChange( const QString& ) ) );
    connect( m_filter, SIGNAL( returnPressed( const QString& ) ), m_filter, SLOT( addToHistory( const QString& ) ) );

    m_dirOperator = new MyDirOperator( QDir::home().path(), this );
    KConfigGroup config = Amarok::config( "File Browser" );
    m_dirOperator->setViewConfig( config ); //KDirOperator needs this for reading view settings
    //setView( KFile::Default );

    QPalette p = m_dirOperator->palette();
    QColor c = p.color( QPalette::Base );
    c.setAlpha( 0 );
    p.setColor( QPalette::Base, c );
    m_dirOperator->setPalette( p );

    connect( m_dirOperator, SIGNAL( viewChanged( QAbstractItemView * ) ), this, SLOT( selectorViewChanged( QAbstractItemView * ) ) );
    setStretchFactor( m_dirOperator, 2 );
    m_dirOperator->setSizePolicy( QSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding ) );

    KActionCollection *coll = m_dirOperator->actionCollection();
    // some shortcuts of diroperator that clash with Kate
    coll->action( "delete" )->setShortcut( Qt::ALT + Qt::Key_Delete );

    // bookmarks action!
    KActionMenu *acmBookmarks = new KActionMenu( KIcon( "bookmarks" ), i18n( "Bookmarks" ), this );
    // m_actionCollection->addAction( "bookmarks", acmBookmarks ); // TODO DISABLED FOR 2.0 final because of crashes
    acmBookmarks->setDelayed( false );
    m_bookmarkHandler = new KBookmarkHandler( this, acmBookmarks->menu() );

    m_toolbar->setToolButtonStyle( Qt::ToolButtonIconOnly );
    m_toolbar->setIconDimensions( 16 );
    m_toolbar->setContextMenuPolicy( Qt::NoContextMenu );

    connect( m_urlNav, SIGNAL( urlChanged( const KUrl& ) ), this, SLOT( cmbPathActivated( const KUrl& ) ) );
    connect( m_dirOperator, SIGNAL( urlEntered( const KUrl& ) ), this, SLOT( dirUrlEntered( const KUrl& ) ) );
    connect( m_dirOperator, SIGNAL( finishedLoading() ), this, SLOT( dirFinishedLoading() ) );

    // Connect the bookmark handler
    connect( m_bookmarkHandler, SIGNAL( openUrl( const QString& ) ), this, SLOT( setDir( const QString& ) ) );

    waitingUrl.clear();

    // whatsthis help
    m_urlNav->setWhatsThis( i18n( "<p>Here you can enter a path for a folder to display.</p>"
                                  "<p>To go to a folder previously entered, press the arrow on "
                                  "the right and choose one.</p><p>The entry has folder "
                                  "completion. Right-click to choose how completion should behave.</p>" ) );
    m_filter->setWhatsThis( i18n( "<p>Here you can enter a name filter to limit which files are displayed.</p>"
                                  "<p>To clear the filter, toggle off the filter button to the left.</p>"
                                  "<p>To reapply the last filter used, toggle on the filter button.</p>" ) );
    m_btnFilter->setWhatsThis( i18n( "<p>This button clears the name filter when toggled off, or "
                                     "reapplies the last filter used when toggled on.</p>" ) );

    readConfig();
    setupToolbar();

    m_actionCollection->addAssociatedWidget( this );
    foreach( QAction* action, m_actionCollection->actions() )
    action->setShortcutContext( Qt::WidgetWithChildrenShortcut );
}

FileBrowser::Widget::~Widget()
{
    DEBUG_BLOCK

    writeConfig();
}


//BEGIN Public Methods

void FileBrowser::Widget::readConfig()
{
    DEBUG_BLOCK

    KConfigGroup config = Amarok::config( "File Browser" );

    m_filter->setMaxCount( config.readEntry( "Filter History Length", 9 ) );

    m_dirOperator->readConfig( config );

    setDir( config.readEntry( "Current Directory" ) );

    m_filter->setHistoryItems( config.readEntry( "Filter History", QStringList() ), true );
    lastFilter = config.readEntry( "Last Filter" );

    const QString filter = config.readEntry( "Current Filter" );
    m_filter->lineEdit()->setText( filter );
    slotFilterChange( filter );
}


void FileBrowser::Widget::writeConfig()
{
    DEBUG_BLOCK

    KConfigGroup config = Amarok::config( "File Browser" );

    config.writeEntry( "Current Directory", m_dirOperator->url() );
    config.writeEntry( "Filter History Length", m_filter->maxCount() );
    config.writeEntry( "Filter History", m_filter->historyItems() );
    config.writeEntry( "Current Filter", m_filter->currentText() );
    config.writeEntry( "Last Filter", lastFilter );

    // Writes some settings from KDirOperator
    m_dirOperator->writeConfig( config );
}


void FileBrowser::Widget::initialDirChangeHack()
{
    setDir( waitingDir );
}


void FileBrowser::Widget::setupToolbar()
{
    QStringList actions;
    actions << "up" << "back" << "forward" << "home" << "short view" << "detailed view" << "bookmarks";

    QAction *ac;
    for ( QStringList::ConstIterator it = actions.constBegin(); it != actions.constEnd(); ++it )
    {
        if ( *it == "bookmarks" )
            ac = m_actionCollection->action(( *it ).toLatin1().constData() );
        else
            ac = m_dirOperator->actionCollection()->action(( *it ).toLatin1().constData() );
        if ( ac )
            m_toolbar->addAction( ac );
    }
}


void FileBrowser::Widget::setView( KFile::FileView view )
{
    m_dirOperator->setView( view );
    m_dirOperator->view()->setSelectionMode( QAbstractItemView::ExtendedSelection );
}

//END Public Methods

//BEGIN Public Slots

void FileBrowser::Widget::slotFilterChange( const QString & nf )
{
    DEBUG_BLOCK

    QString f = nf.trimmed();
    const bool empty = f.isEmpty() || f == "*";

    if ( empty )
    {
        m_dirOperator->clearFilter();
        m_filter->lineEdit()->setText( QString() );
        m_btnFilter->setToolTip( i18n( "Apply last filter (\"%1\")", lastFilter ) ) ;
    }
    else
    {
        f = '*' + f + '*'; // add regexp matches surrounding the filter
        m_dirOperator->setNameFilter( f );
        lastFilter = f;
        m_btnFilter->setToolTip( i18n( "Clear filter" ) );
    }

    m_btnFilter->setChecked( !empty );
    m_dirOperator->updateDir(); //FIXME Crashes here, see http://bugs.kde.org/show_bug.cgi?id=177981

    // this will be never true after the filter has been used;)
    m_btnFilter->setEnabled( !( empty && lastFilter.isEmpty() ) );
}

namespace FileBrowser
{

bool isReadable( const KUrl& url )
{
    if ( !url.isLocalFile() )
        return true; // what else can we say?

    QDir dir( url.path() );
    return dir.exists();
}

}
void FileBrowser::Widget::setDir( KUrl u )
{
    KUrl newurl;

    if ( !u.isValid() )
        newurl.setPath( QDir::homePath() );
    else
        newurl = u;

    QString pathstr = newurl.path( KUrl::AddTrailingSlash );
    newurl.setPath( pathstr );

    if ( !FileBrowser::isReadable( newurl ) )
        newurl.cd( QString::fromLatin1( ".." ) );

    if ( !FileBrowser::isReadable( newurl ) )
        newurl.setPath( QDir::homePath() );

    m_dirOperator->setUrl( newurl, true );
}

//END Public Slots

//BEGIN Private Slots

void FileBrowser::Widget::dirUrlEntered( const KUrl& u )
{
    m_urlNav->setUrl( u );
}

void FileBrowser::Widget::dirFinishedLoading()
{}

/*
   When the button in the filter box toggles:
   If off:
   If the name filer is anything but "" or "*", reset it.
   If on:
   Set last filter.
*/

void FileBrowser::Widget::btnFilterClick()
{
    if ( !m_btnFilter->isChecked() )
    {
        slotFilterChange( QString() );
    }
    else
    {
        m_filter->lineEdit()->setText( lastFilter );
        slotFilterChange( lastFilter );
    }
}

void FileBrowser::Widget::selectorViewChanged( QAbstractItemView * newView )
{
    newView->setSelectionMode( QAbstractItemView::ExtendedSelection );
}

//END Private Slots

//BEGIN Protected

void FileBrowser::Widget::focusInEvent( QFocusEvent * )
{
    m_dirOperator->setFocus();
}

bool FileBrowser::Widget::eventFilter( QObject* o, QEvent *e )
{
    return QWidget::eventFilter( o, e );
}

//END Protected


// kate: space-indent on; indent-width 4; replace-tabs on;

#include "FileBrowser.moc"

