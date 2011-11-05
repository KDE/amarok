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

#define DEBUG_PREFIX "FileBrowser"

#include "FileBrowser.h"
#include "FileBrowser_p.h"
#include "FileBrowser_p.moc"

#include "amarokconfig.h"
#include "core/support/Debug.h"
#include "BrowserBreadcrumbItem.h"
#include "BrowserCategoryList.h"
#include "EngineController.h"
#include "FileView.h"
#include "MimeTypeFilterProxyModel.h"
#include "playlist/PlaylistController.h"
#include "widgets/SearchWidget.h"

#include <KAction>
#include <KComboBox>
#include <KConfigGroup>
#include <KDirLister>
#include <KIO/NetAccess>
#include <KSaveFile>
#include <KStandardAction>
#include <KStandardDirs>
#include <KToolBar>

#include <QHeaderView>

FileBrowser::Private::Private( FileBrowser *parent )
    : placesModel( 0 )
    , showingPlaces( false )
    , q( parent )
{
    KHBox *topHBox = new KHBox( q );

    KToolBar *navigationToolbar = new KToolBar( topHBox );
    navigationToolbar->setToolButtonStyle( Qt::ToolButtonIconOnly );
    navigationToolbar->setIconDimensions( 16 );

    backAction = KStandardAction::back( q, SLOT(back()), topHBox );
    forwardAction = KStandardAction::forward( q, SLOT(forward()), topHBox );
    backAction->setEnabled( false );
    forwardAction->setEnabled( false );

    upAction = KStandardAction::up( q, SLOT(up()), topHBox );
    homeAction = KStandardAction::home( q, SLOT(home()), topHBox );
    placesAction = new KAction( KIcon( "folder-remote" ),
                               i18nc( "Show Dolphin Places the user configured", "Places" ),
                               topHBox );

    navigationToolbar->addAction( backAction );
    navigationToolbar->addAction( forwardAction );
    navigationToolbar->addAction( upAction );
    navigationToolbar->addAction( homeAction );
    navigationToolbar->addAction( placesAction );

    searchWidget = new SearchWidget( topHBox, false );
    searchWidget->setClickMessage( i18n( "Filter Files" ) );

    fileView = new FileView( q );
}

FileBrowser::Private::~Private()
{
    writeConfig();
}

void
FileBrowser::Private::readConfig()
{
    const KUrl homeUrl( QDir::homePath() );
    const KUrl savedUrl = Amarok::config( "File Browser" ).readEntry( "Current Directory", homeUrl );
    bool useHome( true );
    // fall back to $HOME if the saved dir has since disappeared or is a remote one
    if( savedUrl.isLocalFile() )
    {
        QDir dir( savedUrl.path() );
        if( dir.exists() )
            useHome = false;
    }
    else if( KIO::NetAccess::exists( savedUrl, KIO::NetAccess::DestinationSide, 0 ) )
    {
        useHome = false;
    }
    currentPath = useHome ? homeUrl : savedUrl;
}

void
FileBrowser::Private::writeConfig()
{
    Amarok::config( "File Browser" ).writeEntry( "Current Directory", kdirModel->dirLister()->url() );
}

QStringList
FileBrowser::Private::siblingsForDir( const QString &path )
{
    // includes the dir itself
    QStringList siblings;
    QDir dir( path );
    if( !dir.isRoot() )
    {
        dir.cdUp();
        siblings = dir.entryList( QDir::Dirs | QDir::NoDotAndDotDot );
    }
    return siblings;
}

void
FileBrowser::Private::updateNavigateActions()
{
    backAction->setEnabled( !backStack.isEmpty() );
    forwardAction->setEnabled( !forwardStack.isEmpty() );
    upAction->setEnabled( !showingPlaces && !QDir(currentPath.path()).isRoot() );
}

void
FileBrowser::Private::restoreHeaderState()
{
    QFile file( Amarok::saveLocation() + "file_browser_layout" );
    if( file.open( QIODevice::ReadOnly ) )
    {
        fileView->header()->restoreState( file.readAll() );
        file.close();
    }
    else
    {
        // default layout
        fileView->hideColumn( 3 );
        fileView->hideColumn( 4 );
        fileView->hideColumn( 5 );
        fileView->hideColumn( 6 );
        fileView->sortByColumn( 0, Qt::AscendingOrder );
    }
}

void
FileBrowser::Private::slotSaveHeaderState()
{
    if( !showingPlaces )
    {
        //save the state of the header (column size and order). Yay, another QByteArray thingie...
        KSaveFile file( Amarok::saveLocation() + "file_browser_layout" );
        if( file.open() )
            file.write( fileView->header()->saveState() );
        else
            debug() << "unable to save header state";

        file.finalize();
    }
}

FileBrowser::FileBrowser( const char *name, QWidget *parent )
    : BrowserCategory( name, parent )
    , d( new FileBrowser::Private( this ) )
{
    setLongDescription( i18n( "The file browser lets you browse files anywhere on your system, " \
                        "regardless of whether these files are part of your local collection. " \
                        "You can then add these files to the playlist as well as perform basic "\
                        "file operations." )
                       );
    setImagePath( KStandardDirs::locate( "data", "amarok/images/hover_info_files.png" ) );

    // set background
    if( AmarokConfig::showBrowserBackgroundImage() )
        setBackgroundImage( imagePath() );

    QTimer::singleShot( 0, this, SLOT(initView()) );
}

void
FileBrowser::initView()
{
    d->kdirModel = new DirBrowserModel( this );

    d->mimeFilterProxyModel =
            new MimeTypeFilterProxyModel( EngineController::supportedMimeTypes(), this );
    d->mimeFilterProxyModel->setSourceModel( d->kdirModel );
    d->mimeFilterProxyModel->setSortCaseSensitivity( Qt::CaseInsensitive );
    d->mimeFilterProxyModel->setFilterCaseSensitivity( Qt::CaseInsensitive );
    d->mimeFilterProxyModel->setDynamicSortFilter( true );
    connect( d->searchWidget, SIGNAL( filterChanged( const QString & ) ),
             d->mimeFilterProxyModel, SLOT( setFilterFixedString( const QString & ) ) );

    d->fileView->setModel( d->mimeFilterProxyModel );
    d->fileView->header()->setContextMenuPolicy( Qt::ActionsContextMenu );
    d->fileView->header()->setVisible( true );
    d->fileView->setDragEnabled( true );
    d->fileView->setSortingEnabled( true );
    d->fileView->setSelectionMode( QAbstractItemView::ExtendedSelection );
    d->readConfig();
    d->restoreHeaderState();

    d->kdirModel->dirLister()->openUrl( d->currentPath );

    for( int i = 0, columns = d->fileView->model()->columnCount(); i < columns ; ++i )
    {
        QAction *action =
                new QAction( d->fileView->model()->headerData( i, Qt::Horizontal ).toString(),
                             d->fileView->header()
                           );
        d->fileView->header()->addAction( action );
        d->columnActions.append( action );
        action->setCheckable( true );
        if( !d->fileView->isColumnHidden( i ) )
            action->setChecked( true );
        connect( action, SIGNAL( toggled(bool) ), this, SLOT(toggleColumn(bool) ) );
    }

    connect( d->fileView->header(), SIGNAL(geometriesChanged()), this, SLOT(slotSaveHeaderState()) );
    connect( d->fileView, SIGNAL(activated( const QModelIndex & )),
                          SLOT(itemActivated( const QModelIndex & )) );
    if( !KGlobalSettings::singleClick() )
    {
        connect( d->fileView, SIGNAL(doubleClicked( const QModelIndex & )),
                              SLOT(itemActivated( const QModelIndex & ))
               );
    }

    connect( d->placesAction, SIGNAL( triggered( bool) ), this, SLOT( showPlaces() ) );
}

FileBrowser::~FileBrowser()
{
    delete d;
}

void
FileBrowser::toggleColumn( bool toggled )
{
    int index = d->columnActions.indexOf( qobject_cast< QAction* >( sender() ) );
    if( index != -1 )
    {
        if( toggled )
            d->fileView->showColumn( index );
        else
            d->fileView->hideColumn( index );
    }
}

void
FileBrowser::polish()
{
    setupAddItems();
}

QString
FileBrowser::currentDir() const
{
    if( d->showingPlaces )
        return "places:";
    else if( d->currentPath.isLocalFile() )
        return d->currentPath.toLocalFile();
    else
        return d->currentPath.url();
}

void
FileBrowser::itemActivated( const QModelIndex &index )
{
    if( d->showingPlaces )
    {
        QString placesUrl = index.data( KFilePlacesModel::UrlRole  ).value<QString>();

        if( !placesUrl.isEmpty() )
        {
            d->fileView->setModel( d->mimeFilterProxyModel );
            d->fileView->setSelectionMode( QAbstractItemView::ExtendedSelection );
            d->fileView->setDragEnabled( true );
            d->fileView->header()->setVisible( true );
            d->restoreHeaderState();
            d->showingPlaces = false;

            //needed to make the home folder url look nice. We cannot jsut strip all protocol headers
            //as that will break remote, trash, ...
            if( placesUrl.startsWith( "file://" ) )
                placesUrl = placesUrl.replace( "file://", QString() );

            d->backStack.push( d->currentPath );
            setDir( KUrl( placesUrl ) );
        }
        else
        {
            //check if this url needs setup/mounting
            if( index.data( KFilePlacesModel::SetupNeededRole ).value<bool>() )
            {
                d->placesModel->requestSetup( index );
            }
            else
            {
                d->fileView->setModel( d->mimeFilterProxyModel );
                d->fileView->setSelectionMode( QAbstractItemView::ExtendedSelection );
                d->fileView->header()->setVisible( true );
                d->fileView->setDragEnabled( true );
                d->restoreHeaderState();
                d->showingPlaces = false;
            }
        }
    }
    else
    {
        KFileItem file = index.data( KDirModel::FileItemRole ).value<KFileItem>();
        KUrl filePath = file.url();

        if( file.isDir() )
        {
            d->backStack.push( d->currentPath );
            setDir( filePath );
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
FileBrowser::addItemActivated( const QString &callbackString )
{
    if( callbackString.isEmpty() )
        return;

    d->kdirModel->dirLister()->openUrl( KUrl( callbackString ) );
    d->backStack.push( d->currentPath );
    d->currentPath = KUrl(callbackString);
    d->updateNavigateActions();
    setupAddItems();
    activate();
}

void
FileBrowser::setupAddItems()
{
    clearAdditionalItems();

    if( d->currentPath.isLocalFile() )
    {
        const QString localPath = d->currentPath.toLocalFile();
        QStringList parts;
        QString partialPath;

        /*
         * A URL like /home/user/Music/Prince is shown as [Home] > [Music] > [Prince]
         */
        if( localPath.startsWith( QDir::homePath() ) )
        {
            int idx = localPath.indexOf( QDir::homePath() ) + QDir::homePath().size();
            // everything after the homedir e.g., Music/Prince
            QString everything_else = localPath.mid( idx );
            // replace parts with everything else
            parts = everything_else.split( QDir::separator() ) ;
            partialPath = QDir::homePath();

            // Add the [Home]
            QStringList siblings = d->siblingsForDir( QDir::homePath() );
            addAdditionalItem( new BrowserBreadcrumbItem( i18n( "Home" ), siblings,
                                                         QDir::homePath(), this )
                             );
        }
        else
        {
            parts = localPath.split( QDir::separator() );
        }

        foreach( const QString& part, parts )
        {
            if( !part.isEmpty() )
            {
                partialPath += '/' + part;
                QStringList siblings = d->siblingsForDir( partialPath );
                addAdditionalItem( new BrowserBreadcrumbItem( part, siblings, partialPath, this ) );
            }
        }
    }
    else
    {
        // TODO: setup remote siblings in breadcrumb arrows
        const QString proto = d->currentPath.protocol();
        const QString authority = d->currentPath.authority();
        const QString protoAuthority = QString( "%1://%2" ).arg( proto, authority );
        addAdditionalItem( new BrowserBreadcrumbItem( proto + QLatin1Char(':'),
                                                      QStringList(), proto + QLatin1String("://"),
                                                      this )
                         );
        addAdditionalItem( new BrowserBreadcrumbItem( authority, QStringList(), protoAuthority,
                                                     this )
                         );
        QStringList parts = d->currentPath.path().split( QLatin1Char('/'), QString::SkipEmptyParts );
        QString partialPath = protoAuthority;
        foreach( const QString &part, parts )
        {
            partialPath += QLatin1Char('/') + part;
            addAdditionalItem( new BrowserBreadcrumbItem( part, QStringList(), partialPath, this ) );
        }
    }
}

void
FileBrowser::reActivate()
{
    //go to root:
    d->kdirModel->dirLister()->openUrl( KUrl( QDir::rootPath() ) );
    d->currentPath = KUrl( QDir::rootPath() );
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
FileBrowser::setDir( const KUrl &dir )
{
    if( dir == "places:" )
        showPlaces();
    else
    {
       //if we are currently showing "places" we need to remember to change the model
       //back to the regular file model
       if( d->showingPlaces )
       {
           d->fileView->setModel( d->mimeFilterProxyModel );
           d->fileView->setSelectionMode( QAbstractItemView::ExtendedSelection );
           d->fileView->setDragEnabled( true );
           d->fileView->header()->setVisible( true );
           d->restoreHeaderState(); // read config so the header state is restored
           d->showingPlaces = false;
       }

       d->kdirModel->dirLister()->openUrl( dir );
       d->currentPath = dir;
       d->updateNavigateActions();
       setupAddItems();
       activate();
    }
}

void
FileBrowser::back()
{
    if( d->backStack.isEmpty() )
        return;

    d->forwardStack.push( d->currentPath );
    setDir( d->backStack.pop() );
}

void
FileBrowser::forward()
{
    if( d->forwardStack.isEmpty() )
        return;

    d->backStack.push( d->currentPath );
    setDir( d->forwardStack.pop() );
}

void
FileBrowser::up()
{
    if( d->showingPlaces )
    {
        //apparently, the root level of "places" counts as a valid dir. If we are here, make the
        //up button simply go to "home"
        home();
    }
    else
    {
        d->backStack.push( d->currentPath );
        setDir( d->currentPath.upUrl() );
    }
}

void
FileBrowser::home()
{
    setDir( KUrl( QDir::homePath() ) );
}

void
FileBrowser::showPlaces()
{
    if( !d->placesModel )
    {
        d->placesModel = new FilePlacesModel( this );
        d->placesModel->setObjectName( "PLACESMODEL");
        connect( d->placesModel, SIGNAL(setupDone( const QModelIndex &, bool )),
                                 SLOT(setupDone( const QModelIndex &, bool )) );
    }

    clearAdditionalItems();

    QStringList siblings;
    addAdditionalItem( new BrowserBreadcrumbItem( i18n( "Places" ), siblings, QDir::homePath(),
                                                  this )
                     );
    d->showingPlaces = true;
    d->fileView->setModel( d->placesModel );
    d->fileView->setSelectionMode( QAbstractItemView::SingleSelection );
    d->fileView->header()->setVisible( false );
    d->fileView->setDragEnabled( false );
}

void
FileBrowser::setupDone( const QModelIndex & index, bool success )
{
    if( success )
    {
        QString placesUrl = index.data( KFilePlacesModel::UrlRole  ).value<QString>();

        if( !placesUrl.isEmpty() )
        {
            d->fileView->setModel( d->mimeFilterProxyModel );
            d->fileView->setSelectionMode( QAbstractItemView::ExtendedSelection );
            d->fileView->header()->setVisible( true );
            d->fileView->setDragEnabled( true );
            d->restoreHeaderState();
            d->showingPlaces = false;

            //needed to make folder urls look nice. We cannot just strip all protocol headers
            //as that will break remote, trash, ...
            if( placesUrl.startsWith( "file://" ) )
                placesUrl = placesUrl.replace( "file://", QString() );

            setDir( placesUrl );
        }
    }
}

#include "FileBrowser.moc"
