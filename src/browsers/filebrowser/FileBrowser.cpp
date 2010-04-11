/****************************************************************************************
 * Copyright (c) 2010 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2010 Casey Link <unnamedrambler@gmail.com>                             *
 * Copyright (c) 2010 Téo Mrnjavac <teo@kde.org>                                        *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "FileBrowser.h"

#include "BrowserBreadcrumbItem.h"
#include "BrowserCategoryList.h"
#include "core/support/Debug.h"
#include "EngineController.h"
#include "FileView.h"
#include "MimeTypeFilterProxyModel.h"
#include "playlist/PlaylistModelStack.h"

#include <KLineEdit>
#include <KDirModel>
#include <KDirLister>
#include <KStandardDirs>
#include <KToolBar>

#include <QHeaderView>
#include <QDir>

FileBrowser::FileBrowser( const char * name, QWidget *parent )
    : BrowserCategory( name, parent )
    , m_placesModel( 0 )
    , m_showingPlaces( false )
{

    DEBUG_BLOCK;

    KHBox * topHBox = new KHBox( this );

    KToolBar * navigationToolbar = new KToolBar( topHBox );
    navigationToolbar->setToolButtonStyle( Qt::ToolButtonIconOnly );
    navigationToolbar->setIconDimensions( 16 );

    //add navigation actions
    m_upAction = new QAction( KIcon( "go-up" ), "Up one level", this );
    navigationToolbar->addAction( m_upAction );
    connect( m_upAction, SIGNAL( triggered( bool) ), this, SLOT( up() ) );

    m_homeAction = new QAction( KIcon( "user-home" ), "Home", this );
    navigationToolbar->addAction( m_homeAction );
    connect( m_homeAction, SIGNAL( triggered( bool) ), this, SLOT( home() ) );

    m_placesAction = new QAction( KIcon( "folder-remote" ), "Places", this );
    navigationToolbar->addAction( m_placesAction );
    connect( m_placesAction, SIGNAL( triggered( bool) ), this, SLOT( showPlaces() ) );

    m_searchWidget = new SearchWidget( topHBox, this, false );
    m_searchWidget->setClickMessage( i18n( "Filter Files" ) );

    m_filterTimer.setSingleShot( true );
    connect( &m_filterTimer, SIGNAL( timeout() ), this, SLOT( slotFilterNow() ) );

    m_kdirModel = new KDirModel( this );

    m_mimeFilterProxyModel = new MimeTypeFilterProxyModel( EngineController::supportedMimeTypes(), this );
    m_mimeFilterProxyModel->setSourceModel( m_kdirModel );
    m_mimeFilterProxyModel->setSortCaseSensitivity( Qt::CaseInsensitive );
    m_mimeFilterProxyModel->setFilterCaseSensitivity( Qt::CaseInsensitive );
    m_mimeFilterProxyModel->sort( 0 );

    debug() << "home path: " <<  QDir::homePath();

    m_fileView = new FileView( this );
    m_fileView->setModel( m_mimeFilterProxyModel );
    m_fileView->setSortingEnabled( true );
    
    m_fileView->hideColumn( 3 );
    m_fileView->hideColumn( 4 );
    m_fileView->hideColumn( 5 );
    m_fileView->hideColumn( 6 );

    m_fileView->setDragEnabled( true );
    m_fileView->setSelectionMode( QAbstractItemView::ExtendedSelection );

    readConfig();

    m_fileView->header()->setContextMenuPolicy( Qt::ActionsContextMenu );

    for( int i = 0; i < m_fileView->model()->columnCount(); i++ )
    {
        QAction *action = new QAction( m_fileView->model()->headerData( i, Qt::Horizontal ).toString(), m_fileView->header() );
        m_fileView->header()->addAction( action );
        m_columnActions.append( action );
        action->setCheckable( true );
        if( !m_fileView->isColumnHidden( i ) )
            action->setChecked( true );
        connect( action, SIGNAL( toggled(bool) ), this, SLOT(toggleColumn(bool) ) );
    }

    connect( m_fileView, SIGNAL( activated( const QModelIndex & ) ), this, SLOT( itemActivated( const QModelIndex & ) ) );
    if( !KGlobalSettings::singleClick() )
        connect( m_fileView, SIGNAL( doubleClicked( const QModelIndex & ) ), this, SLOT( itemActivated( const QModelIndex & ) ) );

    setLongDescription( i18n( "The file browser lets you browse files anywhere on your system, regardless of whether these files are part of your local collection. You can then add these files to the playlist as well as perform basic file operations." ) );
    setImagePath( KStandardDirs::locate( "data", "amarok/images/hover_info_files.png" ) );

    
}

FileBrowser::~FileBrowser()
{
    writeConfig();
}

void
FileBrowser::toggleColumn( bool toggled )
{
    int index = m_columnActions.indexOf( qobject_cast< QAction* >( sender() ) );
    if( index != -1 )
    {
        if( toggled )
            m_fileView->showColumn( index );
        else
            m_fileView->hideColumn( index );
    }
}

void
FileBrowser::polish()
{
    DEBUG_BLOCK
    setupAddItems();
}

QString
FileBrowser::currentDir()
{
    if( m_showingPlaces )
        return "places:";
    else
        return m_currentPath;
}

void
FileBrowser::itemActivated( const QModelIndex &index )
{
    DEBUG_BLOCK

    if( m_showingPlaces )
    {
        debug() << "place activated!";
        QString placesUrl = index.data( KFilePlacesModel::UrlRole  ).value<QString>();

        if( !placesUrl.isEmpty() )
        {
            m_fileView->setModel( m_mimeFilterProxyModel );

            //needed to make the home folder url look nice. We cannot jsut strip all protocol headers
            //as that will break remote, trash, ...
            if( placesUrl.startsWith( "file://" ) )
                placesUrl = placesUrl.replace( "file://", QString() );
            
            setDir( placesUrl );
            m_showingPlaces = false;
        }
        else
        {


            //check if this url needs setup/mounting
            if( index.data( KFilePlacesModel::SetupNeededRole ).value<bool>() )
            {
                m_placesModel->requestSetup( index );
            }
            else
            {
                m_fileView->setModel( m_mimeFilterProxyModel );
            }
        }

    }
    else
    {
        KFileItem file = index.data( KDirModel::FileItemRole ).value<KFileItem>();
        KUrl filePath = file.url();
        m_currentPath = filePath.path();

        debug() << "activated url: " << filePath.url();
        debug() << "filename: " << filePath.fileName();

        if( file.isDir() ) {
            debug() << "setting root path to: " << filePath.path();
            m_kdirModel->dirLister()->openUrl( filePath );
            m_fileView->setRootIndex( index );

            //add this dir to the breadcrumb
            setupAddItems();
            activate();
        }
        else
        {
            if( EngineController::canDecode( filePath ) )
            {
                QList<KUrl> urls;
                urls << filePath;
                The::playlistController()->insertOptioned( urls, Playlist::AppendAndPlay );
            }
        }
    }
}

void
FileBrowser::slotSetFilterTimeout()
{
    KLineEdit *lineEdit = dynamic_cast<KLineEdit*>( sender() );
    if( lineEdit )
    {
        m_currentFilter = lineEdit->text();
        m_filterTimer.stop();
        m_filterTimer.start( 500 );
    }
}

void
FileBrowser::slotFilterNow()
{
    m_mimeFilterProxyModel->setFilterFixedString( m_currentFilter );

    QStringList filters;
    filters << m_currentFilter;
}

void
FileBrowser::readConfig()
{
    DEBUG_BLOCK

    KConfigGroup config = Amarok::config( "File Browser" );

    KUrl currentDirectory = config.readEntry( "Current Directory", QDir::homePath() );
    m_kdirModel->dirLister()->openUrl( currentDirectory );
    m_currentPath = currentDirectory.path();

    QFile file( Amarok::saveLocation() + "file_browser_layout" );
    QByteArray layout;
    if ( file.open( QIODevice::ReadOnly ) )
    {
        layout = file.readAll();
        file.close();
    }

    m_fileView->header()->restoreState( layout );
}

void FileBrowser::writeConfig()
{
    DEBUG_BLOCK
    KConfigGroup config = Amarok::config( "File Browser" );
    config.writeEntry( "Current Directory", m_kdirModel->dirLister()->url().toLocalFile() );
    config.sync();

    //save the state of the header (column size and order). Yay, another QByteArray thingie...
    QFile file( Amarok::saveLocation() + "file_browser_layout" );
    if ( file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
    {
        file.write( m_fileView->header()->saveState() );

        #ifdef Q_OS_UNIX  // fsync() only exists on Posix
        fsync( file.handle() );
        #endif

        file.close();
    }
}


void
FileBrowser::addItemActivated( const QString &callbackString )
{
    DEBUG_BLOCK
    
    debug() << "callback: " << callbackString;
    
    m_kdirModel->dirLister()->openUrl( KUrl( callbackString ) );
    m_currentPath = callbackString;
    setupAddItems();
    activate();
}

void
FileBrowser::setupAddItems()
{
    DEBUG_BLOCK
    clearAdditionalItems();
    
    QStringList parts = m_currentPath.split( QDir::separator() );
    QString partialPath;
    debug() << "current path" << m_currentPath;


    /*
     * A URL like /home/user/Music/Prince is shown as [Home] > [Music] > [Prince]
     */
    if( m_currentPath.startsWith( QDir::homePath() ) )
    {
        int idx = m_currentPath.indexOf( QDir::homePath() ) + QDir::homePath().size();
        // everything after the homedir e.g., Music/Prince
        QString everything_else = m_currentPath.mid( idx );
        debug() << "everything else" << everything_else;
        // replace parts with everything else
        parts = everything_else.split( QDir::separator() ) ;
        debug() << "parts" << parts;
        partialPath = QDir::homePath();

        // Add the [Home]
        QStringList siblings = siblingsForDir( QDir::homePath() );
        addAdditionalItem( new BrowserBreadcrumbItem( i18n( "Home" ), siblings, QDir::homePath(), this ) );
    }

    foreach( const QString& part, parts )
    {
        if( !part.isEmpty() )
        {
            partialPath += '/' + part;
            QStringList siblings = siblingsForDir( partialPath );
            addAdditionalItem( new BrowserBreadcrumbItem( part, siblings, partialPath, this ) );
        }
    }
}

QStringList
FileBrowser::siblingsForDir( const QString &path )
{
    // includes the dir itself
    DEBUG_BLOCK
    debug() << "path: " << path;
    QStringList siblings;

    QDir dir( path );
    QString currentName = dir.dirName();
    if( !dir.isRoot() )
    {
        dir.cdUp();
        siblings = dir.entryList( QDir::Dirs | QDir::NoDotAndDotDot );
    }
    return siblings;
}

void
FileBrowser::reActivate()
{
    DEBUG_BLOCK
    
    //go to root:
    m_kdirModel->dirLister()->openUrl( KUrl( QDir::rootPath() ) );
    m_currentPath = QDir::rootPath();
    setupAddItems();
    activate();
}

QString
FileBrowser::prettyName() const
{
    if( parentList()->activeCategory() == this )
        return QDir::rootPath();
    else
        return BrowserCategory::prettyName();
}

void
FileBrowser::setDir( const QString &dir )
{

    if( dir == "places:" )
        showPlaces();
    else
       addItemActivated( dir );  //This function just happens to do exactly what we need
}

void
FileBrowser::up()
{
    DEBUG_BLOCK
    debug() << "current dir: " << m_currentPath;

    QDir dir( m_currentPath );
    
    if( !dir.exists( m_currentPath ) )
    {
        //assume that we are browsing "places" where "up" does not really work
        //so just bounce back to the places root

        debug() << "special case for handling up when browsing 'places'";
        showPlaces();
    }
    else
    {
        KUrl url( m_currentPath);
        setDir( url.upUrl().path() );
    }
}

void
FileBrowser::home()
{
    setDir( QDir::homePath() );
}

void
FileBrowser::showPlaces()
{
    if( !m_placesModel )
    {
        m_placesModel = new KFilePlacesModel( this );
        connect( m_placesModel, SIGNAL( setupDone( const QModelIndex &, bool ) ), this, SLOT( setupDone( const QModelIndex &, bool ) ) );
    }

    clearAdditionalItems();

    QStringList siblings;
    addAdditionalItem( new BrowserBreadcrumbItem( i18n( "Places" ), siblings, QDir::homePath(), this ) );

    m_fileView->setModel( m_placesModel );
    m_showingPlaces = true;
}

void
FileBrowser::setupDone( const QModelIndex & index, bool success )
{
    DEBUG_BLOCK
    if( success )
    {
        QString placesUrl = index.data( KFilePlacesModel::UrlRole  ).value<QString>();

        if( !placesUrl.isEmpty() )
        {
            m_fileView->setModel( m_mimeFilterProxyModel );

            //needed to make folder urls look nice. We cannot just strip all protocol headers
            //as that will break remote, trash, ...
            if( placesUrl.startsWith( "file://" ) )
                placesUrl = placesUrl.replace( "file://", QString() );

            setDir( placesUrl );
            m_showingPlaces = false;
        }
    }
}



