/****************************************************************************************
 * Copyright (c) 2003 Scott Wheeler <wheeler@kde.org>                                   *
 * Copyright (c) 2004 Max Howell <max.howell@methylblue.com>                            *
 * Copyright (c) 2004-2008 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2008 Sebastian Trueg <trueg@kde.org>                                   *
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

#include "CollectionSetup.h"

#include "core-impl/collections/support/CollectionManager.h"
#include "core-impl/collections/db/sql/SqlCollection.h"
#include "core/support/Debug.h"
#include "amarokconfig.h"
#include "dialogs/DatabaseImporterDialog.h"

#include <KLocale>
#include <KGlobalSettings>
#include <KPushButton>
#include <KVBox>

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QMenu>

CollectionSetupTreeView::CollectionSetupTreeView( QWidget *parent )
        : QTreeView( parent )
{
    DEBUG_BLOCK
    m_rescanDirAction = new QAction( this );
    connect( this, SIGNAL( pressed(const QModelIndex &) ), this, SLOT( slotPressed(const QModelIndex&) ) );
    connect( m_rescanDirAction, SIGNAL( triggered() ), this, SLOT( slotRescanDirTriggered() ) );
}

CollectionSetupTreeView::~CollectionSetupTreeView()
{
    this->disconnect();
    delete m_rescanDirAction;
}

void
CollectionSetupTreeView::slotPressed( const QModelIndex &index )
{
    DEBUG_BLOCK
    // --- show context menu on right mouse button
    if( ( QApplication::mouseButtons() & Qt::RightButton ) && parent() )
    {
        m_currDir = qobject_cast<CollectionSetup*>(parent())->modelFilePath( index );
        debug() << "Setting current dir to " << m_currDir;

        // check if there is an sql collection covering the directory
        bool covered = false;
        QList<Collections::Collection*> queryableCollections = CollectionManager::instance()->queryableCollections();
        foreach( Collections::Collection *collection, queryableCollections )
        {
            if( collection->isDirInCollection( m_currDir ) )
                covered = true;
        }

        // it's covered, so we can show the rescan option
        if( covered )
        {
            m_rescanDirAction->setText( i18n( "Rescan '%1'", m_currDir ) );
            QMenu menu;
            menu.addAction( m_rescanDirAction );
            menu.exec( QCursor::pos() );
        }
    }
}

void
CollectionSetupTreeView::slotRescanDirTriggered()
{
    DEBUG_BLOCK
    CollectionManager::instance()->startIncrementalScan( m_currDir );
}

CollectionSetup* CollectionSetup::s_instance;


CollectionSetup::CollectionSetup( QWidget *parent )
        : KVBox( parent )
{
    DEBUG_BLOCK

    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

    setObjectName( "CollectionSetup" );
    s_instance = this;

    QLabel* descriptionLabel = new QLabel( i18n( 
        "These folders will be scanned for "
        "media to make up your collection. You can "
        "right-click on a folder to individually "
        "rescan it, if it was previously selected:" ), this );
    descriptionLabel->setAlignment( Qt::AlignJustify );
    descriptionLabel->setWordWrap( true );

    m_view  = new CollectionSetupTreeView( this );
    m_view->setHeaderHidden( true );
    m_view->setRootIsDecorated( true );
    if( KGlobalSettings::graphicEffectsLevel() != KGlobalSettings::NoEffects )
        m_view->setAnimated( true );
    m_view->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    connect( m_view, SIGNAL( clicked( const QModelIndex & ) ), this, SIGNAL( changed() ) );

    KHBox* buttonBox = new KHBox( this );

    KPushButton *rescan = new KPushButton( KIcon( "collection-rescan-amarok" ), i18n( "Full rescan" ), buttonBox );
    rescan->setToolTip( i18n( "Rescan your entire collection. This will <i>not</i> delete any statistics." ) );
    connect( rescan, SIGNAL( clicked() ), CollectionManager::instance(), SLOT( startFullScan() ) );

    KPushButton *import = new KPushButton( KIcon( "tools-wizard" ), i18n( "Import" ), buttonBox );
    import->setToolTip( i18n( "Import collection and/or statistics from older Amarok versions, the batch scanner or media players." ) );
    connect( import, SIGNAL( clicked() ), this, SLOT( importCollection() ) );

    m_recursive = new QCheckBox( i18n("&Scan folders recursively (requires full rescan if newly checked)"), this );
    m_monitor   = new QCheckBox( i18n("&Watch folders for changes"), this );
    m_charset   = new QCheckBox( i18n("&Enable character set detection in ID3 tags"), this );
    connect( m_recursive, SIGNAL( toggled( bool ) ), this, SIGNAL( changed() ) );
    connect( m_monitor  , SIGNAL( toggled( bool ) ), this, SIGNAL( changed() ) );
    connect( m_charset  , SIGNAL( toggled( bool ) ), this, SIGNAL( changed() ) );

    m_recursive->setToolTip( i18n( "If selected, Amarok will read all subfolders." ) );
    m_monitor->setToolTip(   i18n( "If selected, folders will automatically get rescanned when the content is modified, e.g. when a new file was added." ) );
    m_charset->setToolTip(   i18n( "If selected, Amarok will use Mozilla's Character Set Detector to attempt to automatically guess the character sets used in ID3 tags." ) );

    m_recursive->setChecked( AmarokConfig::scanRecursively() );
    m_monitor->setChecked( AmarokConfig::monitorChanges() );
    m_charset->setChecked( AmarokConfig::useCharsetDetector() );

    // set the model _after_ constructing the checkboxes
    m_model = new CollectionFolder::Model();
    m_view->setModel( m_model );
    #ifndef Q_OS_WIN
    m_view->setRootIndex( m_model->setRootPath( QDir::rootPath() ) );
    #else
    m_view->setRootIndex( m_model->setRootPath( m_model->myComputer().toString() ) );
    #endif
    
    Collections::Collection *primaryCollection = CollectionManager::instance()->primaryCollection();
    QStringList dirs = primaryCollection ? primaryCollection->property( "collectionFolders" ).toStringList() : QStringList();
    m_model->setDirectories( dirs );
    
    // make sure that the tree is expanded to show all selected items
    foreach( const QString &dir, dirs )
    {
        QModelIndex index = m_model->index( dir );
        m_view->scrollTo( index, QAbstractItemView::EnsureVisible );
    }

    setSpacing( 6 );
}

bool
CollectionSetup::hasChanged() const
{
    DEBUG_BLOCK

    Collections::Collection *primaryCollection = CollectionManager::instance()->primaryCollection();
    QStringList collectionFolders = primaryCollection ? primaryCollection->property( "collectionFolders" ).toStringList() : QStringList();
    const bool foldersChanged = m_model->directories() != collectionFolders;
    const bool recursiveChanged = m_recursive->isChecked() != AmarokConfig::scanRecursively();
    const bool monitorChanged  = m_monitor->isChecked() != AmarokConfig::monitorChanges();
    const bool charsetChanged  = m_charset->isChecked() != AmarokConfig::useCharsetDetector();

    return foldersChanged || recursiveChanged || monitorChanged || charsetChanged;
}

void
CollectionSetup::writeConfig()
{
    DEBUG_BLOCK

    AmarokConfig::setScanRecursively( recursive() );
    AmarokConfig::setMonitorChanges( monitor() );
    AmarokConfig::setUseCharsetDetector( charset() );

    Collections::Collection *primaryCollection = CollectionManager::instance()->primaryCollection();
    QStringList collectionFolders = primaryCollection ? primaryCollection->property( "collectionFolders" ).toStringList() : QStringList();

    if( m_model->directories() != collectionFolders )
    {
        debug() << "Selected collection folders: " << m_model->directories();
        if( primaryCollection )
            primaryCollection->setProperty( "collectionFolders", m_model->directories() );

        debug() << "MountPointManager collection folders: " << collectionFolders;
        CollectionManager::instance()->startFullScan();
    }
}

void
CollectionSetup::importCollection()
{
    DatabaseImporterDialog *dlg = new DatabaseImporterDialog( this );
    dlg->exec(); // be modal to avoid messing about by the user in the application
}

const QString
CollectionSetup::modelFilePath( const QModelIndex &index ) const
{
    return m_model->filePath( index );
}


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS Model
//////////////////////////////////////////////////////////////////////////////////////////

namespace CollectionFolder {

    Model::Model()
        : QFileSystemModel()
    {
        setFilter( QDir::AllDirs | QDir::NoDotAndDotDot );
    }

    Qt::ItemFlags
    Model::flags( const QModelIndex &index ) const
    {
        Qt::ItemFlags flags = QFileSystemModel::flags( index );
        const QString path = filePath( index );
        if( isForbiddenPath( path ) )
            flags ^= Qt::ItemIsEnabled; //disabled!
       
        flags |= Qt::ItemIsUserCheckable;

        return flags;
    }

    QVariant
    Model::data( const QModelIndex& index, int role ) const
    {
        if( index.isValid() && index.column() == 0 && role == Qt::CheckStateRole )
        {
            const QString path = filePath( index );
            if( recursive() && ancestorChecked( path ) )
                return Qt::Checked; // always set children of recursively checked parents to checked
            if( isForbiddenPath( path ) )
                return Qt::Unchecked; // forbidden paths can never be checked
            if( !m_checked.contains( path ) && descendantChecked( path ) )
                return Qt::PartiallyChecked;
            return m_checked.contains( path ) ? Qt::Checked : Qt::Unchecked;
        }
        return QFileSystemModel::data( index, role );
    }

    bool
    Model::setData( const QModelIndex& index, const QVariant& value, int role )
    {
        if( index.isValid() && index.column() == 0 && role == Qt::CheckStateRole )
        {
            const QString path = filePath( index );
            if( value.toInt() == Qt::Checked )
            {
                // New path selected
                if( recursive() )
                {
                    // Recursive, so clear any paths in m_checked that are made
                    // redundant by this new selection
                    QString _path = normalPath( path );
                    foreach( QString elem, m_checked )
                    {
                        if( normalPath( elem ).startsWith( _path ) )
                            m_checked.remove( elem );
                    }
                }
                m_checked << path;
            }
            else
            {
                // Path un-selected
                m_checked.remove( path );
                if( recursive() && ancestorChecked( path ) )
                {
                    // Recursive, so we need to deal with the case of un-selecting
                    // an implicitly selected path
                    const QStringList ancestors = allCheckedAncestors( path );
                    QString topAncestor;
                    // Remove all selected ancestor of path, and find shallowest
                    // ancestor
                    foreach( QString elem, ancestors )
                    {
                        m_checked.remove( elem );
                        if( elem < topAncestor || topAncestor.isEmpty() )
                            topAncestor = elem;
                    }
                    // Check all paths reachable from topAncestor, except for
                    // those that are ancestors of path
                    checkRecursiveSubfolders( topAncestor, path );
                }
            }
            // A check or un-check can possibly require the whole view to change,
            // so we signal that the root's data is changed
            emit dataChanged( QModelIndex(), QModelIndex() );
            return true;
        }
        return QFileSystemModel::setData( index, value, role );
    }

    void
    Model::setDirectories( QStringList &dirs )
    {
        m_checked.clear();
        foreach( const QString &dir, dirs )
        {
            m_checked.insert( dir );
        }
    }

    QStringList
    Model::directories() const
    {
        QStringList dirs = m_checked.toList();

        qSort( dirs.begin(), dirs.end() );

        // we need to remove any children of selected items as 
        // they are redundant when recursive mode is chosen
        if( recursive() )
        {
            foreach( const QString &dir, dirs )
            {
                if( ancestorChecked( dir ) )
                    dirs.removeAll( dir );
            }
        }

        return dirs;
    }

    inline bool
    Model::isForbiddenPath( const QString &path ) const
    {
        // we need the trailing slash otherwise we could forbid "/dev-music" for example
        QString _path = normalPath( path );
        return _path.startsWith( "/proc/" ) || _path.startsWith( "/dev/" ) || _path.startsWith( "/sys/" );
    }

    bool
    Model::ancestorChecked( const QString &path ) const
    {
        // we need the trailing slash otherwise sibling folders with one as the prefix of the other are seen as parent/child
        const QString _path = normalPath( path );

        foreach( const QString &element, m_checked )
        {
            const QString _element = normalPath( element );
            if( _path.startsWith( _element ) && _element != _path )
                return true;
        }
        return false;
    }

    /**
     * Get a list of all checked paths that are an ancestor of
     * the given path.
     */
    QStringList
    Model::allCheckedAncestors( const QString &path ) const
    {
        const QString _path = normalPath( path );
        QStringList rtn;
        foreach( const QString &element, m_checked )
        {
            const QString _element = normalPath( element );
            if ( _path.startsWith( _element ) && _element != _path )
                rtn << element;
        }
        return rtn;
    }

    bool
    Model::descendantChecked( const QString &path ) const
    {
        // we need the trailing slash otherwise sibling folders with one as the prefix of the other are seen as parent/child
        const QString _path = normalPath( path );

        foreach( const QString& element, m_checked )
        {
            const QString _element = normalPath( element );
            if( _element.startsWith( _path ) && _element != _path )
                return true;
        }
        return false;
    }

    /**
     * Check the logical recursive difference of root and excludePath.
     * For example, if excludePath is a grandchild of root, then this method
     * will check all of the children of root except the one that is the 
     * parent of excludePath, as well as excludePath's siblings.
     */
    void
    Model::checkRecursiveSubfolders( const QString &root, const QString &excludePath )
    {
        QString _root = normalPath( root );
        QString _excludePath = normalPath( excludePath );
        if( _root == _excludePath )
            return;
        QDirIterator it( _root );
        while( it.hasNext() )
        {
            QString nextPath = it.next();
            if( nextPath.endsWith( "/." ) || nextPath.endsWith( "/.." ) )
                continue;
            if( !_excludePath.startsWith( nextPath ) )
                m_checked << nextPath;
            else
                checkRecursiveSubfolders( nextPath, excludePath );
        }
    }

} //namespace Collection

