/****************************************************************************************
 * Copyright (c) 2007-2011 Bart Cerneels <bart.cerneels@kde.org>                        *
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

#include "QtGroupingProxy.h"

#include <QDebug>
#include <QIcon>
#include <QInputDialog>
#include <QTimer>

/*!
    \class QtGroupingProxy
    \brief The QtGroupingProxy class will group source model rows by adding a new top tree-level.
    The source model can be flat or tree organized, but only the original top level rows are used
    for determining the grouping.
    \ingroup model-view
*/


QtGroupingProxy::QtGroupingProxy( QObject *parent )
    : QAbstractProxyModel( parent )
{
}

QtGroupingProxy::QtGroupingProxy( QAbstractItemModel *model, const QModelIndex &rootIndex,
                                  int groupedColumn, QObject *parent )
    : QAbstractProxyModel( parent )
    , m_rootIndex( rootIndex )
    , m_groupedColumn( 0 )
{
    setSourceModel( model );

    if( groupedColumn != -1 )
        setGroupedColumn( groupedColumn );
}

QtGroupingProxy::~QtGroupingProxy()
{
}

void
QtGroupingProxy::setSourceModel( QAbstractItemModel *sourceModel )
{
    QAbstractProxyModel::setSourceModel( sourceModel );
    // signal proxies
    connect( sourceModel, &QAbstractItemModel::dataChanged,
             this, &QtGroupingProxy::modelDataChanged );
    connect( sourceModel, &QAbstractItemModel::rowsInserted,
             this, &QtGroupingProxy::modelRowsInserted );
    connect( sourceModel, &QAbstractItemModel::rowsAboutToBeInserted,
             this, &QtGroupingProxy::modelRowsAboutToBeInserted );
    connect( sourceModel, &QAbstractItemModel::rowsRemoved,
             this, &QtGroupingProxy::modelRowsRemoved );
    connect( sourceModel, &QAbstractItemModel::rowsAboutToBeRemoved,
             this, &QtGroupingProxy::modelRowsAboutToBeRemoved );
    connect( sourceModel, &QAbstractItemModel::layoutChanged,
             this, &QtGroupingProxy::buildTree );
    connect( sourceModel, &QAbstractItemModel::dataChanged,
             this, &QtGroupingProxy::modelDataChanged );
    //set invalid index from source as root index
    m_rootIndex = sourceModel->index( -1, -1 );
}

void
QtGroupingProxy::setRootIndex( const QModelIndex &rootIndex )
{
    if( m_rootIndex == rootIndex )
        return;

    m_rootIndex = rootIndex;
    //TODO: invalidate tree so buildTree() can be called later.
}

void
QtGroupingProxy::setGroupedColumn( int groupedColumn )
{
    m_groupedColumn = groupedColumn;
    //TODO: invalidate tree so buildTree() can be called later.
    buildTree();
}

/** Maps to what groups the source row belongs by returning the data of those groups.
  *
  * @returns a list of data for the rows the argument belongs to. In common cases this list will
  * contain only one entry. An empty list means that the source item will be placed in the root of
  * this proxyModel. There is no support for hiding source items.
  *
  * Group data can be pre-loaded in the return value so it's added to the cache maintained by this
  * class. This is required if you want to have data that is not present in the source model.
  */
QList<RowData>
QtGroupingProxy::belongsTo( const QModelIndex &idx )
{
    //qDebug() << __FILE__ << __FUNCTION__;
    QList<RowData> rowDataList;

    //get all the data for this index from the model
    ItemData itemData = sourceModel()->itemData( idx );
    QMapIterator<int, QVariant> i( itemData );
    while( i.hasNext() )
    {
        i.next();
        int role = i.key();
        QVariant variant = i.value();
        // qDebug() << "role " << role << " : (" << variant.typeName() << ") : "<< variant;
        if( variant.type() == QVariant::List )
        {
            //a list of variants get's expanded to multiple rows
            QVariantList list = variant.toList();
            for( int i = 0; i < list.length(); i++ )
            {
                //take an existing row data or create a new one
                RowData rowData = (rowDataList.count() > i) ?  rowDataList.takeAt( i )
                                       : RowData();

                //we only gather data for the first column
                ItemData indexData = rowData.contains( 0 ) ? rowData.take( 0 ) : ItemData();
                indexData.insert( role, list.value( i ) );
                rowData.insert( 0, indexData );
                //for the grouped column the data should not be gathered from the children
                //this will allow filtering on the content of this column with a
                //QSortFilterProxyModel
                rowData.insert( m_groupedColumn, indexData );
                rowDataList.insert( i, rowData );
            }
        }
        else if( !variant.isNull() )
        {
            //it's just a normal item. Copy all the data and break this loop.
            RowData rowData;
            rowData.insert( 0, itemData );
            rowDataList << rowData;
            break;
        }
    }

    return rowDataList;
}

/* m_groupMap layout
*  key : index of the group in m_groupMaps
*  value : a QList of the original rows in sourceModel() for the children of this group
*
*  key = -1  contains a QList of the non-grouped indexes
*
* TODO: sub-groups
*/
void
QtGroupingProxy::buildTree()
{
    if( !sourceModel() )
        return;

    beginResetModel();

    m_groupMap.clear();
    //don't clear the data maps since most of it will probably be needed again.
    m_parentCreateList.clear();

    int max = sourceModel()->rowCount( m_rootIndex );
    //qDebug() << QStringLiteral("building tree with %1 leafs.").arg( max );
    //WARNING: these have to be added in order because the addToGroups function is optimized for
    //modelRowsInserted(). Failure to do so will result in wrong data shown in the view at best.
    for( int row = 0; row < max; row++ )
    {
        QModelIndex idx = sourceModel()->index( row, m_groupedColumn, m_rootIndex );
        addSourceRow( idx );
    }
//    dumpGroups();

    endResetModel();
}

QList<int>
QtGroupingProxy::addSourceRow( const QModelIndex &idx )
{
    QList<int> updatedGroups;

    QList<RowData> groupData = belongsTo( idx );

    //an empty list here means it's supposed to go in root.
    if( groupData.isEmpty() )
    {
        updatedGroups << -1;
        if( !m_groupMap.keys().contains( -1 ) )
            m_groupMap.insert( -1, QList<int>() ); //add an empty placeholder
    }

    //an item can be in multiple groups
    for( RowData data : groupData )
    {
        int updatedGroup = -1;
        if( !data.isEmpty() )
        {
//            qDebug() << QStringLiteral("index %1 belongs to group %2").arg( row )
//                         .arg( data[0][Qt::DisplayRole].toString() );

            for( const RowData &cachedData : m_groupMaps )
            {
                //when this matches the index belongs to an existing group
                if( data[0][Qt::DisplayRole] == cachedData[0][Qt::DisplayRole] )
                {
                    data = cachedData;
                    break;
                }
            }

            updatedGroup = m_groupMaps.indexOf( data );
            //-1 means not found
            if( updatedGroup == -1 )
            {
                QModelIndex newGroupIdx = addEmptyGroup( data );
                updatedGroup = newGroupIdx.row();
            }

            if( !m_groupMap.keys().contains( updatedGroup ) )
                m_groupMap.insert( updatedGroup, QList<int>() ); //add an empty placeholder
        }

        if( !updatedGroups.contains( updatedGroup ) )
            updatedGroups << updatedGroup;
    }


    //update m_groupMap to the new source-model layout (one row added)
    QMutableMapIterator<quint32, QList<int> > i( m_groupMap );
    while( i.hasNext() )
    {
        i.next();
        QList<int> &groupList = i.value();
        int insertedProxyRow = groupList.count();
        for( ; insertedProxyRow > 0 ; insertedProxyRow-- )
        {
            int &rowValue = groupList[insertedProxyRow-1];
            if( idx.row() <= rowValue )
            {
                //increment the rows that come after the new row since they moved one place up.
                rowValue++;
            }
            else
            {
                break;
            }
        }

        if( updatedGroups.contains( i.key() ) )
        {
            //the row needs to be added to this group
            beginInsertRows( index( i.key() ), insertedProxyRow, insertedProxyRow );
            groupList.insert( insertedProxyRow, idx.row() );
            endInsertRows();
        }
    }

    return updatedGroups;
}

/** Each ModelIndex has in it's internalId a position in the parentCreateList.
  * struct ParentCreate are the instructions to recreate the parent index.
  * It contains the proxy row number of the parent and the position in this list of the grandfather.
  * This function creates the ParentCreate structs and saves them in a list.
  */
int
QtGroupingProxy::indexOfParentCreate( const QModelIndex &parent ) const
{
    if( !parent.isValid() )
        return -1;

    struct ParentCreate pc;
    for( int i = 0 ; i < m_parentCreateList.size() ; i++ )
    {
        pc = m_parentCreateList[i];
        if( pc.parentCreateIndex == parent.internalId() && pc.row == parent.row() )
            return i;
    }
    //there is no parentCreate yet for this index, so let's create one.
    pc.parentCreateIndex = parent.internalId();
    pc.row = parent.row();
    m_parentCreateList << pc;

    //dumpParentCreateList();
//    qDebug() << QString( "m_parentCreateList: (%1)" ).arg( m_parentCreateList.size() );
//    for( int i = 0 ; i < m_parentCreateList.size() ; i++ )
//    {
//        qDebug() << i << " : " << m_parentCreateList[i].parentCreateIndex <<
//                 " | " << m_parentCreateList[i].row;
//    }

    return m_parentCreateList.size() - 1;
}

QModelIndex
QtGroupingProxy::index( int row, int column, const QModelIndex &parent ) const
{
//    qDebug() << "index requested for: (" << row << "," << column << "), " << parent;
    if( !hasIndex(row, column, parent) )
        return QModelIndex();

    if( parent.column() > 0 )
        return QModelIndex();

    /* We save the instructions to make the parent of the index in a struct.
     * The place of the struct in the list is stored in the internalId
     */
    int parentCreateIndex = indexOfParentCreate( parent );

    return createIndex( row, column, parentCreateIndex );
}

QModelIndex
QtGroupingProxy::parent( const QModelIndex &index ) const
{
    //qDebug() << "parent: " << index;
    if( !index.isValid() )
        return QModelIndex();

    int parentCreateIndex = index.internalId();
    //qDebug() << "parentCreateIndex: " << parentCreateIndex;
    if( parentCreateIndex == -1 || parentCreateIndex >= m_parentCreateList.count() )
        return QModelIndex();

    struct ParentCreate pc = m_parentCreateList[parentCreateIndex];
    //qDebug() << "parentCreate: (" << pc.parentCreateIndex << "," << pc.row << ")";
    //only items at column 0 have children
    return createIndex( pc.row, 0, pc.parentCreateIndex );
}

int
QtGroupingProxy::rowCount( const QModelIndex &index ) const
{
    //qDebug() << "rowCount: " << index;
    if( !index.isValid() )
    {
        //the number of top level groups + the number of non-grouped playlists
        int rows = m_groupMaps.count() + m_groupMap.value( -1 ).count();
        //qDebug() << rows << " in root group";
        return rows;
    }

    //TODO:group in group support.
    if( isGroup( index ) )
    {
        qint64 groupIndex = index.row();
        int rows = m_groupMap.value( groupIndex ).count();
        //qDebug() << rows << " in group " << m_groupMaps[groupIndex];
        return rows;
    }

    QModelIndex originalIndex = mapToSource( index );
    int rowCount = sourceModel()->rowCount( originalIndex );
    //qDebug() << "original item: rowCount == " << rowCount;
    return rowCount;
}

int
QtGroupingProxy::columnCount( const QModelIndex &index ) const
{
    if( !index.isValid() )
        return sourceModel()->columnCount( m_rootIndex );

    if( index.column() != 0 )
        return 0;

    return sourceModel()->columnCount( mapToSource( index ) );
}

QVariant
QtGroupingProxy::data( const QModelIndex &index, int role ) const
{
    if( !index.isValid() )
        return sourceModel()->data( m_rootIndex, role ); //rootNode could have useful data

    //qDebug() << __FUNCTION__ << index << " role: " << role;
    int row = index.row();
    int column = index.column();
    if( isGroup( index ) )
    {
        //qDebug() << __FUNCTION__ << "is a group";
        //use cached or precalculated data
        if( m_groupMaps[row][column].contains( role ) )
        {
            //qDebug() << "Using cached data";
            return m_groupMaps[row][column].value( role );
        }

        //for column 0 we gather data from the grouped column instead
        if( column == 0 )
            column = m_groupedColumn;

        //map all data from children to columns of group to allow grouping one level up
        QVariantList variantsOfChildren;
        int childCount = m_groupMap.value( row ).count();
        if( childCount == 0 )
            return QVariant();

        //qDebug() << __FUNCTION__ << "childCount: " << childCount;
        //Need a parentIndex with column == 0 because only those have children.
        QModelIndex parentIndex = this->index( row, 0, index.parent() );
        for( int childRow = 0; childRow < childCount; childRow++ )
        {
            QModelIndex childIndex = this->index( childRow, column, parentIndex );
            QVariant data = mapToSource( childIndex ).data( role );
            //qDebug() << __FUNCTION__ << data << QVariant::typeToName(data.type());
            if( data.isValid() && !variantsOfChildren.contains( data ) )
                variantsOfChildren << data;
        }
        //qDebug() << "gathered this data from children: " << variantsOfChildren;
        //saving in cache
        ItemData roleMap = m_groupMaps[row].value( column );
        for( const QVariant &variant : variantsOfChildren )
        {
            if( roleMap[ role ] != variant )
                roleMap.insert( role, variantsOfChildren );
        }

        //qDebug() << QStringLiteral("roleMap[%1]:").arg(role) << roleMap[role];
        //only one unique variant? No need to return a list
        if( variantsOfChildren.count() == 1 )
            return variantsOfChildren.first();

        if( variantsOfChildren.isEmpty() )
            return QVariant();

        return variantsOfChildren;
    }

    return mapToSource( index ).data( role );
}

bool
QtGroupingProxy::setData( const QModelIndex &idx, const QVariant &value, int role )
{
    if( !idx.isValid() )
        return false;

    //no need to set data to exactly the same value
    if( idx.data( role ) == value )
        return false;

    if( isGroup( idx ) )
    {
        ItemData columnData = m_groupMaps[idx.row()][idx.column()];

        columnData.insert( role, value );
        //QItemDelegate will always use Qt::EditRole
        if( role == Qt::EditRole )
            columnData.insert( Qt::DisplayRole, value );

        //and make sure it's stored in the map
        m_groupMaps[idx.row()].insert( idx.column(), columnData );

        int columnToChange = idx.column() ? idx.column() : m_groupedColumn;
        for( int originalRow : m_groupMap.value( idx.row() ) )
        {
            QModelIndex childIdx = sourceModel()->index( originalRow, columnToChange,
                                                   m_rootIndex );
            if( childIdx.isValid() )
                sourceModel()->setData( childIdx, value, role );
        }
        //TODO: we might need to reload the data from the children at this point

        Q_EMIT dataChanged( idx, idx );
        return true;
    }

    return sourceModel()->setData( mapToSource( idx ), value, role );
}

bool
QtGroupingProxy::isGroup( const QModelIndex &index ) const
{
    int parentCreateIndex = index.internalId();
    if( parentCreateIndex == -1 && index.row() < m_groupMaps.count() )
        return true;
    return false;
}

QModelIndex
QtGroupingProxy::mapToSource( const QModelIndex &index ) const
{
    //qDebug() << "mapToSource: " << index;
    if( !index.isValid() )
        return m_rootIndex;

    if( isGroup( index ) )
    {
        //qDebug() << "is a group: " << index.data( Qt::DisplayRole ).toString();
        return m_rootIndex;
    }

    QModelIndex proxyParent = index.parent();
    //qDebug() << "parent: " << proxyParent;
    QModelIndex originalParent = mapToSource( proxyParent );
    //qDebug() << "originalParent: " << originalParent;
    int originalRow = index.row();
    if( originalParent == m_rootIndex )
    {
        int indexInGroup = index.row();
        if( !proxyParent.isValid() )
            indexInGroup -= m_groupMaps.count();
        //qDebug() << "indexInGroup" << indexInGroup;
        QList<int> childRows = m_groupMap.value( proxyParent.row() );
        if( childRows.isEmpty() || indexInGroup >= childRows.count() || indexInGroup < 0 )
            return QModelIndex();

        originalRow = childRows.at( indexInGroup );
        //qDebug() << "originalRow: " << originalRow;
    }
    return sourceModel()->index( originalRow, index.column(), originalParent );
}

QModelIndexList
QtGroupingProxy::mapToSource( const QModelIndexList& list ) const
{
    QModelIndexList originalList;
    for( const QModelIndex &index : list )
    {
        QModelIndex originalIndex = mapToSource( index );
        if( originalIndex.isValid() )
            originalList << originalIndex;
    }
    return originalList;
}

QModelIndex
QtGroupingProxy::mapFromSource( const QModelIndex &idx ) const
{
    if( !idx.isValid() )
        return QModelIndex();

    QModelIndex proxyParent;
    QModelIndex sourceParent = idx.parent();
    //qDebug() << "sourceParent: " << sourceParent;
    int proxyRow = idx.row();
    int sourceRow = idx.row();

    if( sourceParent.isValid() && ( sourceParent != m_rootIndex ) )
    {
        //idx is a child of one of the items in the source model
        proxyParent = mapFromSource( sourceParent );
    }
    else
    {
        //idx is an item in the top level of the source model (child of the rootnode)
        int groupRow = -1;
        QMapIterator<quint32, QList<int> > iterator( m_groupMap );
        while( iterator.hasNext() )
        {
            iterator.next();
            if( iterator.value().contains( sourceRow ) )
            {
                groupRow = iterator.key();
                break;
            }
        }

        if( groupRow != -1 ) //it's in a group, let's find the correct row.
        {
            proxyParent = this->index( groupRow, 0, QModelIndex() );
            proxyRow = m_groupMap.value( groupRow ).indexOf( sourceRow );
        }
        else
        {
            proxyParent = QModelIndex();
            // if the proxy item is not in a group it will be below the groups.
            int groupLength = m_groupMaps.count();
            //qDebug() << "groupNames length: " << groupLength;
            int i = m_groupMap.value( -1 ).indexOf( sourceRow );
            //qDebug() << "index in hash: " << i;
            proxyRow = groupLength + i;
        }
    }

    //qDebug() << "proxyParent: " << proxyParent;
    //qDebug() << "proxyRow: " << proxyRow;
    return this->index( proxyRow, 0, proxyParent );
}

Qt::ItemFlags
QtGroupingProxy::flags( const QModelIndex &idx ) const
{
    if( !idx.isValid() )
    {
        Qt::ItemFlags rootFlags = sourceModel()->flags( m_rootIndex );
        if( rootFlags.testFlag( Qt::ItemIsDropEnabled ) )
            return Qt::ItemFlags( Qt::ItemIsDropEnabled );

        return {};
    }
    //only if the grouped column has the editable flag set allow the
    //actions leading to setData on the source (edit & drop)
//    qDebug() << idx;
    if( isGroup( idx ) )
    {
//        dumpGroups();
        Qt::ItemFlags defaultFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable );
        bool groupIsEditable = true;

        //it's possible to have empty groups
        if( m_groupMap.value( idx.row() ).isEmpty() )
        {
            //check the flags of this column with the root node
            QModelIndex originalRootNode = sourceModel()->index( m_rootIndex.row(), m_groupedColumn,
                                                           m_rootIndex.parent() );
            groupIsEditable = originalRootNode.flags().testFlag( Qt::ItemIsEditable );
        }
        else
        {
            for( int originalRow : m_groupMap.value( idx.row() ) )
            {
                QModelIndex originalIdx = sourceModel()->index( originalRow, m_groupedColumn,
                                                          m_rootIndex );
//                qDebug() << "originalIdx: " << originalIdx;
                groupIsEditable = groupIsEditable
                                  ? originalIdx.flags().testFlag( Qt::ItemIsEditable )
                                  : false;
                if( !groupIsEditable ) //all children need to have an editable grouped column
                    break;
            }
        }

        if( groupIsEditable )
            return (  defaultFlags | Qt::ItemIsEditable | Qt::ItemIsDropEnabled );
        return defaultFlags;
    }

    QModelIndex originalIdx = mapToSource( idx );
    Qt::ItemFlags originalItemFlags = sourceModel()->flags( originalIdx );

    //check the source model to see if the grouped column is editable;
    QModelIndex groupedColumnIndex =
            sourceModel()->index( originalIdx.row(), m_groupedColumn, originalIdx.parent() );
    bool groupIsEditable = sourceModel()->flags( groupedColumnIndex ).testFlag( Qt::ItemIsEditable );
    if( groupIsEditable )
        return originalItemFlags | Qt::ItemIsDragEnabled;

    return originalItemFlags;
}

QModelIndex
QtGroupingProxy::buddy( const QModelIndex &index ) const
{
    /* We need to override this method in case of groups. Otherwise, at least editing
     * of groups is prevented, following sequence occurs:
     *
     * #0  QtGroupingProxy::mapToSource (this=0x15ad8a0, index=...) at /home/strohel/projekty/amarok/src/browsers/playlistbrowser/QtGroupingProxy.cpp:492
     * #1  0x00007ffff609d7b6 in QAbstractProxyModel::buddy (this=0x15ad8a0, index=...) at itemviews/qabstractproxymodel.cpp:306
     * #2  0x00007ffff609ed25 in QSortFilterProxyModel::buddy (this=0x15ae730, index=...) at itemviews/qsortfilterproxymodel.cpp:2015
     * #3  0x00007ffff6012a2c in QAbstractItemView::edit (this=0x15aec30, index=..., trigger=QAbstractItemView::AllEditTriggers, event=0x0) at itemviews/qabstractitemview.cpp:2569
     * #4  0x00007ffff6f9aa9f in Amarok::PrettyTreeView::edit (this=0x15aec30, index=..., trigger=QAbstractItemView::AllEditTriggers, event=0x0)
     *     at /home/strohel/projekty/amarok/src/widgets/PrettyTreeView.cpp:58
     * #5  0x00007ffff6007f1e in QAbstractItemView::edit (this=0x15aec30, index=...) at itemviews/qabstractitemview.cpp:1138
     * #6  0x00007ffff6dc86e4 in PlaylistBrowserNS::PlaylistBrowserCategory::createNewFolder (this=0x159bf90)
     *     at /home/strohel/projekty/amarok/src/browsers/playlistbrowser/PlaylistBrowserCategory.cpp:298
     *
     * but we return invalid index in mapToSource() for group index.
     */
    if( index.isValid() && isGroup( index ) )
        return index;
    return QAbstractProxyModel::buddy( index );
}

QVariant
QtGroupingProxy::headerData( int section, Qt::Orientation orientation, int role ) const
{
    return sourceModel()->headerData( section, orientation, role );
}

bool
QtGroupingProxy::canFetchMore( const QModelIndex &parent ) const
{
    if( !parent.isValid() )
        return false;

    if( isGroup( parent ) )
        return false;

    return sourceModel()->canFetchMore( mapToSource( parent ) );
}

void
QtGroupingProxy::fetchMore ( const QModelIndex & parent )
{
    if( !parent.isValid() )
        return;

    if( isGroup( parent ) )
        return;

    sourceModel()->fetchMore( mapToSource( parent ) );
}

QModelIndex
QtGroupingProxy::addEmptyGroup( const RowData &data )
{
    int newRow = m_groupMaps.count();
    beginInsertRows( QModelIndex(), newRow, newRow );
    m_groupMaps << data;
    endInsertRows();
    return index( newRow, 0, QModelIndex() );
}

bool
QtGroupingProxy::removeGroup( const QModelIndex &idx )
{
    beginRemoveRows( idx.parent(), idx.row(), idx.row() );
    m_groupMap.remove( idx.row() );
    m_groupMaps.removeAt( idx.row() );
    m_parentCreateList.removeAt( idx.internalId() );
    endRemoveRows();

    //TODO: only true if all data could be unset.
    return true;
}

bool
QtGroupingProxy::hasChildren( const QModelIndex &parent ) const
{
    if( !parent.isValid() )
        return true;

    if( isGroup( parent ) )
        return !m_groupMap.value( parent.row() ).isEmpty();

    return sourceModel()->hasChildren( mapToSource( parent ) );
}

void
QtGroupingProxy::modelRowsAboutToBeInserted( const QModelIndex &parent, int start, int end )
{
    if( parent != m_rootIndex )
    {
        //an item will be added to an original index, remap and pass it on
        QModelIndex proxyParent = mapFromSource( parent );
        beginInsertRows( proxyParent, start, end );
    }
}

void
QtGroupingProxy::modelRowsInserted( const QModelIndex &parent, int start, int end )
{
    if( parent == m_rootIndex )
    {
        //top level of the model changed, these new rows need to be put in groups
        for( int modelRow = start; modelRow <= end ; modelRow++ )
        {
            addSourceRow( sourceModel()->index( modelRow, m_groupedColumn, m_rootIndex ) );
        }
    }
    else
    {
        //beginInsertRows had to be called in modelRowsAboutToBeInserted()
        endInsertRows();
    }
}

void
QtGroupingProxy::modelRowsAboutToBeRemoved( const QModelIndex &parent, int start, int end )
{
    if( parent == m_rootIndex )
    {
        QMap<quint32, QList<int> >::const_iterator i;
        //HACK, we are going to call beginRemoveRows() multiple times without
        // endRemoveRows() if a source index is in multiple groups.
        // This can be a problem for some views/proxies, but Q*Views can handle it.
        // TODO: investigate a queue for applying proxy model changes in the correct order
        for( i = m_groupMap.constBegin(); i != m_groupMap.constEnd(); ++i )
        {
            int groupIndex = i.key();
            const QList<int> &groupList = i.value();
            QModelIndex proxyParent = index( groupIndex, 0 );
            for( int originalRow : groupList )
            {
                if( originalRow >= start && originalRow <= end )
                {
                    int proxyRow = groupList.indexOf( originalRow );
                    if( groupIndex == -1 ) //adjust for non-grouped (root level) original items
                        proxyRow += m_groupMaps.count();
                    //TODO: optimize for continues original rows in the same group
                    beginRemoveRows( proxyParent, proxyRow, proxyRow );
                }
            }
        }
    }
    else
    {
        //child item(s) of an original item will be removed, remap and pass it on
//        qDebug() << parent;
        QModelIndex proxyParent = mapFromSource( parent );
//        qDebug() << proxyParent;
        beginRemoveRows( proxyParent, start, end );
    }
}

void
QtGroupingProxy::modelRowsRemoved( const QModelIndex &parent, int start, int end )
{
    if( parent == m_rootIndex )
    {
        //TODO: can be optimised by iterating over m_groupMap and checking start <= r < end

        //rather than increasing i we change the stored sourceRows in-place and reuse argument start
        //X-times (where X = end - start).
        for( int i = start; i <= end; i++ )
        {
            //HACK: we are going to iterate the map in reverse so calls to endRemoveRows()
            // are matched up with the beginRemoveRows() in modelRowsAboutToBeRemoved()
            //NOTE: easier to do reverse with java style iterator
            //NOTE: changed from hash to map for Qt6 compatibility
            QMutableMapIterator<quint32, QList<int> > it( m_groupMap );
            it.toBack();
            while( it.hasPrevious() )
            {
                it.previous();
                //has to be a modifiable reference for remove and replace operations
                QList<int> &groupList = it.value();
                int rowIndex = groupList.indexOf( start );
                if( rowIndex != -1 )
                {
                    groupList.removeAt( rowIndex );
                }
                //Now decrement all source rows that are after the removed row
                for( int j = 0; j < groupList.count(); j++ )
                {
                    int sourceRow = groupList.at( j );
                    if( sourceRow > start )
                        groupList.replace( j, sourceRow-1 );
                }
                if( rowIndex != -1)
                    endRemoveRows(); //end remove operation only after group was updated.
            }
        }

        return;
    }

    //beginRemoveRows had to be called in modelRowsAboutToBeRemoved();
    endRemoveRows();
}

void
QtGroupingProxy::modelDataChanged( const QModelIndex &topLeft, const QModelIndex &bottomRight )
{
    //TODO: need to look in the groupedColumn and see if it changed and changed grouping accordingly
    QModelIndex proxyTopLeft = mapFromSource( topLeft );
    if( !proxyTopLeft.isValid() )
        return;

    if( topLeft == bottomRight )
    {
        Q_EMIT dataChanged( proxyTopLeft, proxyTopLeft );
    }
    else
    {
        QModelIndex proxyBottomRight = mapFromSource( bottomRight );
        Q_EMIT dataChanged( proxyTopLeft, proxyBottomRight );
    }
}

bool
QtGroupingProxy::isAGroupSelected( const QModelIndexList& list ) const
{
    for( const QModelIndex &index : list )
    {
        if( isGroup( index ) )
            return true;
    }
    return false;
}

void
QtGroupingProxy::dumpGroups() const
{
    qDebug() << "m_groupMap: ";
    for( int groupIndex = -1; groupIndex < m_groupMap.keys().count() - 1; groupIndex++ )
    {
        qDebug() << groupIndex << " : " << m_groupMap.value( groupIndex );
    }

    qDebug() << "m_groupMaps: ";
    for( int groupIndex = 0; groupIndex < m_groupMaps.count(); groupIndex++ )
        qDebug() << m_groupMaps[groupIndex] << ": " << m_groupMap.value( groupIndex );
    qDebug() << m_groupMap.value( -1 );
}
