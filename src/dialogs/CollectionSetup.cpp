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

#include "CollectionManager.h"
#include "Debug.h"
#include "MainWindow.h"
#include "MountPointManager.h"
#include "amarokconfig.h"
#include "dialogs/DatabaseImporterDialog.h"

#include <KLocale>
#include <KPushButton>
#include <KVBox>

#include <QDBusInterface>
#include <QDBusReply>
#include <QDir>
#include <QFile>
#include <QLabel>


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
    if( ( QApplication::mouseButtons() |= Qt::RightButton ) && parent() )
    {
        m_currDir = qobject_cast<CollectionSetup*>(parent())->modelFilePath( index );        
        debug() << "Setting current dir to " << m_currDir;
        QDBusInterface interface( "org.kde.amarok", "/SqlCollection" );
        QDBusReply<bool> reply = interface.call( "isDirInCollection", m_currDir );
        if( reply.isValid() && reply.value() )
        {
            m_rescanDirAction->setText( i18n( "Rescan" ) + " '" + m_currDir + "'" );
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

    (new QLabel( i18n(
        "These folders will be scanned for "
        "media to make up your collection. You can\n"
        "right-click on a folder to individually"
        "rescan it, if it was previously selected:"), this ))->setAlignment( Qt::AlignJustify );

    m_view  = new CollectionSetupTreeView( this );
    m_view->setHeaderHidden( true );
    m_view->setRootIsDecorated( true );
    m_view->setAnimated( true );
    m_view->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    connect( m_view, SIGNAL( clicked( const QModelIndex & ) ), this, SIGNAL( changed() ) );

    KHBox* buttonBox = new KHBox( this );

    KPushButton *rescan = new KPushButton( KIcon( "collection-rescan-amarok" ), i18n( "Fully Rescan Entire Collection" ), buttonBox );
    connect( rescan, SIGNAL( clicked() ), CollectionManager::instance(), SLOT( startFullScan() ) );

    KPushButton *import = new KPushButton( KIcon( "tools-wizard" ), i18n( "Import Statistics" ), buttonBox );
    import->setToolTip( i18n( "Import collection statistics from older Amarok versions, or from other media players." ) );
    connect( import, SIGNAL( clicked() ), this, SLOT( importCollection() ) );

    m_recursive = new QCheckBox( i18n("&Scan folders recursively"), this );
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
    
    QStringList dirs = MountPointManager::instance()->collectionFolders();
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

    const bool foldersChanged = m_model->directories() != MountPointManager::instance()->collectionFolders();
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

    if( m_model->directories() != MountPointManager::instance()->collectionFolders() )
    {
        debug() << "Selected collection folders: " << m_model->directories();
        MountPointManager::instance()->setCollectionFolders( m_model->directories() );

        debug() << "MountPointManager collection folders: " << MountPointManager::instance()->collectionFolders();
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
        if( ( recursive() && ancestorChecked( path ) ) || isForbiddenPath( path ) )
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
            QString path = filePath( index );
            // store checked paths, remove unchecked paths
            if( value.toInt() == Qt::Checked )
                m_checked.insert( path );
            else
                m_checked.remove( path );

            const QModelIndex &parentIndex = parent( index );
            const int lastRow = rowCount( parentIndex );
            emit dataChanged( sibling( 0, 0, parentIndex), sibling( lastRow, 0, parentIndex ) );
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
        QString _path = path.endsWith( '/' ) ? path : path + '/';
        return _path.startsWith( "/proc/" ) || _path.startsWith( "/dev/" ) || _path.startsWith( "/sys/" );
    }

    bool
    Model::ancestorChecked( const QString &path ) const
    {
        // we need the trailing slash otherwise sibling folders with one as the prefix of the other are seen as parent/child
        const QString _path = path.endsWith( '/' ) ? path : path + '/';

        foreach( const QString &element, m_checked )
        {
            const QString _element = element.endsWith( '/' ) ? element : element + '/';
            if( _path.startsWith( _element ) && _element != _path )
                return true;
        }
        return false;
    }

    bool Model::descendantChecked( const QString& path ) const
    {
        // we need the trailing slash otherwise sibling folders with one as the prefix of the other are seen as parent/child
        const QString _path = path.endsWith( '/' ) ? path : path + '/';

        foreach( const QString& element, m_checked )
        {
            const QString _element = element.endsWith( '/' ) ? element : element + '/';
            if( _element.startsWith( _path ) && _element != _path )
                return true;
        }
        return false;
    }

} //namespace Collection

