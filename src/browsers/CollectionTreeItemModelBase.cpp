/****************************************************************************************
 * Copyright (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>                  *
 * Copyright (c) 2007-2009 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#define DEBUG_PREFIX "CollectionTreeItemModelBase"

#include "CollectionTreeItemModelBase.h"

#include "AmarokMimeData.h"
#include "FileType.h"
#include "SvgHandler.h"
#include "amarokconfig.h"
#include "browsers/CollectionTreeItem.h"
#include "core/collections/Collection.h"
#include "core/collections/QueryMaker.h"
#include "core/meta/TrackEditor.h"
#include "core/meta/support/MetaConstants.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/TextualQueryFilter.h"
#include "widgets/PrettyTreeRoles.h"

#include <KGlobalSettings>
#include <QIcon>
#include <KIconLoader>
#include <KLocale>
#include <KStandardDirs>

#include <QApplication>
#include <QStyle>
#include <QPixmap>
#include <QTimeLine>
#include <QTimer>

using namespace Meta;


inline uint qHash( const Meta::DataPtr &data )
{
    return qHash( data.data() );
}

/**
 * This set determines which collection browser levels should have shown Various Artists
 * item under them. AlbumArtist is certain, (Track)Artist is questionable.
 */
static const QSet<CategoryId::CatMenuId> variousArtistCategories =
        QSet<CategoryId::CatMenuId>() << CategoryId::AlbumArtist;

CollectionTreeItemModelBase::CollectionTreeItemModelBase( )
    : QAbstractItemModel()
    , m_rootItem( 0 )
    , m_animFrame( 0 )
    , m_loading1( QPixmap( KStandardDirs::locate("data", "amarok/images/loading1.png" ) ) )
    , m_loading2( QPixmap( KStandardDirs::locate("data", "amarok/images/loading2.png" ) ) )
    , m_currentAnimPixmap( m_loading1 )
    , m_autoExpand( false )
{
    m_timeLine = new QTimeLine( 10000, this );
    m_timeLine->setFrameRange( 0, 20 );
    m_timeLine->setLoopCount ( 0 );
    connect( m_timeLine, SIGNAL(frameChanged(int)), this, SLOT(loadingAnimationTick()) );
}

CollectionTreeItemModelBase::~CollectionTreeItemModelBase()
{
    KConfigGroup config = Amarok::config( "Collection Browser" );
    QList<int> levelNumbers;
    foreach( CategoryId::CatMenuId category, levels() )
        levelNumbers.append( category );
    config.writeEntry( "TreeCategory", levelNumbers );

    if( m_rootItem )
        m_rootItem->deleteLater();
}

Qt::ItemFlags CollectionTreeItemModelBase::flags(const QModelIndex & index) const
{
    Qt::ItemFlags flags = 0;
    if( index.isValid() )
    {
        flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEditable;
    }
    return flags;

}

bool
CollectionTreeItemModelBase::setData( const QModelIndex &index, const QVariant &value, int role )
{
    Q_UNUSED( role )

    if( !index.isValid() )
        return false;
    CollectionTreeItem *item = static_cast<CollectionTreeItem*>( index.internalPointer() );

    Meta::DataPtr data = item->data();

    if( Meta::TrackPtr track = Meta::TrackPtr::dynamicCast( data ) )
    {
        Meta::TrackEditorPtr ec = track->editor();
        if( ec )
        {
            ec->setTitle( value.toString() );
            emit dataChanged( index, index );
            return true;
        }
    }
    else if( Meta::AlbumPtr album = Meta::AlbumPtr::dynamicCast( data ) )
    {
        Meta::TrackList tracks = album->tracks();
        if( !tracks.isEmpty() )
        {
            foreach( Meta::TrackPtr track, tracks )
            {
                Meta::TrackEditorPtr ec = track->editor();
                if( ec )
                    ec->setAlbum( value.toString() );
            }
            emit dataChanged( index, index );
            return true;
        }
    }
    else if( Meta::ArtistPtr artist = Meta::ArtistPtr::dynamicCast( data ) )
    {
        Meta::TrackList tracks = artist->tracks();
        if( !tracks.isEmpty() )
        {
            foreach( Meta::TrackPtr track, tracks )
            {
                Meta::TrackEditorPtr ec = track->editor();
                if( ec )
                    ec->setArtist( value.toString() );
            }
            emit dataChanged( index, index );
            return true;
        }
    }
    else if( Meta::GenrePtr genre = Meta::GenrePtr::dynamicCast( data ) )
    {
        Meta::TrackList tracks = genre->tracks();
        if( !tracks.isEmpty() )
        {
            foreach( Meta::TrackPtr track, tracks )
            {
                Meta::TrackEditorPtr ec = track->editor();
                if( ec )
                    ec->setGenre( value.toString() );
            }
            emit dataChanged( index, index );
            return true;
        }
    }
    else if( Meta::YearPtr year = Meta::YearPtr::dynamicCast( data ) )
    {
        Meta::TrackList tracks = year->tracks();
        if( !tracks.isEmpty() )
        {
            foreach( Meta::TrackPtr track, tracks )
            {
                Meta::TrackEditorPtr ec = track->editor();
                if( ec )
                    ec->setYear( value.toInt() );
            }
            emit dataChanged( index, index );
            return true;
        }
    }
    else if( Meta::ComposerPtr composer = Meta::ComposerPtr::dynamicCast( data ) )
    {
        Meta::TrackList tracks = composer->tracks();
        if( !tracks.isEmpty() )
        {
            foreach( Meta::TrackPtr track, tracks )
            {
                Meta::TrackEditorPtr ec = track->editor();
                if( ec )
                    ec->setComposer( value.toString() );
            }
            emit dataChanged( index, index );
            return true;
        }
    }
    return false;
}

QVariant
CollectionTreeItemModelBase::dataForItem( CollectionTreeItem *item, int role, int level ) const
{
    if( level == -1 )
        level = item->level();

    if( item->isTrackItem() )
    {
        Meta::TrackPtr track = Meta::TrackPtr::dynamicCast( item->data() );
        switch( role )
        {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
        case PrettyTreeRoles::FilterRole:
            {
                QString name = track->prettyName();
                Meta::AlbumPtr album = track->album();
                Meta::ArtistPtr artist = track->artist();

                if( album && artist && album->isCompilation() )
                    name.prepend( QString("%1 - ").arg(artist->prettyName()) );

                if( AmarokConfig::showTrackNumbers() )
                {
                    int trackNum = track->trackNumber();
                    if( trackNum > 0 )
                        name.prepend( QString("%1 - ").arg(trackNum) );
                }

                // Check empty after track logic and before album logic
                if( name.isEmpty() )
                    name = i18nc( "The Name is not known", "Unknown" );
                return name;
            }

        case Qt::DecorationRole:
            return QIcon::fromTheme( "media-album-track" );
        case PrettyTreeRoles::SortRole:
            return track->sortableName();
        }
    }
    else if( item->isAlbumItem() )
    {
        Meta::AlbumPtr album = Meta::AlbumPtr::dynamicCast( item->data() );
        switch( role )
        {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            {
                QString name = album->prettyName();
                // add years for named albums (if enabled)
                if( AmarokConfig::showYears() && !album->name().isEmpty() )
                {
                    Meta::TrackList tracks = album->tracks();
                    if( !tracks.isEmpty() )
                    {
                        Meta::YearPtr year = tracks.first()->year();
                        if( year && (year->year() != 0) )
                            name.prepend( QString("%1 - ").arg( year->name() ) );
                    }
                }
                return name;
            }

        case Qt::DecorationRole:
            if( AmarokConfig::showAlbumArt() )
            {
                QStyle *style = QApplication::style();
                const int largeIconSize = style->pixelMetric( QStyle::PM_LargeIconSize );

                return The::svgHandler()->imageWithBorder( album, largeIconSize, 2 );
            }
            else
                return iconForLevel( level );

        case PrettyTreeRoles::SortRole:
            return album->sortableName();

        case PrettyTreeRoles::HasCoverRole:
            return AmarokConfig::showAlbumArt();
        }
    }
    else if( item->isDataItem() )
    {
        switch( role )
        {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
        case PrettyTreeRoles::FilterRole:
            {
                QString name = item->data()->prettyName();
                if( name.isEmpty() )
                    name = i18nc( "The Name is not known", "Unknown" );
                return name;
            }

        case Qt::DecorationRole:
            {
                if( m_childQueries.values().contains( item ) )
                {
                    if( level < m_levelType.count() )
                        return m_currentAnimPixmap;
                }
                return iconForLevel( level );
            }

        case PrettyTreeRoles::SortRole:
            return item->data()->sortableName();
        }
    }
    else if( item->isVariousArtistItem() )
    {
        switch( role )
        {
        case Qt::DecorationRole:
            return QIcon::fromTheme( "similarartists-amarok" );
        case Qt::DisplayRole:
            return i18n( "Various Artists" );
        case PrettyTreeRoles::SortRole:
            return QString(); // so that we can compare it against other strings
        }
    }

    // -- all other roles are handled by item
    return item->data( role );
}

QVariant
CollectionTreeItemModelBase::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        if (section == 0)
            return m_headerText;
    }
    return QVariant();
}

QModelIndex
CollectionTreeItemModelBase::index(int row, int column, const QModelIndex & parent) const
{
    //ensure sanity of parameters
    //we are a tree model, there are no columns
    if( row < 0 || column != 0 )
        return QModelIndex();

    CollectionTreeItem *parentItem;

    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = static_cast<CollectionTreeItem*>(parent.internalPointer());

    CollectionTreeItem *childItem = parentItem->child(row);
    if( childItem )
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex
CollectionTreeItemModelBase::parent(const QModelIndex & index) const
{
     if( !index.isValid() )
         return QModelIndex();

     CollectionTreeItem *childItem = static_cast<CollectionTreeItem*>(index.internalPointer());
     CollectionTreeItem *parentItem = childItem->parent();

     return itemIndex( parentItem );
}

int
CollectionTreeItemModelBase::rowCount(const QModelIndex & parent) const
{
    CollectionTreeItem *parentItem;

    if( !parent.isValid() )
        parentItem = m_rootItem;
    else
        parentItem = static_cast<CollectionTreeItem*>(parent.internalPointer());

    return parentItem->childCount();
}

int CollectionTreeItemModelBase::columnCount(const QModelIndex & parent) const
{
    Q_UNUSED( parent )
    return 1;
}

QStringList
CollectionTreeItemModelBase::mimeTypes() const
{
    QStringList types;
    types << AmarokMimeData::TRACK_MIME;
    return types;
}

QMimeData*
CollectionTreeItemModelBase::mimeData( const QModelIndexList &indices ) const
{
    if ( indices.isEmpty() )
        return 0;

    // first, filter out duplicate entries that may arise when both parent and child are selected
    QSet<QModelIndex> indexSet = QSet<QModelIndex>::fromList( indices );
    QMutableSetIterator<QModelIndex> it( indexSet );
    while( it.hasNext() )
    {
        it.next();
        // we go up in parent hierarchy searching whether some parent indices are already in set
        QModelIndex parentIndex = it.value();
        while( parentIndex.isValid() )  // leave the root (top, invalid) index intact
        {
            parentIndex = parentIndex.parent();  // yes, we start from the parent of current index
            if( indexSet.contains( parentIndex ) )
            {
                it.remove(); // parent already in selected set, remove child
                break; // no need to continue inner loop, already deleted
            }
        }
    }

    QList<CollectionTreeItem*> items;
    foreach( const QModelIndex &index, indexSet )
    {
        if( index.isValid() )
            items << static_cast<CollectionTreeItem*>( index.internalPointer() );
    }

    return mimeData( items );
}

QMimeData*
CollectionTreeItemModelBase::mimeData( const QList<CollectionTreeItem*> &items ) const
{
    if ( items.isEmpty() )
        return 0;

    Meta::TrackList tracks;
    QList<Collections::QueryMaker*> queries;

    foreach( CollectionTreeItem *item, items )
    {
        if( item->allDescendentTracksLoaded() ) {
            tracks << item->descendentTracks();
        }
        else
        {
            Collections::QueryMaker *qm = item->queryMaker();
            for( CollectionTreeItem *tmp = item; tmp; tmp = tmp->parent() )
                tmp->addMatch( qm, levelCategory( tmp->level() - 1 ) );
            Collections::addTextualFilter( qm, m_currentFilter );
            queries.append( qm );
        }
    }

    qStableSort( tracks.begin(), tracks.end(), Meta::Track::lessThan );

    AmarokMimeData *mimeData = new AmarokMimeData();
    mimeData->setTracks( tracks );
    mimeData->setQueryMakers( queries );
    mimeData->startQueries();
    return mimeData;
}

bool
CollectionTreeItemModelBase::hasChildren ( const QModelIndex & parent ) const
{
     if( !parent.isValid() )
         return true; // must be root item!

    CollectionTreeItem *item = static_cast<CollectionTreeItem*>(parent.internalPointer());
    //we added the collection level so we have to be careful with the item level
    return !item->isDataItem() || item->level() + levelModifier() <= m_levelType.count();

}

void
CollectionTreeItemModelBase::ensureChildrenLoaded( CollectionTreeItem *item )
{
    //only start a query if necessary and we are not querying for the item's children already
    if ( item->requiresUpdate() && !m_runningQueries.contains( item ) )
    {
     //   listForLevel( item->level() + levelModifier(), item->queryMaker(), item );
    }
}

CollectionTreeItem *
CollectionTreeItemModelBase::treeItem( const QModelIndex &index ) const
{
    if( !index.isValid() || index.model() != this )
        return 0;

    return static_cast<CollectionTreeItem *>( index.internalPointer() );
}

QModelIndex
CollectionTreeItemModelBase::itemIndex( CollectionTreeItem *item ) const
{
    if( !item || item == m_rootItem )
        return QModelIndex();

    return createIndex( item->row(), 0, item );
}

void CollectionTreeItemModelBase::listForLevel(int level, Collections::QueryMaker * qm, CollectionTreeItem * parent)
{
    if( qm && parent )
    {
        // this check should not hurt anyone... needs to check if single... needs it
        if( m_runningQueries.contains( parent ) )
            return;

        // following special cases are handled extra - right when the parent is added
        if( level > m_levelType.count() ||
            parent->isVariousArtistItem() ||
            parent->isNoLabelItem() )
        {
            qm->deleteLater();
            return;
        }

        // - the last level are always the tracks
        if ( level == m_levelType.count() )
            qm->setQueryType( Collections::QueryMaker::Track );

        // - all other levels are more complicate
        else
        {
            Collections::QueryMaker::QueryType nextLevel;
            if( level + 1 >= m_levelType.count() )
                nextLevel = Collections::QueryMaker::Track;
            else
                nextLevel = mapCategoryToQueryType( m_levelType.value( level + 1 ) );

            qm->setQueryType( mapCategoryToQueryType( m_levelType.value( level ) ) );

            CategoryId::CatMenuId category = m_levelType.value( level );
            if( category == CategoryId::Album )
            {
                // restrict query to normal albums if the previous level
                // was the AlbumArtist category. In that case we handle compilations below
                if( levelCategory( level - 1 ) == CategoryId::AlbumArtist )
                    qm->setAlbumQueryMode( Collections::QueryMaker::OnlyNormalAlbums );
            }
            else if( variousArtistCategories.contains( category ) )
                // we used to handleCompilations() only if nextLevel is Album, but I cannot
                // tell any reason why we should have done this --- strohel
                handleCompilations( nextLevel, parent );
            else if( category == CategoryId::Label )
                handleTracksWithoutLabels( nextLevel, parent );
        }

        for( CollectionTreeItem *tmp = parent; tmp; tmp = tmp->parent() )
            tmp->addMatch( qm, levelCategory( tmp->level() - 1 ) );
        Collections::addTextualFilter( qm, m_currentFilter );
        addQueryMaker( parent, qm );
        m_childQueries.insert( qm, parent );
        qm->run();

        //some very quick queries may be done so fast that the loading
        //animation creates an unnecessary flicker, therefore delay it for a bit
        QTimer::singleShot( 150, this, SLOT(startAnimationTick()) );
    }
}

void
CollectionTreeItemModelBase::setLevels( const QList<CategoryId::CatMenuId> &levelType )
{
    if( m_levelType == levelType )
        return;

    m_levelType = levelType;
    updateHeaderText();
    m_expandedItems.clear();
    m_expandedSpecialNodes.clear();
    m_runningQueries.clear();
    m_childQueries.clear();
    m_compilationQueries.clear();
    filterChildren();
}

Collections::QueryMaker::QueryType
CollectionTreeItemModelBase::mapCategoryToQueryType( int levelType ) const
{
    Collections::QueryMaker::QueryType type;
    switch( levelType )
    {
    case CategoryId::Album:
        type = Collections::QueryMaker::Album;
        break;
    case CategoryId::Artist:
        type = Collections::QueryMaker::Artist;
        break;
    case CategoryId::AlbumArtist:
        type = Collections::QueryMaker::AlbumArtist;
        break;
    case CategoryId::Composer:
        type = Collections::QueryMaker::Composer;
        break;
    case CategoryId::Genre:
        type = Collections::QueryMaker::Genre;
        break;
    case CategoryId::Label:
        type = Collections::QueryMaker::Label;
        break;
    case CategoryId::Year:
        type = Collections::QueryMaker::Year;
        break;
    default:
        type = Collections::QueryMaker::None;
        break;
    }

    return type;
}

void
CollectionTreeItemModelBase::addQueryMaker( CollectionTreeItem* item,
                                            Collections::QueryMaker *qm ) const
{
    connect( qm, SIGNAL(newResultReady(Meta::TrackList)), SLOT(newResultReady(Meta::TrackList)), Qt::QueuedConnection );
    connect( qm, SIGNAL(newResultReady(Meta::ArtistList)), SLOT(newResultReady(Meta::ArtistList)), Qt::QueuedConnection );
    connect( qm, SIGNAL(newResultReady(Meta::AlbumList)), SLOT(newResultReady(Meta::AlbumList)), Qt::QueuedConnection );
    connect( qm, SIGNAL(newResultReady(Meta::GenreList)), SLOT(newResultReady(Meta::GenreList)), Qt::QueuedConnection );
    connect( qm, SIGNAL(newResultReady(Meta::ComposerList)), SLOT(newResultReady(Meta::ComposerList)), Qt::QueuedConnection );
    connect( qm, SIGNAL(newResultReady(Meta::YearList)), SLOT(newResultReady(Meta::YearList)), Qt::QueuedConnection );
    connect( qm, SIGNAL(newResultReady(Meta::LabelList)), SLOT(newResultReady(Meta::LabelList)), Qt::QueuedConnection );
    connect( qm, SIGNAL(newResultReady(Meta::DataList)), SLOT(newResultReady(Meta::DataList)), Qt::QueuedConnection );
    connect( qm, SIGNAL(queryDone()), SLOT(queryDone()), Qt::QueuedConnection );
    m_runningQueries.insert( item, qm );
}

void
CollectionTreeItemModelBase::queryDone()
{
    Collections::QueryMaker *qm = qobject_cast<Collections::QueryMaker*>( sender() );
    if( !qm )
        return;

    CollectionTreeItem* item = 0;
    if( m_childQueries.contains( qm ) )
        item = m_childQueries.take( qm );
    else if( m_compilationQueries.contains( qm ) )
        item = m_compilationQueries.take( qm );
    else if( m_noLabelsQueries.contains( qm ) )
        item = m_noLabelsQueries.take( qm );

    if( item )
        m_runningQueries.remove( item, qm );

    //reset icon for this item
    if( item && item != m_rootItem )
    {
        emit dataChanged( itemIndex( item ), itemIndex( item ) );
    }

    //stop timer if there are no more animations active
    if( m_runningQueries.isEmpty() )
    {
        emit allQueriesFinished( m_autoExpand );
        m_autoExpand = false; // reset to default value
        m_timeLine->stop();
    }
    qm->deleteLater();
}

// TODO

/** Small helper function to convert a list of e.g. tracks to a list of DataPtr */
template<class PointerType, class ListType>
static Meta::DataList
convertToDataList( const ListType& list )
{
    Meta::DataList data;
    foreach( PointerType p, list )
        data << Meta::DataPtr::staticCast( p );

    return data;
}

void
CollectionTreeItemModelBase::newResultReady( Meta::TrackList res )
{
    newResultReady( convertToDataList<Meta::TrackPtr, Meta::TrackList>( res ) );
}

void
CollectionTreeItemModelBase::newResultReady( Meta::ArtistList res )
{
    newResultReady( convertToDataList<Meta::ArtistPtr, Meta::ArtistList>( res ) );
}

void
CollectionTreeItemModelBase::newResultReady( Meta::AlbumList res )
{
    newResultReady( convertToDataList<Meta::AlbumPtr, Meta::AlbumList>( res ) );
}

void
CollectionTreeItemModelBase::newResultReady( Meta::GenreList res )
{
    newResultReady( convertToDataList<Meta::GenrePtr, Meta::GenreList>( res ) );
}

void
CollectionTreeItemModelBase::newResultReady( Meta::ComposerList res )
{
    newResultReady( convertToDataList<Meta::ComposerPtr, Meta::ComposerList>( res ) );
}

void
CollectionTreeItemModelBase::newResultReady( Meta::YearList res )
{
    newResultReady( convertToDataList<Meta::YearPtr, Meta::YearList>( res ) );
}

void
CollectionTreeItemModelBase::newResultReady( Meta::LabelList res )
{
    newResultReady( convertToDataList<Meta::LabelPtr, Meta::LabelList>( res ) );
}

void
CollectionTreeItemModelBase::newResultReady( Meta::DataList data )
{
    //if we are expanding an item, we'll find the sender in childQueries
    //otherwise we are filtering all collections
    Collections::QueryMaker *qm = qobject_cast<Collections::QueryMaker*>( sender() );
    if( !qm )
        return;

    if( m_childQueries.contains( qm ) )
        handleNormalQueryResult( qm, data );

    else if( m_compilationQueries.contains( qm ) )
        handleSpecialQueryResult( CollectionTreeItem::VariousArtist, qm, data );

    else if( m_noLabelsQueries.contains( qm ) )
        handleSpecialQueryResult( CollectionTreeItem::NoLabel, qm, data );
}

void
CollectionTreeItemModelBase::handleSpecialQueryResult( CollectionTreeItem::Type type, Collections::QueryMaker *qm, const Meta::DataList &dataList )
{
    CollectionTreeItem *parent = 0;

    if( type == CollectionTreeItem::VariousArtist )
        parent = m_compilationQueries.value( qm );

    else if( type == CollectionTreeItem::NoLabel )
        parent = m_noLabelsQueries.value( qm );

    if( parent )
    {
        QModelIndex parentIndex = itemIndex( parent );

        //if the special query did not return a result we have to remove the
        //the special node itself
        if( dataList.isEmpty() )
        {
            for( int i = 0; i < parent->childCount(); i++ )
            {
                CollectionTreeItem *cti = parent->child( i );
                if( cti->type() == type )
                {
                    //found the special node
                    beginRemoveRows( parentIndex, cti->row(), cti->row() );
                    cti = 0; //will be deleted;
                    parent->removeChild( i );
                    endRemoveRows();
                    break;
                }
            }
            //we have removed the special node if it existed
            return;
        }

        CollectionTreeItem *specialNode = 0;
        if( parent->childCount() == 0 )
        {
            //we only insert the special node
            beginInsertRows( parentIndex, 0, 0 );
            specialNode = new CollectionTreeItem( type, dataList, parent, this );
            //set requiresUpdate, otherwise we will query for the children of specialNode again!
            specialNode->setRequiresUpdate( false );
            endInsertRows();
        }
        else
        {
            for( int i = 0; i < parent->childCount(); i++ )
            {
                CollectionTreeItem *cti = parent->child( i );
                if( cti->type() == type )
                {
                    //found the special node
                    specialNode = cti;
                    break;
                }
            }
            if( !specialNode )
            {
                //we only insert the special node
                beginInsertRows( parentIndex, 0, 0 );
                specialNode = new CollectionTreeItem( type, dataList, parent, this );
                //set requiresUpdate, otherwise we will query for the children of specialNode again!
                specialNode->setRequiresUpdate( false );
                endInsertRows();
            }
            else
            {
                //only call populateChildren for the special node if we have not
                //created it in this method call. The special node ctor takes care
                //of that itself
                populateChildren( dataList, specialNode, itemIndex( specialNode ) );
            }
            //populate children will call setRequiresUpdate on vaNode
            //but as the special query is based on specialNode's parent,
            //we have to call setRequiresUpdate on the parent too
            //yes, this will mean we will call setRequiresUpdate twice
            parent->setRequiresUpdate( false );

            for( int count = specialNode->childCount(), i = 0; i < count; ++i )
            {
                CollectionTreeItem *item = specialNode->child( i );
                if ( m_expandedItems.contains( item->data() ) ) //item will always be a data item
                {
                    listForLevel( item->level() + levelModifier(), item->queryMaker(), item );
                }
            }
        }

        //if the special node exists, check if it has to be expanded
        if( specialNode )
        {
            if( m_expandedSpecialNodes.contains( parent->parentCollection() ) )
            {
                emit expandIndex( createIndex( 0, 0, specialNode ) ); //we have just inserted the vaItem at row 0
            }
        }
    }
}

void
CollectionTreeItemModelBase::handleNormalQueryResult( Collections::QueryMaker *qm, const Meta::DataList &dataList )
{
    CollectionTreeItem *parent = m_childQueries.value( qm );
    if( parent ) {
        QModelIndex parentIndex = itemIndex( parent );
        populateChildren( dataList, parent, parentIndex );

        if ( parent->isDataItem() )
        {
            if ( m_expandedItems.contains( parent->data() ) )
                emit expandIndex( parentIndex );
            else
                //simply insert the item, nothing will change if it is already in the set
                m_expandedItems.insert( parent->data() );
        }
    }
}

void
CollectionTreeItemModelBase::populateChildren( const DataList &dataList, CollectionTreeItem *parent, const QModelIndex &parentIndex )
{
    CategoryId::CatMenuId childCategory = levelCategory( parent->level() );

    // add new rows after existing ones here (which means all artists nodes
    // will be inserted after the "Various Artists" node)
    // figure out which children of parent have to be removed,
    // which new children have to be added, and preemptively emit dataChanged for the rest
    // have to check how that influences performance...
    const QSet<Meta::DataPtr> dataSet = dataList.toSet();
    QSet<Meta::DataPtr> childrenSet;
    foreach( CollectionTreeItem *child, parent->children() )
    {
        // we don't add null children, these are special-cased below
        if( !child->data() )
            continue;

        childrenSet.insert( child->data() );
    }
    const QSet<Meta::DataPtr> dataToBeAdded = dataSet - childrenSet;
    const QSet<Meta::DataPtr> dataToBeRemoved = childrenSet - dataSet;

    // first remove all rows that have to be removed
    // walking through the cildren in reverse order does not screw up the order
    for( int i = parent->childCount() - 1; i >= 0; i-- )
    {
        CollectionTreeItem *child = parent->child( i );

        // should this child be removed?
        bool toBeRemoved;

        if( child->isDataItem() )
            toBeRemoved = dataToBeRemoved.contains( child->data() );
        else if( child->isVariousArtistItem() )
            toBeRemoved = !variousArtistCategories.contains( childCategory );
        else if( child->isNoLabelItem() )
            toBeRemoved = childCategory != CategoryId::Label;
        else
        {
            warning() << "Unknown child type encountered in populateChildren(), removing";
            toBeRemoved = true;
        }

        if( toBeRemoved )
        {
            beginRemoveRows( parentIndex, i, i );
            parent->removeChild( i );
            endRemoveRows();
        }
        else
        {
            // the remainging child items may be dirty, so refresh them
            if( child->isDataItem() && child->data() && m_expandedItems.contains( child->data() ) )
                ensureChildrenLoaded( child );

            // tell the view that the existing children may have changed
            QModelIndex idx = index( i, 0, parentIndex );
            emit dataChanged( idx, idx );
        }
    }

    // add the new rows
    if( !dataToBeAdded.isEmpty() )
    {
        int lastRow = parent->childCount() - 1;
        //the above check ensures that Qt does not crash on beginInsertRows ( because lastRow+1 > lastRow+0)
        beginInsertRows( parentIndex, lastRow + 1, lastRow + dataToBeAdded.count() );
        foreach( Meta::DataPtr data, dataToBeAdded )
        {
            new CollectionTreeItem( data, parent, this );
        }
        endInsertRows();
    }

    parent->setRequiresUpdate( false );
}

void
CollectionTreeItemModelBase::updateHeaderText()
{
    m_headerText.clear();
    for( int i=0; i< m_levelType.count(); ++i )
        m_headerText += nameForLevel( i ) + " / ";

    m_headerText.chop( 3 );
}

QIcon
CollectionTreeItemModelBase::iconForCategory( CategoryId::CatMenuId category )
{
    switch( category )
    {
        case CategoryId::Album :
            return QIcon::fromTheme( "media-optical-amarok" );
        case CategoryId::Artist :
            return QIcon::fromTheme( "view-media-artist-amarok" );
        case CategoryId::AlbumArtist :
            return QIcon::fromTheme( "view-media-artist-amarok" );
        case CategoryId::Composer :
            return QIcon::fromTheme( "filename-composer-amarok" );
        case CategoryId::Genre :
            return QIcon::fromTheme( "favorite-genres-amarok" );
        case CategoryId::Year :
            return QIcon::fromTheme( "clock" );
        case CategoryId::Label :
            return QIcon::fromTheme( "label-amarok" );
        case CategoryId::None:
        default:
            return QIcon::fromTheme( "image-missing" );
    }

}

QIcon
CollectionTreeItemModelBase::iconForLevel( int level ) const
{
    return iconForCategory( m_levelType.value( level ) );
}

QString
CollectionTreeItemModelBase::nameForCategory( CategoryId::CatMenuId category, bool showYears )
{
    switch( category )
    {
        case CategoryId::Album:
            return showYears ? i18n( "Year - Album" ) : i18n( "Album" );
        case CategoryId::Artist:
            return i18n( "Track Artist" );
        case CategoryId::AlbumArtist:
            return i18n( "Album Artist" );
        case CategoryId::Composer:
            return i18n( "Composer" );
        case CategoryId::Genre:
            return i18n( "Genre" );
        case CategoryId::Year:
            return i18n( "Year" );
        case CategoryId::Label:
            return i18n( "Label" );
        case CategoryId::None:
            return i18n( "None" );
        default:
            return QString();
    }
}

QString
CollectionTreeItemModelBase::nameForLevel( int level ) const
{
    return nameForCategory( m_levelType.value( level ), AmarokConfig::showYears() );
}

void
CollectionTreeItemModelBase::handleCompilations( Collections::QueryMaker::QueryType queryType, CollectionTreeItem *parent ) const
{
    // this method will be called when we retrieve a list of artists from the database.
    // we have to query for all compilations, and then add a "Various Artists" node if at least
    // one compilation exists
    Collections::QueryMaker *qm = parent->queryMaker();
    qm->setQueryType( queryType );
    qm->setAlbumQueryMode( Collections::QueryMaker::OnlyCompilations );
    for( CollectionTreeItem *tmp = parent; tmp; tmp = tmp->parent() )
        tmp->addMatch( qm, levelCategory( tmp->level() - 1 ) );

    Collections::addTextualFilter( qm, m_currentFilter );
    addQueryMaker( parent, qm );
    m_compilationQueries.insert( qm, parent );
    qm->run();
}

void
CollectionTreeItemModelBase::handleTracksWithoutLabels( Collections::QueryMaker::QueryType queryType, CollectionTreeItem *parent ) const
{
    Collections::QueryMaker *qm = parent->queryMaker();
    qm->setQueryType( queryType );
    qm->setLabelQueryMode( Collections::QueryMaker::OnlyWithoutLabels );
    for( CollectionTreeItem *tmp = parent; tmp; tmp = tmp->parent() )
        tmp->addMatch( qm, levelCategory( tmp->level() - 1 ) );

    Collections::addTextualFilter( qm, m_currentFilter );
    addQueryMaker( parent, qm );
    m_noLabelsQueries.insert( qm, parent );
    qm->run();
}


void CollectionTreeItemModelBase::startAnimationTick()
{
    //start animation
    if( ( m_timeLine->state() != QTimeLine::Running ) && !m_runningQueries.isEmpty() )
        m_timeLine->start();
}

void CollectionTreeItemModelBase::loadingAnimationTick()
{
    if ( m_animFrame == 0 )
        m_currentAnimPixmap = m_loading2;
    else
        m_currentAnimPixmap = m_loading1;

    m_animFrame = 1 - m_animFrame;

    //trigger an update of all items being populated at the moment;

    QList< CollectionTreeItem * > items = m_runningQueries.uniqueKeys();
    foreach ( CollectionTreeItem* item, items  )
    {
        if( item == m_rootItem )
            continue;
        emit dataChanged( itemIndex( item ), itemIndex( item ) );
    }
}

QString
CollectionTreeItemModelBase::currentFilter() const
{
    return m_currentFilter;
}

void
CollectionTreeItemModelBase::setCurrentFilter( const QString &filter )
{
    m_currentFilter = filter;
    slotFilter( /* autoExpand */ true );
}

void
CollectionTreeItemModelBase::slotFilter( bool autoExpand )
{
    m_autoExpand = autoExpand;
    filterChildren();

    // following is not auto-expansion, it is restoring the state before filtering
    foreach( Collections::Collection *expanded, m_expandedCollections )
    {
        CollectionTreeItem *expandedItem = m_collections.value( expanded->collectionId() ).second;
        if( expandedItem )
            emit expandIndex( itemIndex( expandedItem ) );
    }
}

void
CollectionTreeItemModelBase::slotCollapsed( const QModelIndex &index )
{
    if ( index.isValid() )      //probably unnecessary, but let's be safe
    {
        CollectionTreeItem *item = static_cast<CollectionTreeItem*>( index.internalPointer() );

        switch( item->type() )
        {
        case CollectionTreeItem::Root:
            break; // nothing to do

        case CollectionTreeItem::Collection:
            m_expandedCollections.remove( item->parentCollection() );
            break;

        case CollectionTreeItem::VariousArtist:
        case CollectionTreeItem::NoLabel:
            m_expandedSpecialNodes.remove( item->parentCollection() );
            break;
        case CollectionTreeItem::Data:
            m_expandedItems.remove( item->data() );
            break;
        }
    }
}

void
CollectionTreeItemModelBase::slotExpanded( const QModelIndex &index )
{
    if( !index.isValid() )
        return;

    CollectionTreeItem *item = static_cast<CollectionTreeItem*>( index.internalPointer() );
    // we are really only interested in the special nodes here.
    // we have to remember whether the user expanded a various artists/no labels node or not.
    // otherwise we won't be able to automatically expand the special node after filtering again
    // there is exactly one special node per type per collection, so use the collection to store that information

    // we also need to store collection expansion state here as they are no longer
    // added to th expanded set in handleNormalQueryResult()
    switch( item->type() )
    {
        case CollectionTreeItem::VariousArtist:
        case CollectionTreeItem::NoLabel:
            m_expandedSpecialNodes.insert( item->parentCollection() );
            break;
        case CollectionTreeItem::Collection:
            m_expandedCollections.insert( item->parentCollection() );
        default:
            break;
    }
}

void CollectionTreeItemModelBase::markSubTreeAsDirty( CollectionTreeItem *item )
{
    //tracks are the leaves so they are never dirty
    if( !item->isTrackItem() )
        item->setRequiresUpdate( true );
    for( int i = 0; i < item->childCount(); i++ )
    {
        markSubTreeAsDirty( item->child( i ) );
    }
}

void CollectionTreeItemModelBase::itemAboutToBeDeleted( CollectionTreeItem *item )
{
    // also all the children will be deleted
    foreach( CollectionTreeItem *child, item->children() )
        itemAboutToBeDeleted( child );

    if( !m_runningQueries.contains( item ) )
        return;
    // TODO: replace this hack with QWeakPointer now than we depend on Qt >= 4.8
    foreach(Collections::QueryMaker *qm, m_runningQueries.values( item ))
    {
        m_childQueries.remove( qm );
        m_compilationQueries.remove( qm );
        m_noLabelsQueries.remove( qm );
        m_runningQueries.remove(item, qm);

        //Disconnect all signals from the QueryMaker so we do not get notified about the results
        qm->disconnect();
        qm->abortQuery();
        //Nuke it
        qm->deleteLater();
    }
}

void
CollectionTreeItemModelBase::setDragSourceCollections( const QSet<Collections::Collection*> &collections )
{
    m_dragSourceCollections = collections;
}

bool
CollectionTreeItemModelBase::hasRunningQueries() const
{
    return !m_runningQueries.isEmpty();
}

CategoryId::CatMenuId
CollectionTreeItemModelBase::levelCategory( const int level ) const
{
    const int actualLevel = level + levelModifier();
    if( actualLevel >= 0 && actualLevel < m_levelType.count() )
        return m_levelType.at( actualLevel );

    return CategoryId::None;
}

