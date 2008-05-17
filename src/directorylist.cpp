/***************************************************************************
                         directorylist.cpp
                            -------------------
   begin                : Tue Feb 4 2003
   copyright            : (C) 2003 Scott Wheeler <wheeler@kde.org>
                        : (C) 2004 Max Howell <max.howell@methylblue.com>
                        : (C) 2004 Mark Kretschmann <markey@web.de>
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "directorylist.h"

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


CollectionSetup* CollectionSetup::s_instance;


CollectionSetup::CollectionSetup( QWidget *parent )
        : KVBox( parent )
{
    setObjectName( "CollectionSetup" );
    s_instance = this;

    (new QLabel( i18n(
        "These folders will be scanned for "
        "media to make up your collection:"), this ))->setAlignment( Qt::AlignJustify );

//     m_view = new QTreeWidget( this );
//     m_view->setColumnCount( 1 );
    m_dirOperator = new KDirOperator( KUrl( "/" ), this );
    m_dirOperator->setSizePolicy (QSizePolicy (QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
    m_dirOperator->setView( KFile::Simple );
    m_dirOperator->dirLister()->setDirOnlyMode( true );
    m_dirOperator->show();
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

//     m_view->setRootIsDecorated( true );
//     m_view->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

//     reinterpret_cast<QWidget*>(m_view->header())->hide();

#ifdef Q_OS_WIN32
    foreach( QFileInfo drive, QDir::drives () )
    {
        // exclude trailing slash on drive letter
//         m_view->insertTopLevelItem( 0, new CollectionFolder::Item( m_view, drive.filePath().left( 2 ) ) );
    }
#else
    m_dirOperator->dirLister()->openUrl( KUrl( "/" ) );
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

namespace CollectionFolder
{

Item::Item( QTreeWidget *parent, const QString &root )
    : QTreeWidgetItem( parent )
    , m_lister( this )
    , m_url( KUrl::fromPath( root ) )
    , m_listed( false )
    , m_fullyDisabled( false )
{
    setText( 0, root );
    setFlags( Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable );
    m_lister.setDelayedMimeTypes( true );
    //Since we create the "/" checklistitem here, we need to enable it if needed
    if ( CollectionSetup::instance()->m_dirs.contains( root ) )
        setCheckState( 0, Qt::Checked );
    m_lister.setDirOnlyMode( true );
    connect( &m_lister, SIGNAL(newItems( const KFileItemList& )), SLOT(newItems( const KFileItemList& )) );

    setExpanded( true );
}


Item::Item( QTreeWidgetItem *parent, const KUrl &url , bool full_disable /* default=false */ )
    : QTreeWidgetItem( parent )
    , m_lister( this )
    , m_url( url )
    , m_listed( false )
    , m_fullyDisabled( full_disable )
{
    setCheckState( 0, Qt::Unchecked );
    setFlags( Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable );
    setText( 0, url.fileName() );
    m_lister.setDelayedMimeTypes( true );
    m_lister.setDirOnlyMode( true );
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
    if ( !m_listed )
    {
        m_lister.openUrl( m_url, KDirLister::Keep );
        m_listed = true;
    }

    QTreeWidgetItem::setExpanded( b );
}

void
Item::setCheckState( int column, Qt::CheckState checkState )
{
    QStringList &cs_m_dirs = CollectionSetup::instance()->m_dirs;

    if ( isFullyDisabled() )
        return;

    if( CollectionSetup::instance()->recursive() )
    {
        const int children = childCount();
        for( int i = 0; i < children; i++ )
        {
            QTreeWidgetItem *childItem = QTreeWidgetItem::child( i );
            if ( dynamic_cast<Item*>( childItem ) && !dynamic_cast<Item*>( childItem )->isFullyDisabled() )
               childItem->setCheckState( 0, Qt::Checked );
        }
    }
    //If it is disabled, allow us to change its appearance (above code) but not add it
    //to the list of folders (code below)
    if ( isDisabled() )
        return;

    // Update folder list
    QStringList::Iterator it = cs_m_dirs.find( m_url.path() );
    if ( checkState == Qt::Checked )
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
        bool fully_disable=false;

        if ( this->m_url.fileName().isEmpty() && ( (*it).url().fileName()=="proc"
             || (*it).url().fileName()=="dev" || (*it).url().fileName()=="sys" ) )
        {
            fully_disable=true;
        }

        Item *item = new Item( this, (*it).url() , fully_disable || this->isFullyDisabled() );

        if ( !item->isFullyDisabled() )
        {
            if( CollectionSetup::instance()->recursive() && ( checkState( 0 ) == Qt::Checked ) ||
                CollectionSetup::instance()->m_dirs.contains( item->fullPath() ) )
            {
                item->setCheckState( 0, Qt::Checked );
            }
        }

//         item->setPixmap( 0, (*it).pixmap( KIconLoader::SizeSmall ) );
    }
}


// void
// Item::paintCell( QPainter * p, const QColorGroup & cg, int column, int width, int align )
// {
//     bool dirty = false;
//     QStringList &cs_m_dirs = CollectionSetup::instance()->m_dirs;
//
//     // Figure out if a child folder is activated
//     for ( QStringList::ConstIterator iter = cs_m_dirs.constBegin(), end = cs_m_dirs.constEnd();
//             iter != end;
//             ++iter )
//         if ( ( *iter ).startsWith( m_url.path( KUrl::AddTrailingSlash ) ) )
//             if ( *iter != "/" ) // "/" should not match as a child of "/"
//                 dirty = true;
//
//     // Use a different color if this folder has an activated child folder
//     const QFont f = p->font();
//     QColorGroup _cg = cg;
//     if ( dirty )
//     {
//         _cg.setColor( QColorGroup::Text, listView()->colorGroup().link() );
//         QFont font = p->font();
//         font.setBold( !font.bold() );
//         p->setFont( font );
//     }
//
//     Q3CheckListItem::paintCell( p, isDisabled() ? listView()->palette().disabled() : _cg, column, width, align );
//     p->setFont( f );
// }

} //namespace Collection

#include "directorylist.moc"
