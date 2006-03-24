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
#include <kfileitem.h>
#include <klocale.h>
#include <qfile.h>
#include <qlabel.h>
#include <qtooltip.h>


CollectionSetup* CollectionSetup::s_instance;


CollectionSetup::CollectionSetup( QWidget *parent )
        : QVBox( parent, "CollectionSetup" )
{
    s_instance = this;

    (new QLabel( i18n(
        "These folders will be scanned for "
        "media to make up your collection."), this ))->setAlignment( Qt::WordBreak );

    m_view = new QFixedListView( this );
    m_recursive = new QCheckBox( i18n("&Scan folders recursively"), this );
    m_monitor   = new QCheckBox( i18n("&Watch folders for changes"), this );
    m_playlists = new QCheckBox( i18n("&Import playlists"), this );

    QToolTip::add( m_recursive, i18n( "If selected, amaroK will read all subfolders." ) );
    QToolTip::add( m_monitor,   i18n( "If selected, folders will automatically get rescanned when the content is modified, e.g. when a new file was added." ) );
    QToolTip::add( m_playlists, i18n( "If selected, playlist files (.m3u) will automatically be added to the Playlist-Browser." ) );

    // Read config values
    m_dirs = AmarokConfig::collectionFolders();
    m_recursive->setChecked( AmarokConfig::scanRecursively() );
    m_monitor->setChecked( AmarokConfig::monitorChanges() );
    m_playlists->setChecked( AmarokConfig::importPlaylists() );

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
    AmarokConfig::setCollectionFolders( m_dirs );
    AmarokConfig::setScanRecursively( recursive() );
    AmarokConfig::setMonitorChanges( monitor() );
    AmarokConfig::setImportPlaylists( importPlaylists() );
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
{
    m_lister.setDirOnlyMode( true );
    connect( &m_lister, SIGNAL(newItems( const KFileItemList& )), SLOT(newItems( const KFileItemList& )) );
    setOpen( true );
    setVisible( true );
}


Item::Item( QListViewItem *parent, const KURL &url )
    : QCheckListItem( parent, url.fileName(), QCheckListItem::CheckBox  )
    , m_lister( true )
    , m_url( url )
    , m_listed( false )
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

    if( CollectionSetup::instance()->recursive() )
        for( QListViewItem *item = firstChild(); item; item = item->nextSibling() )
            static_cast<QCheckListItem*>(item)->QCheckListItem::setOn( b );

    // Update folder list
    QStringList::Iterator it = cs_m_dirs.find( m_url.path() );
    if ( isOn() ) {
        if ( it == cs_m_dirs.end() )
            cs_m_dirs << m_url.path();
    }
    else {
        //Deselect item and recurse through children but only deselect children if they
        //do not exist unless we are in recursive mode (where no children should be
        //selected if the parent is being unselected)
        //Note this does not do anything to the checkboxes, but they should be doing
        //the same thing as we are (hopefully)
        cs_m_dirs.erase( it );
        QStringList::Iterator diriter = cs_m_dirs.begin();
        while ( diriter != cs_m_dirs.end() )
        {
            if ( (*diriter).startsWith( m_url.path() + '/' ) )
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
        Item *item = new Item( this, (*it)->url() );

        item->setOn( CollectionSetup::instance()->recursive() && isOn() ||
                     CollectionSetup::instance()->m_dirs.contains( item->fullPath() ) );

        item->setPixmap( 0, (*it)->pixmap( KIcon::SizeSmall ) );
    }
}


void
Item::paintCell( QPainter * p, const QColorGroup & cg, int column, int width, int align )
{
    bool dirty = false;

    // Figure out if a child folder is activated
    for ( uint i = 0; i < CollectionSetup::instance()->m_dirs.count(); i++ )
        if ( CollectionSetup::instance()->m_dirs[i].startsWith( m_url.path() ) )
            dirty = true;

    // Use a different color if this folder has an activated child folder
    QColorGroup _cg = cg;
    if ( dirty ) _cg.setColor( QColorGroup::Text, listView()->colorGroup().link() );

    QCheckListItem::paintCell( p, isDisabled() ? listView()->palette().disabled() : _cg, column, width, align );
}

} //namespace Collection

#include "directorylist.moc"
