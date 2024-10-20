/****************************************************************************************
 * Copyright (c) 2003 Scott Wheeler <wheeler@kde.org>                                   *
 * Copyright (c) 2004 Max Howell <max.howell@methylblue.com>                            *
 * Copyright (c) 2004-2008 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2008 Sebastian Trueg <trueg@kde.org>                                   *
 * Copyright (c) 2013 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#include "amarokconfig.h"
#include "core/collections/Collection.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "dialogs/DatabaseImporterDialog.h"

#include <KLocalizedString>

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>

CollectionSetup* CollectionSetup::s_instance;


CollectionSetup::CollectionSetup( QWidget *parent )
        : QWidget( parent )
        , m_rescanDirAction( new QAction( this ) )
{
    m_ui.setupUi(this);

    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

    setObjectName( QStringLiteral("CollectionSetup") );
    s_instance = this;

    m_ui.view->setAnimated( true );
    connect( m_ui.view, &QTreeView::clicked,
             this, &CollectionSetup::changed );

    connect( m_ui.view, &QTreeView::pressed,
             this, &CollectionSetup::slotPressed );
    connect( m_rescanDirAction, &QAction::triggered,
             this, &CollectionSetup::slotRescanDirTriggered );

    QPushButton *rescan = new QPushButton( QIcon::fromTheme( QStringLiteral("collection-rescan-amarok") ), i18n( "Full rescan" ), m_ui.buttonContainer );
    rescan->setToolTip( i18n( "Rescan your entire collection. This will <i>not</i> delete any statistics." ) );
    connect( rescan, &QAbstractButton::clicked, CollectionManager::instance(), &CollectionManager::startFullScan );

    QPushButton *import = new QPushButton( QIcon::fromTheme( QStringLiteral("tools-wizard") ), i18n( "Import batch file..." ), m_ui.buttonContainer );
    import->setToolTip( i18n( "Import collection from file produced by amarokcollectionscanner." ) );
    connect( import, &QAbstractButton::clicked, this, &CollectionSetup::importCollection );

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget( rescan );
    buttonLayout->addWidget( import );
    m_ui.buttonContainer->setLayout( buttonLayout );

    m_recursive = new QCheckBox( i18n("&Scan folders recursively (requires full rescan if newly checked)"), m_ui.checkboxContainer );
    m_monitor   = new QCheckBox( i18n("&Watch folders for changes"), m_ui.checkboxContainer );
    connect( m_recursive, &QCheckBox::toggled, this, &CollectionSetup::changed );
    connect( m_monitor  , &QCheckBox::toggled, this, &CollectionSetup::changed );

    QVBoxLayout *checkboxLayout = new QVBoxLayout();
    checkboxLayout->addWidget( m_recursive );
    checkboxLayout->addWidget( m_monitor );
    m_ui.checkboxContainer->setLayout( checkboxLayout );

    m_recursive->setToolTip( i18n( "If selected, Amarok will read all subfolders." ) );
    m_monitor->setToolTip( i18n( "If selected, the collection folders will be watched "
            "for changes.\nThe watcher will not notice changes behind symbolic links." ) );

    m_recursive->setChecked( AmarokConfig::scanRecursively() );
    m_monitor->setChecked( AmarokConfig::monitorChanges() );

    // set the model _after_ constructing the checkboxes
    m_model = new CollectionFolder::Model( this );
    m_ui.view->setModel( m_model );
    #ifndef Q_OS_WIN
    m_ui.view->setRootIndex( m_model->setRootPath( QDir::rootPath() ) );
    #else
    m_ui.view->setRootIndex( m_model->setRootPath( m_model->myComputer().toString() ) );
    #endif

    Collections::Collection *primaryCollection = CollectionManager::instance()->primaryCollection();
    QStringList dirs = primaryCollection ? primaryCollection->property( "collectionFolders" ).toStringList() : QStringList();
    m_model->setDirectories( dirs );

    // make sure that the tree is expanded to show all selected items
    for( const QString &dir : dirs )
    {
        QModelIndex index = m_model->index( dir );
        m_ui.view->scrollTo( index, QAbstractItemView::EnsureVisible );
    }
}

void
CollectionSetup::writeConfig()
{
    DEBUG_BLOCK

    AmarokConfig::setScanRecursively( recursive() );
    AmarokConfig::setMonitorChanges( monitor() );

    Collections::Collection *primaryCollection = CollectionManager::instance()->primaryCollection();
    QStringList collectionFolders = primaryCollection ? primaryCollection->property( "collectionFolders" ).toStringList() : QStringList();

    if( m_model->directories() != collectionFolders )
    {
        debug() << "Selected collection folders: " << m_model->directories();
        if( primaryCollection )
            primaryCollection->setProperty( "collectionFolders", m_model->directories() );

        debug() << "Old collection folders:      " << collectionFolders;
        CollectionManager::instance()->startFullScan();
    }
}

bool
CollectionSetup::hasChanged() const
{
    Collections::Collection *primaryCollection = CollectionManager::instance()->primaryCollection();
    QStringList collectionFolders = primaryCollection ? primaryCollection->property( "collectionFolders" ).toStringList() : QStringList();

    return
        m_model->directories() != collectionFolders ||
        m_recursive->isChecked() != AmarokConfig::scanRecursively() ||
        m_monitor->isChecked() != AmarokConfig::monitorChanges();
}

bool
CollectionSetup::recursive() const
{ return m_recursive && m_recursive->isChecked(); }

bool
CollectionSetup::monitor() const
{ return m_monitor && m_monitor->isChecked(); }

const QString
CollectionSetup::modelFilePath( const QModelIndex &index ) const
{
    return m_model->filePath( index );
}


void
CollectionSetup::importCollection()
{
    DatabaseImporterDialog *dlg = new DatabaseImporterDialog( this );
    dlg->exec(); // be modal to avoid messing about by the user in the application
}

void
CollectionSetup::slotPressed( const QModelIndex &index )
{
    DEBUG_BLOCK

    // --- show context menu on right mouse button
    if( ( QApplication::mouseButtons() & Qt::RightButton ) )
    {
        m_currDir = modelFilePath( index );
        debug() << "Setting current dir to " << m_currDir;

        // check if there is an sql collection covering the directory
        // it's covered, so we can show the rescan option
        if( isDirInCollection( m_currDir ) )
        {
            m_rescanDirAction->setText( i18n( "Rescan '%1'", m_currDir ) );
            QMenu menu;
            menu.addAction( m_rescanDirAction );
            menu.exec( QCursor::pos() );
        }
    }
}

void
CollectionSetup::slotRescanDirTriggered()
{
    DEBUG_BLOCK
    CollectionManager::instance()->startIncrementalScan( m_currDir );
}


bool
CollectionSetup::isDirInCollection( const QString& path ) const
{
    DEBUG_BLOCK

    Collections::Collection *primaryCollection = CollectionManager::instance()->primaryCollection();
    QStringList collectionFolders = primaryCollection ? primaryCollection->property( "collectionFolders" ).toStringList() : QStringList();

    for( const QString &dir : collectionFolders )
    {
        debug() << "Collection Location: " << dir;
        debug() << "path: " << path;
        debug() << "scan Recursively: " << AmarokConfig::scanRecursively();
        QUrl parentUrl = QUrl::fromLocalFile( dir );
        if ( !AmarokConfig::scanRecursively() )
        {
            if ( ( dir == path ) || ( QString( dir + QLatin1Char('/') ) == path ) )
                return true;
        }
        else //scan recursively
        {
            if (parentUrl.isParentOf( QUrl::fromLocalFile(path) ) || parentUrl.matches(QUrl::fromLocalFile(path), QUrl::StripTrailingSlash))
                return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////////////////////
// CLASS Model
//////////////////////////////////////////////////////////////////////////////////////////

namespace CollectionFolder {

    Model::Model( QObject *parent )
        : QFileSystemModel( parent )
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
                    const auto checks = m_checked;
                    for( const QString &elem : checks )
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
                    for ( const QString &elem : ancestors )
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
            Q_EMIT dataChanged( QModelIndex(), QModelIndex() );
            return true;
        }
        return QFileSystemModel::setData( index, value, role );
    }

    void
    Model::setDirectories( const QStringList &dirs )
    {
        m_checked.clear();
        for( const QString &dir : dirs )
        {
            m_checked.insert( dir );
        }
    }

    QStringList
    Model::directories() const
    {
        QStringList dirs = m_checked.values();

        std::sort( dirs.begin(), dirs.end() );

        // we need to remove any children of selected items as
        // they are redundant when recursive mode is chosen
        if( recursive() )
        {
            for( const QString &dir : dirs )
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
        return _path.startsWith( QStringLiteral("/proc/") ) || _path.startsWith( QStringLiteral("/dev/") ) || _path.startsWith( QStringLiteral("/sys/") );
    }

    bool
    Model::ancestorChecked( const QString &path ) const
    {
        // we need the trailing slash otherwise sibling folders with one as the prefix of the other are seen as parent/child
        const QString _path = normalPath( path );

        for( const QString &element : m_checked )
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
        for( const QString &element : m_checked )
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

        for( const QString &element : m_checked )
        {
            const QString _element = normalPath( element );
            if( _element.startsWith( _path ) && _element != _path )
                return true;
        }
        return false;
    }

    void
    Model::checkRecursiveSubfolders( const QString &root, const QString &excludePath )
    {
        QString _root = normalPath( root );
        QString _excludePath = normalPath( excludePath );
        if( _root == _excludePath )
            return;
        QDirIterator it( _root, QDirIterator::NoIteratorFlags );
        while( it.hasNext() )
        {
            QString nextPath = it.next();
            if( nextPath.endsWith( QStringLiteral("/.") ) || nextPath.endsWith( QStringLiteral("/..") ) )
                continue;
            if( !_excludePath.startsWith( nextPath ) )
                m_checked << nextPath;
            else
                checkRecursiveSubfolders( nextPath, excludePath );
        }
    }

} //namespace Collection
