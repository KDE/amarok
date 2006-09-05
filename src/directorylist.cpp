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

#include "amarokconfig.h"
#include "directorylist.h"
#include "mountpointmanager.h"

#include <kfileitem.h>
#include <klocale.h>

#include <qfile.h>
#include <qlabel.h>
#include <qpainter.h>
#include <qtooltip.h>


CollectionSetup* CollectionSetup::s_instance;


CollectionSetup::CollectionSetup( QWidget *parent )
        : QVBox( parent, "CollectionSetup" )
{
    s_instance = this;

    (new QLabel( i18n(
        "These folders will be scanned for "
        "media to make up your collection:"), this ))->setAlignment( Qt::WordBreak );

    m_view = new QFixedListView( this );
    m_recursive = new QCheckBox( i18n("&Scan folders recursively"), this );
    m_monitor   = new QCheckBox( i18n("&Watch folders for changes"), this );

    QToolTip::add( m_recursive, i18n( "If selected, Amarok will read all subfolders." ) );
    QToolTip::add( m_monitor,   i18n( "If selected, folders will automatically get rescanned when the content is modified, e.g. when a new file was added." ) );

    // Read config values
    //we have to detect if this is the actual first run and not get the collectionFolders in that case
    //there won't be any anyway and accessing them creates a Sqlite database, even if the user wants to
    //use another database
    //bug 131719 131724
    if( !Amarok::config()->readBoolEntry( "First Run", true ) )
        m_dirs = MountPointManager::instance()->collectionFolders();

    m_recursive->setChecked( AmarokConfig::scanRecursively() );
    m_monitor->setChecked( AmarokConfig::monitorChanges() );

    m_view->addColumn( QString::null );
    m_view->setRootIsDecorated( true );
    m_view->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    m_view->setResizeMode( QListView::LastColumn );

    reinterpret_cast<QWidget*>(m_view->header())->hide();
    new Collection::Item( m_view );

    setSpacing( 6 );
}


void
CollectionSetup::writeConfig()
{
    //If we are in recursive mode then we don't need to store the names of the
    //subdirectories of the selected directories
    if ( recursive() )
    {
        for ( QStringList::iterator it=m_dirs.begin(); it!=m_dirs.end(); ++it )
        {
            QStringList::iterator jt=m_dirs.begin();
            while ( jt!=m_dirs.end() )
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
                    jt = m_dirs.remove( jt );
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

namespace Collection {

Item::Item( QListView *parent )
    : QCheckListItem( parent, "/", QCheckListItem::CheckBox  )
    , m_lister( true )
    , m_url( "file:/" )
    , m_listed( false )
    , m_fullyDisabled( false )
{
    //Since we create the "/" checklistitem here, we need to enable it if needed
    if ( CollectionSetup::instance()->m_dirs.contains( "/" ) )
        static_cast<QCheckListItem*>( this )->setOn(true);
    m_lister.setDirOnlyMode( true );
    connect( &m_lister, SIGNAL(newItems( const KFileItemList& )), SLOT(newItems( const KFileItemList& )) );
    setOpen( true );
    setVisible( true );
}


Item::Item( QListViewItem *parent, const KURL &url , bool full_disable /* default=false */ )
    : QCheckListItem( parent, url.fileName(), QCheckListItem::CheckBox )
    , m_lister( true )
    , m_url( url )
    , m_listed( false )
    , m_fullyDisabled( full_disable )
{
    m_lister.setDirOnlyMode( true );
    setExpandable( true );
    connect( &m_lister, SIGNAL(newItems( const KFileItemList& )), SLOT(newItems( const KFileItemList& )) );
    connect( &m_lister, SIGNAL(completed()), SLOT(completed()) );
    connect( &m_lister, SIGNAL(canceled()), SLOT(completed()) );
}


QString
Item::fullPath() const
{
    QString path;

    for( const QListViewItem *item = this; item != listView()->firstChild(); item = item->parent() )
    {
        path.prepend( item->text( 0 ) );
        path.prepend( '/' );
    }

    return path;
}


void
Item::setOpen( bool b )
{
    if ( !m_listed )
    {
        m_lister.openURL( m_url, true );
        m_listed = true;
    }

    QListViewItem::setOpen( b );
}


void
Item::stateChange( bool b )
{
    QStringList &cs_m_dirs = CollectionSetup::instance()->m_dirs;

    if ( isFullyDisabled() )
        return;

    if( CollectionSetup::instance()->recursive() )
        for( QListViewItem *item = firstChild(); item; item = item->nextSibling() )
            if ( dynamic_cast<Item*>( item ) && !dynamic_cast<Item*>( item )->isFullyDisabled() )
               static_cast<QCheckListItem*>(item)->QCheckListItem::setOn( b );

    //If it is disabled, allow us to change its appearance (above code) but not add it
    //to the list of folders (code below)
    if ( isDisabled() )
        return;

    // Update folder list
    QStringList::Iterator it = cs_m_dirs.find( m_url.path() );
    if ( isOn() ) {
        if ( it == cs_m_dirs.end() )
            cs_m_dirs << m_url.path();

        // Deselect subdirectories if we are in recursive mode as they are redundant
        if ( CollectionSetup::instance()->recursive() )
        {
            QStringList::Iterator diriter = cs_m_dirs.begin();
            while ( diriter != cs_m_dirs.end() )
            {
                // Since the dir "/" starts with '/', we need a hack to stop it removing
                // itself (it being the only path with a trailing '/')
                if ( (*diriter).startsWith( m_url.path(1) ) && *diriter != "/" )
                    diriter = cs_m_dirs.erase(diriter);
                else
                    ++diriter;
            }
        }
    }
    else {
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
        while ( diriter != cs_m_dirs.end() )
        {
            if ( (*diriter).startsWith( m_url.path(1) ) )   //path(1) adds a trailing '/'
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

    // Redraw parent items
    listView()->triggerUpdate();
}


void
Item::activate()
{
    if( !isDisabled() )
        QCheckListItem::activate();
}


void
Item::newItems( const KFileItemList &list ) //SLOT
{
    for( KFileItemListIterator it( list ); *it; ++it )
    {
        //Fully disable (always appears off and grayed-out) if it is "/proc", "/sys" or
        //"/dev" or one of their children. This is because we will never scan them, so we
        //might as well show that.
        //These match up with the skipped dirs in CollectionScanner::readDir.
        bool fully_disable=false;

        if ( this->m_url.fileName().isEmpty() && ( ( *it )->url().fileName()=="proc"
             || ( *it )->url().fileName()=="dev" || ( *it )->url().fileName()=="sys" ) )
        {
            fully_disable=true;
        }

        Item *item = new Item( this, (*it)->url() , fully_disable || this->isFullyDisabled() );

        if ( !item->isFullyDisabled() )
        {
            if( CollectionSetup::instance()->recursive() && isOn() ||
                CollectionSetup::instance()->m_dirs.contains( item->fullPath() ) )
            {
                item->setOn( true );
            }
        }

        item->setPixmap( 0, (*it)->pixmap( KIcon::SizeSmall ) );
    }
}


void
Item::paintCell( QPainter * p, const QColorGroup & cg, int column, int width, int align )
{
    bool dirty = false;
    QStringList &cs_m_dirs = CollectionSetup::instance()->m_dirs;

    // Figure out if a child folder is activated
    for ( QStringList::const_iterator iter = cs_m_dirs.begin(); iter != cs_m_dirs.end();
            ++iter )
        if ( ( *iter ).startsWith( m_url.path(1) ) )
            if ( *iter != "/" ) // "/" should not match as a child of "/"
                dirty = true;

    // Use a different color if this folder has an activated child folder
    const QFont f = p->font();
    QColorGroup _cg = cg;
    if ( dirty )
    {
        _cg.setColor( QColorGroup::Text, listView()->colorGroup().link() );
        QFont font = p->font();
        font.setBold( !font.bold() );
        p->setFont( font );
    }

    QCheckListItem::paintCell( p, isDisabled() ? listView()->palette().disabled() : _cg, column, width, align );
    p->setFont( f );
}

} //namespace Collection

#include "directorylist.moc"
