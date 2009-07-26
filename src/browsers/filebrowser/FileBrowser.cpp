/****************************************************************************************
 * Copyright (c) 2001 Christoph Cullmann <cullmann@kde.org>                             *
 * Copyright (c) 2001 Joseph Wenninger <jowenn@kde.org>                                 *
 * Copyright (c) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>                         *
 * Copyright (c) 2007 Mirko Stocker <me@misto.ch>                                       *
 * Copyright (c) 2008-2009 Mark Kretschmann <kretschmann@kde.org>                       *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "FileBrowser.h"

#include "Debug.h"
#include "collection/CollectionManager.h"
#include "browsers/filebrowser/MyDirOperator.h"
#include "browsers/filebrowser/kbookmarkhandler.h"
#include "widgets/LineEdit.h"

#include <QDir>
#include <QListWidget>
#include <QToolButton>

#include <KActionCollection>
#include <KActionMenu>
#include <KApplication>
#include <KConfigGroup>
#include <KFile>
#include <KGlobal>
#include <KHistoryComboBox>
#include <KLocale>
#include <KToolBar>
#include <KUrlCompletion>
#include <KUrlNavigator>
#include <KStandardDirs>
#include <kfileplacesmodel.h>


FileBrowser::Widget::Widget( const char * name , QWidget *parent )
    : BrowserCategory( name, parent )
    , m_bookmarkHandler( 0 )
{
    Q_UNUSED( parent )
    DEBUG_BLOCK

    setObjectName( name );
    m_actionCollection = new KActionCollection( this );

    m_toolbar = new KToolBar( this );
    m_toolbar->setMovable( false );

    m_filePlacesModel = new KFilePlacesModel( this );
    m_urlNav = new KUrlNavigator( m_filePlacesModel, KUrl( QDir::home().path() ), this );

    m_dirOperator = new MyDirOperator( QDir::home().path(), this );

    connect( m_dirOperator, SIGNAL( viewChanged( QAbstractItemView * ) ), this, SLOT( selectorViewChanged( QAbstractItemView * ) ) );
    setStretchFactor( m_dirOperator, 2 );
    m_dirOperator->setSizePolicy( QSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding ) );

    KHBox* filterBox = new KHBox( this );
    m_filterButton = new QToolButton( filterBox );
    m_filterButton->setIcon( KIcon( "view-filter" ) );
    m_filterButton->setCheckable( true );
    m_filter = new KHistoryComboBox( true, filterBox );
    m_filter->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );

    Amarok::LineEdit *lineEdit = new Amarok::LineEdit( m_filter );
    lineEdit->setClickMessage( i18n( "Enter search terms here" ) );
    m_filter->setLineEdit( lineEdit );

    filterBox->setStretchFactor( m_filter, 2 );
    connect( m_filterButton, SIGNAL( clicked() ), this, SLOT( filterButtonClicked() ) );

    connect( m_filter, SIGNAL( activated( const QString& ) ), SLOT( slotFilterChange( const QString& ) ) );
    connect( m_filter, SIGNAL( editTextChanged( const QString& ) ), SLOT( slotFilterChange( const QString& ) ) );
    connect( m_filter, SIGNAL( returnPressed( const QString& ) ), m_filter, SLOT( addToHistory( const QString& ) ) );

    KActionCollection *coll = m_dirOperator->actionCollection();
    // some shortcuts of diroperator that clash with Kate
    coll->action( "delete" )->setShortcut( Qt::ALT + Qt::Key_Delete );

    // bookmarks action!
    KActionMenu *acmBookmarks = new KActionMenu( KIcon( "bookmarks" ), i18n( "Bookmarks" ), this );
    m_actionCollection->addAction( "bookmarks", acmBookmarks );
    acmBookmarks->setDelayed( false );
    m_bookmarkHandler = new KBookmarkHandler( this, acmBookmarks->menu() );

    m_toolbar->setToolButtonStyle( Qt::ToolButtonIconOnly );
    m_toolbar->setIconDimensions( 16 );
    m_toolbar->setContextMenuPolicy( Qt::NoContextMenu );

    connect( m_urlNav, SIGNAL( urlChanged( const KUrl& ) ), this, SLOT( setDir( const KUrl& ) ) );
    connect( m_dirOperator, SIGNAL( urlEntered( const KUrl& ) ), this, SLOT( dirUrlEntered( const KUrl& ) ) );

    // Connect the bookmark handler
    connect( m_bookmarkHandler, SIGNAL( openUrl( const KUrl& ) ), this, SLOT( setDir( const KUrl& ) ) );

    // whatsthis help
    m_urlNav->setWhatsThis( i18n( "<p>Here you can enter a path for a folder to display.</p>"
                                  "<p>To go to a folder previously entered, press the arrow on "
                                  "the right and choose one.</p><p>The entry has folder "
                                  "completion. Right-click to choose how completion should behave.</p>" ) );
    m_filter->setWhatsThis( i18n( "<p>Here you can enter a name filter to limit which files are displayed.</p>"
                                  "<p>To clear the filter, toggle off the filter button to the left.</p>"
                                  "<p>To reapply the last filter used, toggle on the filter button.</p>" ) );
    m_filterButton->setWhatsThis( i18n( "<p>This button clears the name filter when toggled off, or "
                                     "reapplies the last filter used when toggled on.</p>" ) );

    readConfig();
    setupToolbar();

    m_actionCollection->addAssociatedWidget( this );
    foreach( QAction* action, m_actionCollection->actions() )
    action->setShortcutContext( Qt::WidgetWithChildrenShortcut );

    setLongDescription( i18n( "The file browser lets you browse files anywhere on your system, regardless of whether these files are part of your local collection. You can then add these files to the playlist as well as perform basic file operations." ) );

    setImagePath( KStandardDirs::locate( "data", "amarok/images/hover_info_files.png" ) );
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

    setDir( config.readEntry( "Current Directory" ) );

    // KDirOperator view configuration:
    QString viewStyle = config.readEntry( "View Style", "Simple" );
    if( viewStyle == "Detail" )
        m_dirOperator->setView( KFile::Detail );
    else if( viewStyle == "Tree" )
        m_dirOperator->setView( KFile::Tree );
    else if( viewStyle == "DetailTree" )
        m_dirOperator->setView( KFile::DetailTree );
    else
        m_dirOperator->setView( KFile::Simple );

    m_urlNav->setUrlEditable( config.readEntry( "UrlEditable", false ) );

    m_dirOperator->view()->setSelectionMode( QAbstractItemView::ExtendedSelection );

    if( config.hasKey( "Sorting" ) )
        m_dirOperator->setSorting( static_cast<QDir::SortFlags>( config.readEntry( "Sorting" ).toInt() ) );

    m_filter->setHistoryItems( config.readEntry( "Filter History", QStringList() ), true );

    m_lastFilter = config.readEntry( "Last Filter" );
}

void FileBrowser::Widget::writeConfig()
{
    DEBUG_BLOCK

    KConfigGroup config = Amarok::config( "File Browser" );

    config.writeEntry( "Current Directory", m_dirOperator->url() );
    config.writeEntry( "Filter History Length", m_filter->maxCount() );
    config.writeEntry( "Filter History", m_filter->historyItems() );
    config.writeEntry( "UrlEditable", m_urlNav->isUrlEditable() );
    config.writeEntry( "Last Filter", m_lastFilter );
    config.writeEntry( "Sorting", QString::number( static_cast<QDir::SortFlags>( m_dirOperator->sorting() ) ) );

    // Writes some settings from KDirOperator
    m_dirOperator->writeConfig( config );

    config.sync();
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

//END Public Methods

//BEGIN Public Slots

void FileBrowser::Widget::slotFilterChange( const QString & nf )
{
    DEBUG_BLOCK

    QString f = nf.trimmed();
    const bool empty = f.isEmpty() || f == "*";

    if( empty )
    {
        m_dirOperator->clearFilter();
        m_filter->lineEdit()->setText( QString() );
        m_filterButton->setToolTip( i18n( "Apply last filter (\"%1\")", m_lastFilter ) ) ;
    }
    else
    {
        if ( !f.startsWith( '*' ) && !f.endsWith( '*' ) )
            f = '*' + f + '*'; // add regexp matches surrounding the filter
        m_dirOperator->setNameFilter( f );
        m_lastFilter = f;
        m_filterButton->setToolTip( i18n( "Clear filter" ) );
    }

    m_filterButton->setChecked( !empty );
    m_dirOperator->updateDir();

    // this will be never true after the filter has been used;)
    m_filterButton->setEnabled( !( empty && m_lastFilter.isEmpty() ) );
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

void FileBrowser::Widget::setDir( const KUrl& url )
{
    DEBUG_BLOCK

    KUrl newurl;

    if ( !url.isValid() )
        newurl.setPath( QDir::homePath() );
    else
        newurl = url;

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


/*
   When the button in the filter box toggles:
   If off:
   If the name filer is anything but "" or "*", reset it.
   If on:
   Set last filter.
*/
void FileBrowser::Widget::filterButtonClicked()
{
    DEBUG_BLOCK

    if ( !m_filterButton->isChecked() )
    {
        slotFilterChange( QString() );
    }
    else
    {
        m_filter->lineEdit()->setText( m_lastFilter );
        slotFilterChange( m_lastFilter );
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

#include "FileBrowser.moc"

