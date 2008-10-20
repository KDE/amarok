/******************************************************************************
 * Copyright (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>        *
 *           (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/

#include "CollectionTreeItem.h"

#include "CollectionTreeView.h"
#include "CollectionWidget.h"
#include "amarokconfig.h"

#include <KLocale>

#include <QtAlgorithms>

CollectionTreeItem::CollectionTreeItem( Meta::DataPtr data, CollectionTreeItem *parent )
    : m_data( data )
    , m_parent( parent )
    , m_parentCollection( 0 )
    , m_childrenLoaded( false )
    , m_isVariousArtistsNode( false )
{
    if ( m_parent )
        m_parent->appendChild( this );
}

CollectionTreeItem::CollectionTreeItem( Collection *parentCollection, CollectionTreeItem *parent )
    : m_data( 0 )
    , m_parent( parent )
    , m_parentCollection( parentCollection )
    , m_childrenLoaded( false )
    , m_isVariousArtistsNode( false )
{
    if ( m_parent )
        m_parent->appendChild( this );
}

CollectionTreeItem::CollectionTreeItem( const Meta::DataList &data, CollectionTreeItem *parent )
    : m_data( 0 )
    , m_parent( parent )
    , m_parentCollection( 0 )
    , m_childrenLoaded( true )
    , m_isVariousArtistsNode( true )
{
    if( m_parent )
        m_parent->m_childItems.insert( 0, this );

    foreach( Meta::DataPtr datap, data )
    {
        new CollectionTreeItem( datap, this );
    }
}

CollectionTreeItem::~CollectionTreeItem()
{
    qDeleteAll(m_childItems);
}

void
CollectionTreeItem::appendChild(CollectionTreeItem *child)
{
    m_childItems.append(child);
}

void
CollectionTreeItem::removeChild( int index )
{
    CollectionTreeItem *child = m_childItems[index];
    m_childItems.removeAt( index );
    delete child;
}

CollectionTreeItem*
CollectionTreeItem::child( int row )
{
    if ( row >= 0 && row < m_childItems.count() )
        return m_childItems.value(row);
    else
        return 0;
}

QVariant
CollectionTreeItem::data( int role ) const
{
    if( !m_data.isNull() )
    {
        if( role == Qt::DisplayRole || role == CustomRoles::FilterRole )
        {
            QString name = m_data->prettyName();
            if( AmarokConfig::showTrackNumbers() )
            {
                if( Meta::TrackPtr track = Meta::TrackPtr::dynamicCast(m_data ) )
                {
                    if( !track.isNull() )
                    {
                        if ( track->trackNumber() > 0 )
                            name = QString::number( track->trackNumber() ) + " - " + track->fixedName();
                        else
                            name = track->fixedName();
                    }
                }
            }
            if ( CollectionWidget::instance()->view()->showYears() )
            {
                if( Meta::AlbumPtr album = Meta::AlbumPtr::dynamicCast( m_data ) )
                {
                    if( !album->tracks().isEmpty() )
                    {   
                        Meta::TrackPtr track = album->tracks()[0];
                        QString year;
                        track ?
                            year = track->year() ? track->year()->prettyName() : QString() :
                            year = QString();

                        QString albumName = album->prettyName();
                        if( albumName.isEmpty() )
                            albumName = i18nc( "The Name is not known", "Unknown" );
                        name = ( (year.isEmpty() || year == "0" )? "" : year + " - " ) + albumName;
                    }
                    else
                        name = album->prettyName();
                }
            }
            if( name.isEmpty() )
                return i18nc( "The Name is not known", "Unknown" );
            return name;
        }
        else if ( role == CustomRoles::SortRole )
            return m_data->sortableName();

        return QVariant();
    }
    else if( m_isVariousArtistsNode )
    {
        if( role == Qt::DisplayRole )
            return i18n( "Various Artists" );
        return QVariant();
    }
    else if ( m_parentCollection && ( role == Qt::DisplayRole || role == CustomRoles::FilterRole ) )
        return m_parentCollection->prettyName();

    return QVariant();
}

int
CollectionTreeItem::row() const
{
    if( m_parent )
        return m_parent->m_childItems.indexOf( const_cast<CollectionTreeItem*>(this) );

    return 0;
}

int
CollectionTreeItem::level() const
{
    if( m_parent )
        return m_parent->level() + 1;
    return -1;
}

bool
CollectionTreeItem::isDataItem() const
{
    //return !m_data.isNull();
    //note a various artists node is also a special data node!
    return !m_parentCollection;
}

QueryMaker*
CollectionTreeItem::queryMaker() const
{
    if ( m_parentCollection )
        return m_parentCollection->queryMaker();
        
    CollectionTreeItem *tmp = m_parent;
    while( tmp->isDataItem() )
        tmp = tmp->parent();
    QueryMaker *qm = tmp->parentCollection()->queryMaker();
    return qm;
}

KUrl::List
CollectionTreeItem::urls() const
{
    /*QueryBuilder qb = queryBuilder();
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
    QStringList values = qb.run();
    KUrl::List list;
    foreach( QString s, values ) {
        list += KUrl( s );
    }
    return list;*/
    KUrl::List list;
    return list;
}

bool
CollectionTreeItem::operator<( const CollectionTreeItem& other ) const
{
    if( m_isVariousArtistsNode )
        return true;
    return m_data->sortableName() < other.m_data->sortableName();
}

QList<Meta::TrackPtr>
CollectionTreeItem::descendentTracks()
{
    QList<Meta::TrackPtr> descendentTracks;
    Meta::TrackPtr track;
    if( isDataItem() )
        track = Meta::TrackPtr::dynamicCast( m_data );

    if( !track.isNull() )
        descendentTracks << track;
    else
    {
        foreach( CollectionTreeItem *child, m_childItems )
        {
            descendentTracks << child->descendentTracks();
        }
    }
    return descendentTracks;
}

bool
CollectionTreeItem::allDescendentTracksLoaded() const
{
    Meta::TrackPtr track;
    if( isDataItem() && !( track = Meta::TrackPtr::dynamicCast( m_data ) ).isNull() )
        return true;
    
    if( childrenLoaded() )
    {
        foreach( CollectionTreeItem *item, m_childItems )
            if( !item->allDescendentTracksLoaded() )
                return false;

        return true;
    }
    return false;
}

void
CollectionTreeItem::setChildrenLoaded( bool loaded )
{
    m_childrenLoaded = loaded;
    if ( !loaded )
    {
        qDeleteAll( m_childItems );
        m_childItems.clear();
    }
}

