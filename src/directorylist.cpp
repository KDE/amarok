/***************************************************************************
                         directorylist.cpp  -  description
                            -------------------
   begin                : Tue Feb 4 2003
   copyright            : (C) 2003 by Scott Wheeler
                          (C) 2004 Max Howell
   email                : wheeler@kde.org
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
#include <kfileitem.h>
#include <klocale.h>
#include <qlabel.h>
#include <qlistview.h>

QStringList CollectionSetup::s_dirs;
QCheckBox  *CollectionSetup::s_recursive = 0;
QCheckBox  *CollectionSetup::s_monitor = 0;

using Collection::Item;


CollectionSetup::CollectionSetup( QWidget *parent )
    : QVBox( parent )
{
    (new QLabel( i18n(
        "These folders will be scanned for "
        "media to make up your collection."), this ))->setAlignment( Qt::WordBreak );

    m_view = new QListView( this );

    s_recursive = new QCheckBox( i18n("&Scan folders recursively"), this );
    s_monitor   = new QCheckBox( i18n("&Monitor changes"), this );

    s_recursive->setChecked( true );
    s_monitor->setChecked( true );

    m_view->addColumn( QString::null );
    m_view->setRootIsDecorated( true );
    reinterpret_cast<QWidget*>(m_view->header())->hide();
    m_view->setResizeMode( QListView::LastColumn );
    new Item( m_view );

    setSpacing( 6 );
}


QStringList
CollectionSetup::dirs() const
{
    QStringList list;

    for( QListViewItem *item = m_view->firstChild(); item; item = item->itemBelow() )
    {
        #define item static_cast<Item*>(item)
        if( !item->isOn() || item->isDisabled() )
           continue;

        list += item->fullPath();
        #undef item
    }

    return list;
}


inline QString
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

inline void
Item::newItems( const KFileItemList &list )
{
    for( KFileItemListIterator it( list ); *it; ++it )
    {
        Item *item = new Item( this, (*it)->url() );

        item->setOn( CollectionSetup::recursive() && isOn() || CollectionSetup::s_dirs.contains( item->fullPath() ) );
        item->setPixmap( 0, (*it)->pixmap( KIcon::SizeSmall ) );
    }
}


inline void
Item::paintCell( QPainter * p, const QColorGroup & cg, int column, int width, int align )
{
    bool dirty = isOn();

    // Figure out if a child folder is activated
    for ( uint i = 0; i < CollectionSetup::s_dirs.count(); i++ )
        if ( CollectionSetup::s_dirs[i].startsWith( m_url.path() ) )
            dirty = true;

    // Use a different color if this folder has an activated child folder
    QColorGroup _cg = cg;
    if ( dirty ) _cg.setColor( QColorGroup::Text, Qt::blue );

    QCheckListItem::paintCell( p, isDisabled() ? listView()->palette().disabled() : _cg, column, width, align );
}


#include "directorylist.moc"
