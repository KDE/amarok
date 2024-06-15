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

#include "FileBrowser_p.h"
#include "FileBrowser.h"

#include "amarokconfig.h"
#include "EngineController.h"
#include "core/support/Amarok.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "core-impl/meta/file/File.h"
#include "browsers/BrowserBreadcrumbItem.h"
#include "browsers/BrowserCategoryList.h"
#include "browsers/filebrowser/DirPlaylistTrackFilterProxyModel.h"
#include "browsers/filebrowser/FileView.h"
#include "playlist/PlaylistController.h"
#include "widgets/SearchWidget.h"

#include <QAction>
#include <QComboBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QStandardPaths>

#include <KConfigGroup>
#include <KDirLister>
#include <KLocalizedString>
#include <KIO/StatJob>
#include <KStandardAction>
#include <KToolBar>

static const QString placesString( "places://" );
static const QUrl placesUrl( placesString );

FileBrowser::Private::Private( FileBrowser *parent )
    : placesModel( nullptr )
    , q( parent )
{
    BoxWidget *topHBox = new BoxWidget( q );

    KToolBar *navigationToolbar = new KToolBar( topHBox );
    navigationToolbar->setToolButtonStyle( Qt::ToolButtonIconOnly );
    navigationToolbar->setIconDimensions( 16 );

    backAction = KStandardAction::back( q, &FileBrowser::back, topHBox );
    forwardAction = KStandardAction::forward( q, &FileBrowser::forward, topHBox );
    backAction->setEnabled( false );
    forwardAction->setEnabled( false );

    upAction = KStandardAction::up( q, &FileBrowser::up, topHBox );
    homeAction = KStandardAction::home( q, &FileBrowser::home, topHBox );
    refreshAction = new QAction( QIcon::fromTheme(QStringLiteral("view-refresh")), i18n( "Refresh" ), topHBox );
    QObject::connect( refreshAction, &QAction::triggered, q, &FileBrowser::refresh );

    navigationToolbar->addAction( backAction );
    navigationToolbar->addAction( forwardAction );
    navigationToolbar->addAction( upAction );
    navigationToolbar->addAction( homeAction );
    navigationToolbar->addAction( refreshAction );

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
    const QUrl homeUrl = QUrl::fromLocalFile( QDir::homePath() );
    const QUrl savedUrl = Amarok::config( "File Browser" ).readEntry( "Current Directory", homeUrl );
    bool useHome( true );
    // fall back to $HOME if the saved dir has since disappeared or is a remote one
    if( savedUrl.isLocalFile() )
    {
        QDir dir( savedUrl.path() );
        if( dir.exists() )
            useHome = false;
    }
    else
    {
        KIO::StatJob *statJob = KIO::statDetails( savedUrl, KIO::StatJob::DestinationSide);
        statJob->exec();
        if( statJob->statResult().isDir() )
        {
            useHome = false;
        }
    }
    currentPath = useHome ? homeUrl : savedUrl;
}

void
FileBrowser::Private::writeConfig()
{
    Amarok::config( "File Browser" ).writeEntry( "Current Directory", kdirModel->dirLister()->url() );
}

BreadcrumbSiblingList
FileBrowser::Private::siblingsForDir( const QUrl &path )
{
    BreadcrumbSiblingList siblings;
    if( path.scheme() == "places" )
    {
        for( int i = 0; i < placesModel->rowCount(); i++ )
        {
            QModelIndex idx = placesModel->index( i, 0 );

            QString name = idx.data( Qt::DisplayRole ).toString();
            QString url = idx.data( KFilePlacesModel::UrlRole ).toString();
            if( url.isEmpty() )
                // the place perhaps needs mounting, use places url instead
                url = placesString + name;
            siblings << BreadcrumbSibling( idx.data( Qt::DecorationRole ).value<QIcon>(),
                                           name, url );
        }
    }
    else if( path.isLocalFile() )
    {
        QDir dir( path.toLocalFile() );
        dir.cdUp();
        foreach( const QString &item, dir.entryList( QDir::Dirs | QDir::NoDotAndDotDot ) )
        {
            siblings << BreadcrumbSibling( QIcon::fromTheme( "folder-amarok" ), item,
                                           dir.absoluteFilePath( item ) );
        }
    }

    return siblings;
}

void
FileBrowser::Private::updateNavigateActions()
{
    backAction->setEnabled( !backStack.isEmpty() );
    forwardAction->setEnabled( !forwardStack.isEmpty() );
    upAction->setEnabled( currentPath != placesUrl );
}

void
FileBrowser::Private::restoreDefaultHeaderState()
{
    fileView->hideColumn( 3 );
    fileView->hideColumn( 4 );
    fileView->hideColumn( 5 );
    fileView->hideColumn( 6 );
    fileView->sortByColumn( 0, Qt::AscendingOrder );
}

void
FileBrowser::Private::restoreHeaderState()
{
    QFile file( Amarok::saveLocation() + "file_browser_layout" );
    if( !file.open( QIODevice::ReadOnly ) )
    {
        restoreDefaultHeaderState();
        return;
    }
    if( !fileView->header()->restoreState( file.readAll() ) )
    {
        warning() << "invalid header state saved, unable to restore. Restoring defaults";
        restoreDefaultHeaderState();
        return;
    }
}

void
FileBrowser::Private::saveHeaderState()
{
    //save the state of the header (column size and order). Yay, another QByteArray thingie...
    QFile file( Amarok::saveLocation() + "file_browser_layout" );
    if( !file.open( QIODevice::WriteOnly ) )
    {
        warning() << "unable to save header state";
        return;
    }
    if( file.write( fileView->header()->saveState() ) < 0 )
    {
        warning() << "unable to save header state, writing failed";
        return;
    }
}

void
FileBrowser::Private::updateHeaderState()
{
    // this slot is triggered right after model change, when currentPath is not yet updated
    if( fileView->model() == mimeFilterProxyModel && currentPath == placesUrl )
        // we are transitioning from places to files
        restoreHeaderState();
}

FileBrowser::FileBrowser( const char *name, QWidget *parent )
    : BrowserCategory( name, parent )
    , d( new FileBrowser::Private( this ) )
{
    setLongDescription( i18n( "The file browser lets you browse files anywhere on your system, "
                        "regardless of whether these files are part of your local collection. "
                        "You can then add these files to the playlist as well as perform basic "
                        "file operations." )
                       );

    setImagePath( QStandardPaths::locate( QStandardPaths::GenericDataLocation, "amarok/images/hover_info_files.png" ) );

    // set background
    if( AmarokConfig::showBrowserBackgroundImage() )
        setBackgroundImage( imagePath() );

    initView();
}

void
FileBrowser::initView()
{
    d->bottomPlacesModel = new FilePlacesModel( this );
    connect( d->bottomPlacesModel, &KFilePlacesModel::setupDone,
             this, &FileBrowser::setupDone );
    d->placesModel = new QSortFilterProxyModel( this );
    d->placesModel->setSourceModel( d->bottomPlacesModel );
    d->placesModel->setSortRole( -1 );
    d->placesModel->setDynamicSortFilter( true );
    d->placesModel->setFilterRole( KFilePlacesModel::HiddenRole );
    // HiddenRole is bool, but QVariant( false ).toString() gives "false"
    d->placesModel->setFilterFixedString( "false" );
    d->placesModel->setObjectName( "PLACESMODEL");

    d->kdirModel = new DirBrowserModel( this );
    d->mimeFilterProxyModel = new DirPlaylistTrackFilterProxyModel( this );
    d->mimeFilterProxyModel->setSourceModel( d->kdirModel );
    d->mimeFilterProxyModel->setSortCaseSensitivity( Qt::CaseInsensitive );
    d->mimeFilterProxyModel->setFilterCaseSensitivity( Qt::CaseInsensitive );
    d->mimeFilterProxyModel->setDynamicSortFilter( true );
    connect( d->searchWidget, &SearchWidget::filterChanged,
             d->mimeFilterProxyModel, &DirPlaylistTrackFilterProxyModel::setFilterFixedString );

    d->fileView->setModel( d->mimeFilterProxyModel );
    d->fileView->header()->setContextMenuPolicy( Qt::ActionsContextMenu );
    d->fileView->header()->setVisible( true );
    d->fileView->setDragEnabled( true );
    d->fileView->setSortingEnabled( true );
    d->fileView->setSelectionMode( QAbstractItemView::ExtendedSelection );
    d->readConfig();
    d->restoreHeaderState();

    setDir( d->currentPath );

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
        connect( action, &QAction::toggled, this, &FileBrowser::toggleColumn );
    }

    connect( d->fileView->header(), &QHeaderView::geometriesChanged,
             this, &FileBrowser::updateHeaderState );
    connect( d->fileView, &FileView::navigateToDirectory,
             this, &FileBrowser::slotNavigateToDirectory );
    connect( d->fileView, &FileView::refreshBrowser,
             this, &FileBrowser::refresh );
}

void
FileBrowser::updateHeaderState()
{
    d->updateHeaderState();
}


FileBrowser::~FileBrowser()
{
    if( d->fileView->model() == d->mimeFilterProxyModel && d->currentPath != placesUrl )
        d->saveHeaderState();
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

QString
FileBrowser::currentDir() const
{
    if( d->currentPath.isLocalFile() )
        return d->currentPath.toLocalFile();
    else
        return d->currentPath.url();
}

void
FileBrowser::slotNavigateToDirectory( const QModelIndex &index )
{
    if( d->currentPath == placesUrl )
    {
        QString url = index.data( KFilePlacesModel::UrlRole ).value<QString>();

        if( !url.isEmpty() )
        {
            d->backStack.push( d->currentPath );
            d->forwardStack.clear(); // navigating resets forward stack
            setDir( QUrl( url ) );
        }
        else
        {
            //check if this url needs setup/mounting
            if( index.data( KFilePlacesModel::SetupNeededRole ).value<bool>() )
            {
                d->bottomPlacesModel->requestSetup( d->placesModel->mapToSource( index ) );
            }
            else
                warning() << __PRETTY_FUNCTION__ << "empty places url that doesn't need setup?";
        }
    }
    else
    {
        KFileItem file = index.data( KDirModel::FileItemRole ).value<KFileItem>();

        if( file.isDir() )
        {
            d->backStack.push( d->currentPath );
            d->forwardStack.clear(); // navigating resets forward stack
            setDir( file.url() );
        }
        else
            warning() << __PRETTY_FUNCTION__ << "called for non-directory";
    }
}


void
FileBrowser::addItemActivated( const QString &callbackString )
{
    if( callbackString.isEmpty() )
        return;

    QUrl newPath;
    // we have been called with a places name, it means that we'll probably have to mount
    // the place
    if( callbackString.startsWith( placesString ) )
    {
        QString name = callbackString.mid( placesString.length() );
        for( int i = 0; i < d->placesModel->rowCount(); i++ )
        {
            QModelIndex idx = d->placesModel->index( i, 0 );
            if( idx.data().toString() == name )
            {
                if( idx.data( KFilePlacesModel::SetupNeededRole ).toBool() )
                {
                    d->bottomPlacesModel->requestSetup( d->placesModel->mapToSource( idx ) );
                    return;
                }
                newPath = QUrl::fromUserInput(idx.data( KFilePlacesModel::UrlRole ).toString());
                break;
            }
        }
        if( newPath.isEmpty() )
        {
            warning() << __PRETTY_FUNCTION__ << "name" << name << "not found under Places";
            return;
        }
    }
    else
        newPath = QUrl::fromUserInput(callbackString);

    d->backStack.push( d->currentPath );
    d->forwardStack.clear(); // navigating resets forward stack
    setDir( QUrl( newPath ) );
}

void
FileBrowser::setupAddItems()
{
    clearAdditionalItems();

    if( d->currentPath == placesUrl )
        return; // no more items to add

    QString workingUrl = d->currentPath.toDisplayString( QUrl::StripTrailingSlash );
    int currentPosition = 0;

    QString name;
    QString callback;
    BreadcrumbSiblingList siblings;

    // find QModelIndex of the NON-HIDDEN closestItem
    QModelIndex placesIndex;
    QUrl tempUrl = d->currentPath;
    do
    {
        placesIndex = d->bottomPlacesModel->closestItem( tempUrl );
        if( !placesIndex.isValid() )
            break; // no valid index even in the bottom model
        placesIndex = d->placesModel->mapFromSource( placesIndex );
        if( placesIndex.isValid() )
            break; // found shown placesindex, good!

        if( KIO::upUrl(tempUrl) == tempUrl )
            break; // prevent infinite loop
        tempUrl = KIO::upUrl(tempUrl);
    } while( true );

    // special handling for the first additional item
    if( placesIndex.isValid() )
    {
        name = placesIndex.data( Qt::DisplayRole ).toString();
        callback = placesIndex.data( KFilePlacesModel::UrlRole ).toString();

        QUrl currPlaceUrl = d->placesModel->data( placesIndex, KFilePlacesModel::UrlRole ).toUrl();
        currPlaceUrl.setPath( QDir::toNativeSeparators(currPlaceUrl.path() + '/') );
        currentPosition = currPlaceUrl.toString().length();
    }
    else
    {
        QRegularExpression threeSlashes( "^[^/]*/[^/]*/[^/]*/" );
        if( workingUrl.indexOf( threeSlashes ) == 0 )
            currentPosition = threeSlashes.match( workingUrl ).capturedLength();
        else
            currentPosition = workingUrl.length();

        callback = workingUrl.left( currentPosition );
        name = callback;
        if( name == "file:///" )
            name = '/'; // just niceness
        else
            name.remove( QRegularExpression( "/$" ) );
    }
    /* always provide siblings for places, regardless of what first item is; this also
     * work-arounds bug 312639, where creating QUrl with accented chars crashes */
    siblings = d->siblingsForDir( placesUrl );
    addAdditionalItem( new BrowserBreadcrumbItem( name, callback, siblings, this ) );

    // other additional items
    while( !workingUrl.midRef( currentPosition ).isEmpty() )
    {
        int nextPosition = workingUrl.indexOf( QLatin1Char('/'), currentPosition ) + 1;
        if( nextPosition <= 0 )
            nextPosition = workingUrl.length();

        name = workingUrl.mid( currentPosition, nextPosition - currentPosition );
        name.remove( QRegularExpression( "/$" ) );
        callback = workingUrl.left( nextPosition );

        siblings = d->siblingsForDir( QUrl::fromLocalFile( callback ) );
        addAdditionalItem( new BrowserBreadcrumbItem( name, callback, siblings, this ) );

        currentPosition = nextPosition;
    }

    if( parentList() )
        parentList()->childViewChanged(); // emits viewChanged() which causes breadCrumb update
}

void
FileBrowser::reActivate()
{
    d->backStack.push( d->currentPath );
    d->forwardStack.clear(); // navigating resets forward stack
    setDir( placesUrl );
}

void
FileBrowser::setDir( const QUrl &dir )
{
    if( dir == placesUrl )
    {
        if( d->currentPath != placesUrl )
        {
            d->saveHeaderState();
            d->fileView->setModel( d->placesModel );
            d->fileView->setSelectionMode( QAbstractItemView::SingleSelection );
            d->fileView->header()->setVisible( false );
            d->fileView->setDragEnabled( false );
        }
    }
    else
    {
        // if we are currently showing "places" we need to remember to change the model
        // back to the regular file model
        if( d->currentPath == placesUrl )
        {
            d->fileView->setModel( d->mimeFilterProxyModel );
            d->fileView->setSelectionMode( QAbstractItemView::ExtendedSelection );
            d->fileView->setDragEnabled( true );
            d->fileView->header()->setVisible( true );
        }
        d->kdirModel->dirLister()->openUrl( dir );
    }

    d->currentPath = dir;
    d->updateNavigateActions();
    setupAddItems();
    // set the first item as current so that keyboard navigation works
    new DelayedActivator( d->fileView );
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
    // no clearing forward stack here!
    setDir( d->forwardStack.pop() );
}

void
FileBrowser::up()
{
    if( d->currentPath == placesUrl )
        return; // nothing to do, we consider places as the root view

    QUrl upUrl = KIO::upUrl(d->currentPath);
    if( upUrl == d->currentPath ) // apparently, we cannot go up withn url
        upUrl = placesUrl;

    d->backStack.push( d->currentPath );
    d->forwardStack.clear(); // navigating resets forward stack
    setDir( upUrl );
}

void
FileBrowser::home()
{
    d->backStack.push( d->currentPath );
    d->forwardStack.clear(); // navigating resets forward stack
    setDir( QUrl::fromLocalFile( QDir::homePath() ) );
}

void
FileBrowser::refresh()
{
    setDir( d->currentPath );
}

void
FileBrowser::setupDone( const QModelIndex &index, bool success )
{
    if( success )
    {
        QString url = index.data( KFilePlacesModel::UrlRole  ).value<QString>();
        if( !url.isEmpty() )
        {
            d->backStack.push( d->currentPath );
            d->forwardStack.clear(); // navigating resets forward stack
            setDir( QUrl::fromLocalFile(url) );
        }
    }
}

DelayedActivator::DelayedActivator( QAbstractItemView *view )
    : QObject( view )
    , m_view( view )
{
    QAbstractItemModel *model = view->model();
    if( !model )
    {
        deleteLater();
        return;
    }

    // short-cut for already-filled models
    if( model->rowCount() > 0 )
    {
        slotRowsInserted( QModelIndex(), 0 );
        return;
    }

    connect( model, &QAbstractItemModel::rowsInserted, this, &DelayedActivator::slotRowsInserted );

    connect( model, &QAbstractItemModel::destroyed, this, &DelayedActivator::deleteLater );
    connect( model, &QAbstractItemModel::layoutChanged, this, &DelayedActivator::deleteLater );
    connect( model, &QAbstractItemModel::modelReset, this, &DelayedActivator::deleteLater );
}

void
DelayedActivator::slotRowsInserted( const QModelIndex &parent, int start )
{
    QAbstractItemModel *model = m_view->model();
    if( model )
    {
        // prevent duplicate calls, deleteLater() may fire REAL later
        disconnect( model, nullptr, this, nullptr );
        QModelIndex idx = model->index( start, 0, parent );
        m_view->selectionModel()->setCurrentIndex( idx, QItemSelectionModel::NoUpdate );
    }
    deleteLater();
}

#include "moc_FileBrowser.cpp"
