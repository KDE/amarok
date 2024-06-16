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

#include "amarokconfig.h"
#include "browsers/CollectionTreeItemModelBase.h"
#include "core/capabilities/ActionsCapability.h"
#include "core/meta/Meta.h"
#include "core/support/Debug.h"
#include "widgets/PrettyTreeRoles.h"

#include <QIcon>
#include <KLocalizedString>

Q_DECLARE_METATYPE( QAction* )
Q_DECLARE_METATYPE( QList<QAction*> )

CollectionTreeItem::CollectionTreeItem( CollectionTreeItemModelBase *model )
: m_data( nullptr )
    , m_parent( nullptr )
    , m_model( model )
    , m_parentCollection( nullptr )
    , m_updateRequired( false )
    , m_trackCount( -1 )
    , m_type( Root )
    //, m_name( "Root" )
    , m_isCounting( false )
{
}

CollectionTreeItem::CollectionTreeItem( const Meta::DataPtr &data, CollectionTreeItem *parent, CollectionTreeItemModelBase *model  )
    : m_data( data )
    , m_parent( parent )
    , m_model( model )
    , m_parentCollection( nullptr )
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
    : m_data( nullptr )
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

    connect( parentCollection, &Collections::Collection::updated, this, &CollectionTreeItem::collectionUpdated );
}

CollectionTreeItem::CollectionTreeItem( Type type, const Meta::DataList &data, CollectionTreeItem *parent, CollectionTreeItemModelBase *model  )
    : m_data( nullptr )
    , m_parent( parent )
    , m_model( model )
    , m_parentCollection( nullptr )
    , m_updateRequired( false )  //the node already has all children
    , m_trackCount( -1 )
    , m_type( type )
    , m_isCounting( false )
{
    if( m_parent )
        m_parent->m_childItems.insert( 0, this );

    for( Meta::DataPtr datap : data )
        new CollectionTreeItem( datap, this, m_model );
}

CollectionTreeItem::~CollectionTreeItem()
{
    qDeleteAll( m_childItems );
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
    m_parent = nullptr;
    m_model->itemAboutToBeDeleted( this );
    for( CollectionTreeItem *item : m_childItems )
    {
        item->prepareForRemoval();
    }
}

CollectionTreeItem*
CollectionTreeItem::child( int row )
{
    return m_childItems.value( row );
}

QVariant
CollectionTreeItem::data( int role ) const
{
    if( isNoLabelItem() )
    {
        switch( role )
        {
        case Qt::DisplayRole:
            return i18nc( "No labels are assigned to the given item are any of its subitems", "No Labels" );
        case Qt::DecorationRole:
            return QIcon::fromTheme( QStringLiteral("label-amarok") );
        }
        return QVariant();
    }
    else if( m_parentCollection )
    {
        static const QString counting = i18n( "Counting..." );
        switch( role )
        {
        case Qt::DisplayRole:
        case PrettyTreeRoles::FilterRole:
        case PrettyTreeRoles::SortRole:
            return m_parentCollection->prettyName();
        case Qt::DecorationRole:
            return m_parentCollection->icon();
        case PrettyTreeRoles::ByLineRole:
            if( m_isCounting )
                return counting;
            if( m_trackCount < 0 )
            {
                m_isCounting = true;

                Collections::QueryMaker *qm = m_parentCollection->queryMaker();
                connect( qm, &Collections::QueryMaker::newResultReady,
                         this, &CollectionTreeItem::tracksCounted );

                qm->setAutoDelete( true )
                  ->setQueryType( Collections::QueryMaker::Custom )
                  ->addReturnFunction( Collections::QueryMaker::Count, Meta::valUrl )
                  ->run();

                return counting;
            }
            return i18np( "1 track", "%1 tracks", m_trackCount );
        case PrettyTreeRoles::HasCapacityRole:
            return m_parentCollection->hasCapacity();
        case PrettyTreeRoles::UsedCapacityRole:
            return m_parentCollection->usedCapacity();
        case PrettyTreeRoles::TotalCapacityRole:
            return m_parentCollection->totalCapacity();
        case PrettyTreeRoles::DecoratorRoleCount:
            return decoratorActions().size();
        case PrettyTreeRoles::DecoratorRole:
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
    QScopedPointer<Capabilities::ActionsCapability> dc( m_parentCollection->create<Capabilities::ActionsCapability>() );
    if( dc )
        decoratorActions = dc->actions();
    return decoratorActions;
}

void
CollectionTreeItem::tracksCounted( QStringList res )
{
    if( !res.isEmpty() )
        m_trackCount = res.first().toInt();
    else
        m_trackCount = 0;
    m_isCounting = false;
    Q_EMIT dataUpdated();
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
    {
        const QList<CollectionTreeItem*> &children = m_parent->m_childItems;
        if( !children.isEmpty() && children.contains( const_cast<CollectionTreeItem*>(this) ) )
            return children.indexOf( const_cast<CollectionTreeItem*>(this) );
        else
            return -1;
    }
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
    return m_type == Data;
}

bool
CollectionTreeItem::isVariousArtistItem() const
{
    return m_type == CollectionTreeItem::VariousArtist;
}

bool
CollectionTreeItem::isNoLabelItem() const
{
    return m_type == CollectionTreeItem::NoLabel;
}

bool
CollectionTreeItem::isAlbumItem() const
{
    return m_type == Data && !Meta::AlbumPtr::dynamicCast( m_data ).isNull();
}

bool
CollectionTreeItem::isArtistItem() const
{
    return m_type == Data && !Meta::ArtistPtr::dynamicCast( m_data ).isNull();
}

bool
CollectionTreeItem::isTrackItem() const
{
    return m_type == Data && !Meta::TrackPtr::dynamicCast( m_data ).isNull();
}

Collections::QueryMaker*
CollectionTreeItem::queryMaker() const
{
    Collections::Collection* coll = parentCollection();
    if( coll )
        return coll->queryMaker();
    return nullptr;
}

void
CollectionTreeItem::addMatch( Collections::QueryMaker *qm, CategoryId::CatMenuId levelCategory ) const
{
    if( !qm )
        return;

    if( isVariousArtistItem() )
        qm->setAlbumQueryMode( Collections::QueryMaker::OnlyCompilations );
    if( isNoLabelItem() )
        qm->setLabelQueryMode( Collections::QueryMaker::OnlyWithoutLabels );
    else if( Meta::TrackPtr track = Meta::TrackPtr::dynamicCast( m_data ) )
        qm->addMatch( track );
    else if( Meta::ArtistPtr artist = Meta::ArtistPtr::dynamicCast( m_data ) )
    {
        Collections::QueryMaker::ArtistMatchBehaviour behaviour =
                ( levelCategory == CategoryId::AlbumArtist ) ? Collections::QueryMaker::AlbumArtists :
                                                               Collections::QueryMaker::TrackArtists;
        qm->addMatch( artist, behaviour );
    }
    else if( Meta::AlbumPtr album = Meta::AlbumPtr::dynamicCast( m_data ) )
        qm->addMatch( album );
    else if( Meta::ComposerPtr composer = Meta::ComposerPtr::dynamicCast( m_data ) )
        qm->addMatch( composer );
    else if( Meta::GenrePtr genre = Meta::GenrePtr::dynamicCast( m_data ) )
        qm->addMatch( genre );
    else if( Meta::YearPtr year = Meta::YearPtr::dynamicCast( m_data ) )
        qm->addMatch( year );
    else if( Meta::LabelPtr label = Meta::LabelPtr::dynamicCast( m_data ) )
        qm->addMatch( label );
}

bool
CollectionTreeItem::operator<( const CollectionTreeItem& other ) const
{
    if( isVariousArtistItem() )
        return true;
    return m_data->sortableName() < other.m_data->sortableName();
}

const Meta::DataPtr
CollectionTreeItem::data() const
{
    return m_data;
}

Meta::TrackList
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
        for( CollectionTreeItem *child : m_childItems )
            descendentTracks << child->descendentTracks();
    }
    return descendentTracks;
}

bool
CollectionTreeItem::allDescendentTracksLoaded() const
{
    if( isTrackItem() )
        return true;

    if( requiresUpdate() )
        return false;

    for( const CollectionTreeItem *item : m_childItems )
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


