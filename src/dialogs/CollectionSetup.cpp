/***************************************************************************
   begin                : Tue Feb 4 2003
   copyright            : (C) 2003 Scott Wheeler <wheeler@kde.org>
                        : (C) 2004 Max Howell <max.howell@methylblue.com>
                        : (C) 2004-2008 Mark Kretschmann <kretschmann@kde.org>
                        : (C) 2008 Seb Ruiz <ruiz@kde.org>
                        : (C) 2008 Sebastian Trueg <trueg@kde.org>
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "CollectionSetup.h"

#include "CollectionManager.h"
#include "Debug.h"
#include "MainWindow.h"
#include "MountPointManager.h"
#include "amarokconfig.h"

#include <KFileItem>
#include <KLocale>
#include <KPushButton>
#include <KVBox>

#include <QDir>
#include <QFile>
#include <QLabel>
#include <QToolTip>


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
        "media to make up your collection:"), this ))->setAlignment( Qt::AlignJustify );

    m_view  = new QTreeView( this );
    m_view->setHeaderHidden( true );
    m_view->setRootIsDecorated( true );
    m_view->setAnimated( true );
    m_view->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    connect( m_view, SIGNAL( clicked( const QModelIndex & ) ), this, SIGNAL( changed() ) );

    KHBox* buttonBox = new KHBox( this );

    KPushButton *rescan = new KPushButton( KIcon( "collection-rescan-amarok" ), i18n( "Rescan Collection" ), buttonBox );
    connect( rescan, SIGNAL( clicked() ), CollectionManager::instance(), SLOT( startFullScan() ) );

    KPushButton *import = new KPushButton( KIcon( "tools-wizard" ), i18n( "Import Collection" ), buttonBox );
    connect( import, SIGNAL( clicked() ), The::mainWindow(), SLOT( importCollection() ) );

    m_recursive = new QCheckBox( i18n("&Scan folders recursively"), this );
    m_monitor   = new QCheckBox( i18n("&Watch folders for changes"), this );
    connect( m_recursive, SIGNAL( toggled( bool ) ), this, SIGNAL( changed() ) );
    connect( m_monitor  , SIGNAL( toggled( bool ) ), this, SIGNAL( changed() ) );

    m_recursive->setToolTip( i18n( "If selected, Amarok will read all subfolders." ) );
    m_monitor->setToolTip(   i18n( "If selected, folders will automatically get rescanned when the content is modified, e.g. when a new file was added." ) );

    m_recursive->setChecked( AmarokConfig::scanRecursively() );
    m_monitor->setChecked( AmarokConfig::monitorChanges() );

    // set the model _after_ constructing the checkboxes
    m_model = new CollectionFolder::Model();
    m_view->setModel( m_model );
    #ifndef Q_OS_WIN
    m_view->setRootIndex( m_model->setRootPath( QDir::rootPath() ) );
    #else
    m_view->setRootIndex( m_model->setRootPath( m_model->myComputer().toString() ) );
    #endif
    
    // Read config values
    //we have to detect if this is the actual first run and not get the collectionFolders in that case
    //there won't be any anyway and accessing them creates a Sqlite database, even if the user wants to
    //use another database
    //bug 131719 131724
    //if( !Amarok::config().readEntry( "First Run", true ) )
    QStringList dirs = MountPointManager::instance()->collectionFolders();
    m_model->setDirectories( dirs );
    
    // make sure that the tree is expanded to show all selected items
    foreach( QString dir, dirs )
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

    return foldersChanged || recursiveChanged || monitorChanged;
}

void
CollectionSetup::writeConfig()
{
    DEBUG_BLOCK

    AmarokConfig::setScanRecursively( recursive() );
    AmarokConfig::setMonitorChanges( monitor() );

    if( m_model->directories() != MountPointManager::instance()->collectionFolders() )
    {
        debug() << "Selected collection folders: " << m_model->directories();
        MountPointManager::instance()->setCollectionFolders( m_model->directories() );

        debug() << "MountPointManager collection folders: " << MountPointManager::instance()->collectionFolders();
        CollectionManager::instance()->startFullScan();
    }
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
            return true;
        }
        return QFileSystemModel::setData( index, value, role );
    }

    void
    Model::setDirectories( QStringList &dirs )
    {
        m_checked.clear();
        foreach( QString dir, dirs )
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
            foreach( QString dir, dirs )
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
        foreach( QString element, m_checked )
        {
            if( path.startsWith( element ) && element != path )
                return true;
        }
        return false;
    }

    bool Model::descendantChecked( const QString& path ) const
    {
        foreach( const QString& element, m_checked )
        {
            if( element.startsWith( path ) && element != path )
                return true;
        }
        return false;
    }

} //namespace Collection

