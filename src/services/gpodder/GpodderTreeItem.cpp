/****************************************************************************************
 * Copyright (c) 2011 Stefan Derkits <stefan@derkits.at>                                *
 * Copyright (c) 2011 Christian Wagner <christian.wagner86@gmx.at>                      *
 * Copyright (c) 2011 Felix Winter <ixos01@gmail.com>                                   *
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

#include "GpodderTreeItem.h"

#include "GpodderPodcastTreeItem.h"
#include "GpodderServiceModel.h"
#include "GpodderTagTreeItem.h"

GpodderTreeItem::GpodderTreeItem( GpodderTreeItem *parent, const QString &name )
    : QObject( parent )
    , m_parentItem( parent )
    , m_name( name )
    , m_hasChildren( false )
{
}

GpodderTreeItem::~GpodderTreeItem()
{
    qDeleteAll( m_childItems );
}

void
GpodderTreeItem::appendChild( GpodderTreeItem *item )
{
    m_childItems.append( item );
}

GpodderTreeItem *
GpodderTreeItem::child( int row )
{
    return m_childItems.value( row );
}

bool
GpodderTreeItem::hasChildren() const
{
    return m_hasChildren;
}

void
GpodderTreeItem::setHasChildren( bool hasChildren )
{
    m_hasChildren = hasChildren;
}

int
GpodderTreeItem::childCount() const
{
    return m_childItems.count();
}

GpodderTreeItem *
GpodderTreeItem::parent() const
{
    return m_parentItem;
}

QVariant
GpodderTreeItem::displayData() const
{
    return m_name;
}

bool
GpodderTreeItem::isRoot() const
{
    return ( m_parentItem == nullptr );
}

void
GpodderTreeItem::appendTags( mygpo::TagListPtr tags )
{
    for( const auto &tag : tags->list() )
    {
        GpodderTagTreeItem *treeItem = new GpodderTagTreeItem( tag, this );
        appendChild( treeItem );
    }
}

void
GpodderTreeItem::appendPodcasts( mygpo::PodcastListPtr podcasts )
{
    for( const auto &podcast : podcasts->list() )
    {
        appendChild( new GpodderPodcastTreeItem( podcast, this ) );
    }
}
