/****************************************************************************************
 * Copyright (c) 2007-2010 Bart Cerneels <bart.cerneels@kde.org>                        *
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


QtGroupingProxy::QtGroupingProxy( QAbstractItemModel *model, QModelIndex rootNode, int groupedColumn )
    : QAbstractProxyModel()
    , m_model( model )
    , m_rootNode( rootNode )
{
    // signal proxies
    connect( m_model,
        SIGNAL( dataChanged( const QModelIndex&, const QModelIndex& ) ),
        this, SLOT( modelDataChanged( const QModelIndex&, const QModelIndex& ) )
    );
    connect( m_model,
        SIGNAL( rowsInserted( const QModelIndex&, int, int ) ), this,
        SLOT( modelRowsInserted( const QModelIndex &, int, int ) ) );
    connect( m_model, SIGNAL( rowsRemoved( const QModelIndex&, int, int ) ),
        this, SLOT( modelRowsRemoved( const QModelIndex&, int, int ) ) );
    connect( m_model, SIGNAL(layoutChanged()), SLOT(buildTree()) );

    if( m_groupedColumn != -1 )
        setGroupedColumn( groupedColumn );
}

QtGroupingProxy::~QtGroupingProxy()
{
}

void
QtGroupingProxy::setGroupedColumn( int groupedColumn )
{
    m_groupedColumn = groupedColumn;
    buildTree();
}

QList<ColumnVariantMap>
QtGroupingProxy::belongsTo( const QModelIndex &idx )
{
    //qDebug() << __FILE__ << __FUNCTION__;
    QList<ColumnVariantMap> cvmList;

    //get all the data we have for this index
    RoleVariantMap roleVariantMap = m_model->itemData( idx );
    QMapIterator<int, QVariant> i(roleVariantMap);
    while( i.hasNext() )
    {
        i.next();
        int role = i.key();
        QVariant variant = i.value();
        qDebug() << "role " << role << " : (" << variant.typeName() << ") : "<< variant; //so spammy
        if( variant.type() == QVariant::List )
        {
            //a list of variants get's expanded to multiple rows
            QVariantList list = variant.toList();
            for( int i = 0; i < list.length(); i++ )
            {
                //take an existing CVM or create a new one
                ColumnVariantMap cvm = (cvmList.count() > i) ?  cvmList.takeAt( i )
                                       : ColumnVariantMap();

                //we only gather data for the first column
                RoleVariantMap rvm = cvm.contains( 0 ) ? cvm.take( 0 ) : RoleVariantMap();
                rvm.insert( role, list.value( i ) );
                cvm.insert( 0, rvm );
                cvmList.insert( i, cvm );
            }
        }
        else if( !variant.isNull() )
        {
            //it's just a normal item. Copy all the data and break this loop.
            ColumnVariantMap cvm;
            cvm.insert( 0, roleVariantMap );
            cvmList << cvm;
            break;
        }
    }
    //for normal items (not root node) an empty list here means it's supposed to go in root.
    if( cvmList.isEmpty() && idx != m_rootNode )
        cvmList << ColumnVariantMap();

    return cvmList;
}

/* m_groupHash layout
*  key : index of the group in m_groupMaps
*  values : original row in m_model of children of this group
*
*  key = -1  contains non-grouped indexes
*
* TODO: sub-groups
*/
void
QtGroupingProxy::buildTree()
{
    if( !m_model )
        return;

    emit layoutAboutToBeChanged();

    m_groupHash.clear();
    //don't clear the data maps since most of it will probably be needed again.
    m_parentCreateList.clear();

    //first load empty groups from the rootnode.
    QModelIndex rootGroupedIndex =
            m_model->index( m_rootNode.row(), m_groupedColumn, m_rootNode.parent() );
    if( rootGroupedIndex.column() == m_groupedColumn )
    {
        QList<ColumnVariantMap> groupData = belongsTo( rootGroupedIndex );
        foreach( ColumnVariantMap cvm , groupData )
        {
            qDebug() << cvm;
            if( cvm.contains( 0 ) && cvm[0].contains( Qt::DisplayRole ) )
            {
                QString groupName = cvm[0][Qt::DisplayRole].toString();
                qDebug() << "Creating empty group: " << groupName;
                m_groupMaps << cvm;
            }
        }
    }

    int max = m_model->rowCount( m_rootNode );
    //qDebug() << QString("building tree with %1 leafs.").arg( max );
    for( int row = max-1; row >= 0; row-- )
    {
        QModelIndex idx = m_model->index( row, m_groupedColumn, m_rootNode );
        QList<ColumnVariantMap> groupData = belongsTo( idx );

        //an item can be in multiple groups
        foreach( ColumnVariantMap data, groupData )
        {
            int groupIndex = -1;
            if( !data.isEmpty() )
            {
                QString groupName = data[0][Qt::DisplayRole].toString();
                qDebug() << QString("index %1 belongs to group %2").arg( row ).arg( groupName );

                foreach( const ColumnVariantMap &cachedData, m_groupMaps )
                {
                    //when this matches the index belongs to an existing group
                    if( data[0][Qt::DisplayRole] == cachedData[0][Qt::DisplayRole] )
                    {
                        data = cachedData;
                        break;
                    }
                }

                groupIndex = m_groupMaps.indexOf( data );
                //-1 means not found
                if( groupIndex == -1 )
                {
                    //new groups are added to the end of the existing list
                    m_groupMaps << data;
                    groupIndex = m_groupMaps.count() - 1;
                }
            }

            m_groupHash.insertMulti( groupIndex, row );
        }
    }
    dumpGroups();

    emit layoutChanged();
}

/** Each Modelindex has in it's internalId an index in the parentCreateList.
  * struct ParentCreate are the instructions to recreate the parent index.
  * It contains the row number and the index in this list of the grandfather.
  * This function creates the ParentCreate structs and saves them in a list.
  */
int
QtGroupingProxy::indexOfParentCreate( const QModelIndex &parent ) const
{
    if( !parent.isValid() || parent == m_rootNode )
        return -1;

    struct ParentCreate pc;
    for( int i = 0 ; i < m_parentCreateList.size() ; i++ )
    {
        pc = m_parentCreateList[i];
        if( pc.parentCreateIndex == parent.internalId() && pc.row == parent.row() )
            return i;
    }
    //their was no parentCreate yet for this index, so let's create one.
    pc.parentCreateIndex = parent.internalId();
    pc.row = parent.row();
    m_parentCreateList << pc;
    return m_parentCreateList.size() - 1;
}

QModelIndex
QtGroupingProxy::index( int row, int column, const QModelIndex& parent ) const
{
    //qDebug() << "index requested for: (" << row << "," << column << "), " << parent;
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
QtGroupingProxy::rowCount( const QModelIndex& index ) const
{
    //qDebug() << "rowCount: " << index;
    if( !index.isValid() )
    {
        //the number of top level groups + the number of non-grouped playlists
        int rows = m_groupMaps.count() + m_groupHash.values( -1 ).count();
        //qDebug() << rows << " in root group";
        return rows;
    }

    //TODO:group in group support.
    if( isGroup( index ) )
    {
        qint64 groupIndex = index.row();
        int rows = m_groupHash.count( groupIndex );
        //qDebug() << rows << " in group " << m_groupMaps[groupIndex];
        return rows;
    }

    QModelIndex originalIndex = mapToSource( index );
    int rowCount = m_model->rowCount( originalIndex );
    //qDebug() << "original item: rowCount == " << rowCount;
    return rowCount;
}

int
QtGroupingProxy::columnCount( const QModelIndex& index ) const
{
    return m_model->columnCount( mapToSource( index ) );
}

QVariant
QtGroupingProxy::data( const QModelIndex &index, int role ) const
{
    if( !index.isValid() )
        return QVariant();
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
        int childCount = m_groupHash.count( row );
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
        RoleVariantMap roleMap = m_groupMaps[row].value( column );
        foreach( const QVariant &variant, variantsOfChildren )
        {
            if( roleMap[ role ] != variant )
                roleMap.insert( role, variantsOfChildren );
        }

        //qDebug() << QString("roleMap[%1]:").arg(role) << roleMap[role];
        //only one unique variant? No need to return a list
        if( variantsOfChildren.count() == 1 )
            return variantsOfChildren.first();

        if( variantsOfChildren.count() == 0 )
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
        RoleVariantMap columnData = m_groupMaps[idx.row()][idx.column()];

        columnData.insert( role, value );
        //also set the display role if we are changing the grouped column data or the
        //first column
        if( idx.column() == m_groupedColumn || idx.column() == 0 )
            columnData.insert( Qt::DisplayRole, value );
        //and make sure it's stored in the map
        m_groupMaps[idx.row()].insert( idx.column(), columnData );

        int columnToChange = idx.row() != 0 ? idx.row() : m_groupedColumn;
        foreach( int originalRow, m_groupHash.values( idx.row() ) )
        {
            QModelIndex childIdx = m_model->index( originalRow, columnToChange,
                                                   m_rootNode );
            if( childIdx.isValid() )
                m_model->setData( childIdx, value, role );
        }
        emit dataChanged( idx, idx );
        return true;
    }

    return m_model->setData( mapToSource( idx ), value, role );
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
QtGroupingProxy::mapToSource( const QModelIndex& index ) const
{
    //qDebug() << "mapToSource: " << index;
    if( !index.isValid() )
        return QModelIndex();

    if( isGroup( index ) )
    {
        //qDebug() << "is a group: " << index.data( Qt::DisplayRole ).toString();
        return m_rootNode;
    }

    QModelIndex proxyParent = index.parent();
    //qDebug() << "parent: " << proxyParent;
    QModelIndex originalParent = mapToSource( proxyParent );
    //qDebug() << "originalParent: " << originalParent;
    int originalRow = index.row();
    if( originalParent == m_rootNode )
    {
        int indexInGroup = index.row();
        if( !proxyParent.isValid() )
            indexInGroup -= m_groupMaps.count();
        //qDebug() << "indexInGroup" << indexInGroup;
        QList<int> childRows = m_groupHash.values( proxyParent.row() );
        if( childRows.isEmpty() || indexInGroup >= childRows.count() || indexInGroup < 0 )
            return QModelIndex();

        originalRow = childRows.at( indexInGroup );
        //qDebug() << "originalRow: " << originalRow;
    }
    return m_model->index( originalRow, index.column(), originalParent );
}

QModelIndexList
QtGroupingProxy::mapToSource( const QModelIndexList& list ) const
{
    QModelIndexList originalList;
    foreach( const QModelIndex &index, list )
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

    if( sourceParent.isValid() )
    {
        //idx is a child of one of the items in the source model
        proxyParent = mapFromSource( sourceParent );
    }
    else
    {
        //idx is an item of the top level of the source model (child of the rootnode)
        int groupRow = m_groupHash.key( sourceRow, -1 );

        if( groupRow != -1 ) //it's in a group, let's find the correct row.
        {
            proxyParent = this->index( groupRow, 0, QModelIndex() );
            proxyRow = m_groupHash.values( groupRow ).indexOf( sourceRow );
        }
        else
        {
            proxyParent = QModelIndex();
            // if the proxy item is not in a group it will be below the groups.
            int groupLength = m_groupMaps.count();
            //qDebug() << "groupNames length: " << groupLength;
            int i = m_groupHash.values( -1 ).indexOf( sourceRow );
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
    //only if the grouped column has the editable flag set allow the
    //actions leading to setData on the source (edit, drop & drag)
    if( isGroup( idx ) )
    {
        dumpGroups();
        Qt::ItemFlags defaultFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable );
        bool groupIsEditable = true;

        //it's possible to have empty groups
        if( m_groupHash.count( idx.row() ) == 0 )
        {
            //check the flags of this column with the root node
            QModelIndex originalRootNode = m_model->index( m_rootNode.row(), m_groupedColumn,
                                                           m_rootNode.parent() );
            groupIsEditable = originalRootNode.flags().testFlag( Qt::ItemIsEditable );
        }
        else
        {
            foreach( int originalRow, m_groupHash.values( idx.row() ) )
            {
                QModelIndex originalIdx = m_model->index( originalRow, m_groupedColumn,
                                                          m_rootNode );
                qDebug() << "originalIdx: " << originalIdx;
                groupIsEditable = groupIsEditable
                                  ? originalIdx.flags().testFlag( Qt::ItemIsEditable )
                                  : false;
                if( !groupIsEditable ) //all children need to have an editable grouped column
                    break;
            }
        }

        if( groupIsEditable )
            return (  defaultFlags | Qt::ItemIsEditable |
                 Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled );
        return defaultFlags;
    }

    QModelIndex originalIdx = mapToSource( idx );
    Qt::ItemFlags originalItemFlags = m_model->flags( originalIdx );

    //check the source model to see if the grouped column is editable;
    QModelIndex groupedColumnIndex = m_model->index( originalIdx.row(), m_groupedColumn, originalIdx.parent() );
    bool groupIsEditable = m_model->flags( groupedColumnIndex ).testFlag( Qt::ItemIsEditable );
    if( groupIsEditable )
        return originalItemFlags | Qt::ItemIsDragEnabled;

    return originalItemFlags;
}

QVariant
QtGroupingProxy::headerData( int section, Qt::Orientation orientation, int role ) const
{
    return m_model->headerData( section, orientation, role );
}

bool
QtGroupingProxy::canFetchMore( const QModelIndex &parent ) const
{
    if( !parent.isValid() )
        return false;

    if( isGroup( parent ) )
        return false;

    return m_model->canFetchMore( mapToSource( parent ) );
}

void
QtGroupingProxy::fetchMore ( const QModelIndex & parent )
{
    if( !parent.isValid() )
        return;

    if( isGroup( parent ) )
        return;

    return m_model->fetchMore( mapToSource( parent ) );
}

QModelIndex
QtGroupingProxy::addEmptyGroup( const ColumnVariantMap &data )
{
    int newRow = m_groupMaps.count();
    beginInsertRows( QModelIndex(), newRow, newRow );
    m_groupMaps << data;
    endInsertRows();
    emit layoutChanged();
    return index( newRow, 0, QModelIndex() );
}

bool
QtGroupingProxy::removeGroup( const QModelIndex &idx )
{
    //TODO:unset this groups value from the childrens grouped column
    beginRemoveRows( idx.parent(), idx.row(), idx.row() );
    m_groupHash.remove( idx.row() );
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
        return !m_groupHash.values( parent.row() ).isEmpty();

    return m_model->hasChildren( mapToSource( parent ) );
}

void
QtGroupingProxy::modelDataChanged( const QModelIndex& start, const QModelIndex& end )
{
    Q_UNUSED( start )
    Q_UNUSED( end )
    //HACK: range might not be continues in the proxy. Worse case it will refresh to much
    //data though.
    emit dataChanged( mapFromSource( start ), mapFromSource( end ) );
}

void
QtGroupingProxy::modelRowsInserted( const QModelIndex& index, int start, int end )
{
    Q_UNUSED( index )
    Q_UNUSED( start )
    Q_UNUSED( end )
    //TODO: see if new groups have to be created, deleted or adjusted. Try to avoid buildTree()
    buildTree();
}

void
QtGroupingProxy::modelRowsRemoved( const QModelIndex& index, int start, int end )
{
    Q_UNUSED( index )
    Q_UNUSED( start )
    Q_UNUSED( end )
    //don't call buildTree() because it will mess up with removing multiple rows
    //individually using removeRow().
    //TODO: this breaks the view when rows are not deleted manually.
}

void
QtGroupingProxy::slotDeleteGroup()
{
}

void
QtGroupingProxy::slotRenameGroup()
{
    //get the name for this new group
    const QString newName = QInputDialog::getText( 0, tr("New name"),
                tr("Enter a new name for a group that already exists", "Enter new group name:") );

    foreach( int originalRow, m_groupHash.values( m_selectedGroups.first().row() ) )
    {
        QModelIndex index = m_model->index( originalRow, 0, QModelIndex() );
        QStringList groups;
        groups << newName;
        //qDebug() << "Applying " << groups << " to " << index;
    }
    buildTree();
    emit layoutChanged();
}

bool
QtGroupingProxy::isAGroupSelected( const QModelIndexList& list ) const
{
    foreach( const QModelIndex &index, list )
    {
        if( isGroup( index ) )
            return true;
    }
    return false;
}

void
QtGroupingProxy::dumpGroups() const
{
    return; //so spammy
    qDebug() << "m_groupHash: ";
    for( int groupIndex = -1; groupIndex < m_groupHash.keys().count() - 1; groupIndex++ )
    {
        qDebug() << groupIndex << " : " << m_groupHash.values( groupIndex );
    }

    qDebug() << "m_groupMaps: ";
    for( int groupIndex = 0; groupIndex < m_groupMaps.count(); groupIndex++ )
        qDebug() << m_groupMaps[groupIndex] << ": " << m_groupHash.values( groupIndex );
    qDebug() << m_groupHash.values( -1 );
}
//#include "GroupingProxy.moc"
