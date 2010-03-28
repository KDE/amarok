/****************************************************************************************
 * Copyright (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>                  *
 * Copyright (c) 2007-2009 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
 * Copyright (c) 2009 Seb Ruiz <ruiz@kde.org>                                           *
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

#include "CollectionTreeItem.h"
#include "CollectionTreeItemModelBase.h"
#include "core/support/Debug.h"
#include "amarokconfig.h"

#include "core/capabilities/DecoratorCapability.h"

#include <KLocale>

Q_DECLARE_METATYPE( QAction* )
Q_DECLARE_METATYPE( QList<QAction*> )

CollectionTreeItem::CollectionTreeItem( CollectionTreeItemModelBase *model )
: m_data( 0 )
    , m_parent( 0 )
    , m_model( model )
    , m_parentCollection( 0 )
    , m_updateRequired( false )
    , m_trackCount( -1 )
    , m_type( Root )
    //, m_name( "Root" )
    , m_isCounting( false )
{
}

CollectionTreeItem::CollectionTreeItem( Meta::DataPtr data, CollectionTreeItem *parent, CollectionTreeItemModelBase *model  )
    : m_data( data )
    , m_parent( parent )
    , m_model( model )
    , m_parentCollection( 0 )
    , m_updateRequired( true )
    , m_trackCount( -1 )
    , m_type( Data )
    //, m_name( data ? data->name() : "NullData" )
    , m_isCounting( false )
{
    if ( m_parent )
        m_parent->appendChild( this );
}

CollectionTreeItem::CollectionTreeItem( Collections::Collection *parentCollection, CollectionTreeItem *parent, CollectionTreeItemModelBase *model  )
    : m_data( 0 )
    , m_parent( parent )
    , m_model( model )
    , m_parentCollection( parentCollection )
    , m_updateRequired( true )
    , m_trackCount( -1 )
    , m_type( Collection )
    //, m_name( parentCollection ? parentCollection->collectionId() : "NullColl" )
    , m_isCounting( false )
{
    if ( m_parent )
        m_parent->appendChild( this );

    connect( parentCollection, SIGNAL( updated() ), SLOT( collectionUpdated() ) );
}

CollectionTreeItem::CollectionTreeItem( const Meta::DataList &data, CollectionTreeItem *parent, CollectionTreeItemModelBase *model  )
    : m_data( 0 )
    , m_parent( parent )
    , m_model( model )
    , m_parentCollection( 0 )
    , m_updateRequired( false )  //the node already has all children
    , m_trackCount( -1 )
    , m_type( VariousArtist )
    //, m_name("VA")
    , m_isCounting( false )
{
    DEBUG_BLOCK
    if( m_parent )
        m_parent->m_childItems.insert( 0, this );

    foreach( Meta::DataPtr datap, data )
        new CollectionTreeItem( datap, this, m_model );
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
    child->prepareForRemoval();
    child->deleteLater();
}

void
CollectionTreeItem::prepareForRemoval()
{
    m_parent = 0;
    m_model->itemAboutToBeDeleted( this );
    foreach( CollectionTreeItem *item, m_childItems )
    {
        item->prepareForRemoval();
    }
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
    else if( isVariousArtistItem() )
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

                Collections::QueryMaker *qm = m_parentCollection->queryMaker();
                connect( qm, SIGNAL( newResultReady(QString, QStringList) ), SLOT( tracksCounted(QString, QStringList) ) );

                qm->setAutoDelete( true )
                  ->setQueryType( Collections::QueryMaker::Custom )
                  ->addReturnFunction( Collections::QueryMaker::Count, Meta::valUrl )
                  ->run();

                return counting;
            }

            return i18np( "1 track", "%1 tracks", m_trackCount );
        }
        else if( role == CustomRoles::HasCapacityRole )
        {
            return m_parentCollection->hasCapacity();
        }
        else if( role == CustomRoles::UsedCapacityRole )
        {
            if( m_parentCollection->hasCapacity() && m_parentCollection->totalCapacity() > 0 )
                return m_parentCollection->usedCapacity() * 100 / m_parentCollection->totalCapacity();
        }
        else if( role == CustomRoles::DecoratorRoleCount )
        {
            return decoratorActions().size();
        }
        else if( role == CustomRoles::DecoratorRole )
        {
            QVariant v;
            v.setValue( decoratorActions() );
            return v;
        }
    }

    return QVariant();
}

QList<QAction*>
CollectionTreeItem::decoratorActions() const
{
    QList<QAction*> decoratorActions;
    Capabilities::DecoratorCapability *dc = m_parentCollection->create<Capabilities::DecoratorCapability>();
    if( dc )
    {
        decoratorActions = dc->decoratorActions();
        delete dc;
    }
    return decoratorActions;
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
    return m_type == Data || m_type == VariousArtist;
}

bool
CollectionTreeItem::isVariousArtistItem() const
{
    return m_type == VariousArtist;
}

bool
CollectionTreeItem::isAlbumItem() const
{
    return isDataItem() && !isVariousArtistItem() && !Meta::AlbumPtr::dynamicCast( m_data ).isNull();
}

bool
CollectionTreeItem::isTrackItem() const
{
    return isDataItem() && !isVariousArtistItem() && !Meta::TrackPtr::dynamicCast( m_data ).isNull();
}

Collections::QueryMaker*
CollectionTreeItem::queryMaker() const
{
    if ( m_parentCollection )
        return m_parentCollection->queryMaker();

    CollectionTreeItem *tmp = m_parent;
    while( tmp->isDataItem() )
        tmp = tmp->parent();
    Collections::QueryMaker *qm = 0;
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
    if( isVariousArtistItem() )
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
    if( isTrackItem() )
        return true;

    if( requiresUpdate() )
        return false;

    foreach( CollectionTreeItem *item, m_childItems )
    {
        if( !item->allDescendentTracksLoaded() )
            return false;
    }

    return true;
}

void
CollectionTreeItem::setRequiresUpdate( bool updateRequired )
{
    m_updateRequired = updateRequired;
}

bool
CollectionTreeItem::requiresUpdate() const
{
    return m_updateRequired;
}

CollectionTreeItem::Type
CollectionTreeItem::type() const
{
    return m_type;
}

QList<CollectionTreeItem*>
CollectionTreeItem::children() const
{
    return m_childItems;
}

#include "CollectionTreeItem.moc"

