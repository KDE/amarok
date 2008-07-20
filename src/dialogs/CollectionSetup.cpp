/***************************************************************************
   begin                : Tue Feb 4 2003
   copyright            : (C) 2003 Scott Wheeler <wheeler@kde.org>
                        : (C) 2004 Max Howell <max.howell@methylblue.com>
                        : (C) 2004 Mark Kretschmann <markey@web.de>
                        : (C) 2008 Seb Ruiz <ruiz@kde.org>
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

#include "amarokconfig.h"
#include "mountpointmanager.h"

#include <KFileItem>
#include <KLocale>
#include <KVBox>

#include <QDir>
#include <QFile>
#include <QLabel>
#include <QPainter>
#include <QToolTip>

#include "Debug.h"

CollectionSetup* CollectionSetup::s_instance;


CollectionSetup::CollectionSetup( QWidget *parent )
        : KVBox( parent )
{
    DEBUG_BLOCK

    setObjectName( "CollectionSetup" );
    s_instance = this;

    (new QLabel( i18n(
        "These folders will be scanned for "
        "media to make up your collection:"), this ))->setAlignment( Qt::AlignJustify );

    m_view      = new CollectionView( this );
    m_recursive = new QCheckBox( i18n("&Scan folders recursively"), this );
    m_monitor   = new QCheckBox( i18n("&Watch folders for changes"), this );

    m_recursive->setToolTip( i18n( "If selected, Amarok will read all subfolders." ) );
    m_monitor->setToolTip(   i18n( "If selected, folders will automatically get rescanned when the content is modified, e.g. when a new file was added." ) );

    // Read config values
    //we have to detect if this is the actual first run and not get the collectionFolders in that case
    //there won't be any anyway and accessing them creates a Sqlite database, even if the user wants to
    //use another database
    //bug 131719 131724
    if( !Amarok::config().readEntry( "First Run", true ) )
        m_dirs = MountPointManager::instance()->collectionFolders();

    m_recursive->setChecked( AmarokConfig::scanRecursively() );
    m_monitor->setChecked( AmarokConfig::monitorChanges() );

    m_view->setHeaderHidden( true );
    m_view->setRootIsDecorated( true );
    m_view->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

#ifdef Q_OS_WIN32
    foreach( QFileInfo drive, QDir::drives () )
    {
        // exclude trailing slash on drive letter
        new CollectionFolder::Item( m_view, drive.filePath().left( 2 ) );
    }
#else
    new CollectionFolder::Item( m_view, "/" );
#endif

    setSpacing( 6 );
}


void
CollectionSetup::writeConfig()
{
    //If we are in recursive mode then we don't need to store the names of the
    //subdirectories of the selected directories
    if ( recursive() )
    {
        for ( QStringList::ConstIterator it=m_dirs.constBegin(), end = m_dirs.constEnd(); it!=end; ++it )
        {
            QStringList::Iterator jt=m_dirs.begin();
            QStringList::ConstIterator dirsEnd = m_dirs.constEnd();
            while ( jt!=dirsEnd )
            {
                if ( it==jt )
                {
                    ++jt;
                    continue;
                }
                //Note: all directories except "/" lack a trailing '/'.
                //If (*jt) is a subdirectory of (*it) it is redundant.
                //As all directories are subdirectories of "/", if "/" is selected, we
                //can delete everything else.
                if ( ( *jt ).startsWith( *it + '/' ) || *it=="/" )
                    jt = m_dirs.erase( jt );
                else
                    ++jt;
            }
        }
    }

    MountPointManager::instance()->setCollectionFolders( m_dirs );
    AmarokConfig::setScanRecursively( recursive() );
    AmarokConfig::setMonitorChanges( monitor() );
}


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS Item
//////////////////////////////////////////////////////////////////////////////////////////

namespace CollectionFolder {

Item::Item( QTreeWidget *parent, const QString &root )
    : QTreeWidgetItem( parent )
    , m_lister( this )
    , m_url( KUrl::fromPath( root ) )
    , m_listed( false )
    , m_fullyDisabled( false )
{
    //Since we create the "/" checklistitem here, we need to enable it if needed
    if( CollectionSetup::instance()->m_dirs.contains( root ) )
        setCheckState( 0, Qt::Checked );

    // we always want the root node expanded by default
    setExpanded( true );

    init();
}

Item::Item( QTreeWidgetItem *parent, const KUrl &url , bool full_disable /* default=false */ )
    : QTreeWidgetItem( parent )
    , m_lister( this )
    , m_url( url )
    , m_listed( false )
    , m_fullyDisabled( full_disable )
{
    connect( &m_lister, SIGNAL(completed()), SLOT(completed()) );
    connect( &m_lister, SIGNAL(canceled()), SLOT(completed()) );
    
    init();
}

void
Item::init()
{
    setText( 0, m_url.fileName() );
    
    m_lister.setDelayedMimeTypes( true );
    m_lister.setDirOnlyMode( true );

    setChildIndicatorPolicy( QTreeWidgetItem::ShowIndicator );
    setDisabled( isDisabled() );

    connect( &m_lister, SIGNAL(newItems( const KFileItemList& )), SLOT(newItems( const KFileItemList& )) );
}

QString
Item::fullPath() const
{
    return m_url.path();
}

void
Item::setExpanded( bool b )
{
    DEBUG_BLOCK

    if( !m_listed )
    {
        m_lister.openUrl( m_url, KDirLister::Keep );
        m_listed = true;
    }

    QTreeWidgetItem::setExpanded( b );
}


void
Item::setCheckState( int column, Qt::CheckState state )
{
    DEBUG_BLOCK

    QStringList &cs_m_dirs = CollectionSetup::instance()->m_dirs;

    QTreeWidgetItem::setCheckState( column, state );
    
    if( isFullyDisabled() )
        return;

    if( CollectionSetup::instance()->recursive() )
    {
        for( int i = 0; i < childCount(); ++i )
        {
            Item *item = dynamic_cast<Item*>( QTreeWidgetItem::child( i ) );
            if( item && !item->isFullyDisabled() )
                item->setCheckState( column, state );
        }
    }

    //If it is disabled, allow us to change its appearance (above code) but not add it
    //to the list of folders (code below)
    if( isDisabled() )
        return;

    // Update folder list
    QStringList::Iterator it = cs_m_dirs.find( m_url.path() );
    if( state == Qt::Checked )
    {
        if ( it == cs_m_dirs.end() )
            cs_m_dirs << m_url.path();

        // Deselect subdirectories if we are in recursive mode as they are redundant
        if ( CollectionSetup::instance()->recursive() )
        {
            QStringList::Iterator diriter = cs_m_dirs.begin();
            QStringList::ConstIterator end = cs_m_dirs.constEnd();
            while ( diriter != end )
            {
                // Since the dir "/" starts with '/', we need a hack to stop it removing
                // itself (it being the only path with a trailing '/')
                if ( (*diriter).startsWith( m_url.path( KUrl::AddTrailingSlash) ) && *diriter != "/" )
                    diriter = cs_m_dirs.erase(diriter);
                else
                    ++diriter;
            }
        }
    }
    else
    {
        //Deselect item and recurse through children but only deselect children if they
        //do not exist unless we are in recursive mode (where no children should be
        //selected if the parent is being unselected)
        //Note this does not do anything to the checkboxes, but they should be doing
        //the same thing as we are (hopefully)
        //Note: all paths lack a trailing '/' except for "/", which must be handled as a
        //special case
        if ( it != cs_m_dirs.end() )
            cs_m_dirs.erase( it );
        QStringList::Iterator diriter = cs_m_dirs.begin();
        QStringList::ConstIterator end = cs_m_dirs.constEnd();
        while ( diriter != end )
        {
            if ( (*diriter).startsWith( m_url.path( KUrl::AddTrailingSlash ) ) )   //path(1) adds a trailing '/'
            {
                if ( CollectionSetup::instance()->recursive() ||
                     !QFile::exists( *diriter ) )
                {
                    diriter = cs_m_dirs.erase(diriter);
                }
                else
                    ++diriter;
            }
            else
                ++diriter;
        }
    }
}


void
Item::newItems( const KFileItemList &list ) //SLOT
{
    for( KFileItemList::ConstIterator it = list.constBegin(), end = list.constEnd(); it != end; ++it )
    {
        //Fully disable (always appears off and grayed-out) if it is "/proc", "/sys" or
        //"/dev" or one of their children. This is because we will never scan them, so we
        //might as well show that.
        //These match up with the skipped dirs in CollectionScanner::readDir.
        bool disable = false;

        if ( this->m_url.fileName().isEmpty() && ( (*it).url().fileName()=="proc"
             || (*it).url().fileName()=="dev" || (*it).url().fileName()=="sys" ) )
        {
            disable = true;
        }

        disable |= this->isFullyDisabled();

        Item *item = new Item( this, (*it).url() , disable );
        
        // if we don't explicitly set the state to unchecked then QTreeWidgetItem doesn't render the checkbox
        item->setCheckState( 0, Qt::Unchecked );
        item->setIcon( 0, (*it).pixmap( KIconLoader::SizeSmall ) );

        if( !item->isFullyDisabled() )
        {
            if( CollectionSetup::instance()->recursive() && checkState(0) == Qt::Checked ||
                CollectionSetup::instance()->m_dirs.contains( item->fullPath() ) )
            {
                item->setCheckState( 0, Qt::Checked );
            }
        }
    }
}

} //namespace Collection

#include "CollectionSetup.moc"

