/****************************************************************************************
 * Copyright (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>                  *
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2009 Seb Ruiz <ruiz@kde.org>                                           *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "CollectionTreeView.h"
#include "Debug.h"
#include "amarokconfig.h"

#include <KLocale>


CollectionTreeItem::CollectionTreeItem( Meta::DataPtr data, CollectionTreeItem *parent )
    : m_data( data )
    , m_parent( parent )
    , m_parentCollection( 0 )
    , m_childrenLoaded( false )
    , m_isVariousArtistsNode( false )
    , m_trackCount( -1 )
    , m_isCounting( false )
{
    if ( m_parent )
        m_parent->appendChild( this );
}

CollectionTreeItem::CollectionTreeItem( Amarok::Collection *parentCollection, CollectionTreeItem *parent )
    : m_data( 0 )
    , m_parent( parent )
    , m_parentCollection( parentCollection )
    , m_childrenLoaded( false )
    , m_isVariousArtistsNode( false )
    , m_trackCount( -1 )
    , m_isCounting( false )
{
    if ( m_parent )
        m_parent->appendChild( this );

    connect( parentCollection, SIGNAL( updated() ), SLOT( collectionUpdated() ) );
}

CollectionTreeItem::CollectionTreeItem( const Meta::DataList &data, CollectionTreeItem *parent )
    : m_data( 0 )
    , m_parent( parent )
    , m_parentCollection( 0 )
    , m_childrenLoaded( true )
    , m_isVariousArtistsNode( true )
    , m_trackCount( -1 )
    , m_isCounting( false )
{
    if( m_parent )
        m_parent->m_childItems.insert( 0, this );

    foreach( Meta::DataPtr datap, data )
        new CollectionTreeItem( datap, this );
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
    return 0;
}

QString
CollectionTreeItem::albumYear() const
{
    QString year;
    if( Meta::AlbumPtr album = Meta::AlbumPtr::dynamicCast( m_data ) )
    {
        if( !album->tracks().isEmpty() )
        {   
            Meta::TrackPtr track = album->tracks()[0];
            if( track && track->year() )
                year = track->year()->prettyName();
        }
    }
    if( year == "0" )
        year.clear();
    return year;
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
                if( Meta::TrackPtr track = Meta::TrackPtr::dynamicCast( m_data ) )
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
            
            // Check empty after track logic and before album logic
            if( name.isEmpty() )
                name = i18nc( "The Name is not known", "Unknown" );
            
            if( AmarokConfig::showYears() )
            {
                QString year = albumYear();
                if( !year.isEmpty() )
                    name = year + " - " + name;
            }
            return name;
        }
        else if( role == CustomRoles::SortRole )
            return m_data->sortableName();

        return QVariant();
    }
    else if( m_isVariousArtistsNode )
    {
        if( role == Qt::DisplayRole )
            return i18n( "Various Artists" );
        return QVariant();
    }
    else if( m_parentCollection )
    {
        if ( m_parentCollection && ( role == Qt::DisplayRole || role == CustomRoles::FilterRole ) )
            return m_parentCollection->prettyName();
        else if( role == Qt::DecorationRole )
            return m_parentCollection->icon();
        else if( role == CustomRoles::ByLineRole )
        {
            static const QString counting = i18n( "Counting" );
            if( m_isCounting )
                  return counting;
            if( m_trackCount < 0 )
            {
                m_isCounting = true;

                QueryMaker *qm = m_parentCollection->queryMaker();
                connect( qm, SIGNAL( newResultReady(QString, QStringList) ), SLOT( tracksCounted(QString, QStringList) ) );

                qm->setAutoDelete( true )
                  ->setQueryType( QueryMaker::Custom )
                  ->addReturnFunction( QueryMaker::Count, Meta::valUrl )
                  ->run();

                return counting;
            }

            return i18np( "1 track", "%1 tracks", m_trackCount );
        }
        else if( role == CustomRoles::HasCapacity )
        {
            return false;//m_parentCollection->hasCapacity();
        }
        else if( role == CustomRoles::UsedCapacity )
        {
            //if( m_parentCollection->hasCapacity() && m_parentCollection->totalCapacity() > 0 )
            //    return m_parentCollection->usedCapacity() * 100 / m_parentCollection->totalCapacity();
            return 0;
        }
    }

    return QVariant();
}

void
CollectionTreeItem::tracksCounted( QString collectionId, QStringList res )
{
    Q_UNUSED( collectionId );
    if( !res.isEmpty() )
        m_trackCount = res.first().toInt();
    else
        m_trackCount = 0;
    m_isCounting = false;
    emit dataUpdated();
}

void
CollectionTreeItem::collectionUpdated()
{
    m_trackCount = -1;
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

bool
CollectionTreeItem::isAlbumItem() const
{
    return isDataItem() && !Meta::AlbumPtr::dynamicCast( m_data ).isNull();
}

bool
CollectionTreeItem::isTrackItem() const
{
    return isDataItem() && !Meta::TrackPtr::dynamicCast( m_data ).isNull();
}

QueryMaker*
CollectionTreeItem::queryMaker() const
{
    if ( m_parentCollection )
        return m_parentCollection->queryMaker();
        
    CollectionTreeItem *tmp = m_parent;
    while( tmp->isDataItem() )
        tmp = tmp->parent();
    QueryMaker *qm = 0;
    if( tmp->parentCollection() )
        qm = tmp->parentCollection()->queryMaker();
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
            descendentTracks << child->descendentTracks();
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
        {
            if( !item->allDescendentTracksLoaded() )
                return false;
        }

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

#include "CollectionTreeItem.moc"

