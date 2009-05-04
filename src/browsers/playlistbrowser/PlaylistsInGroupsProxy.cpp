/* This file is part of the KDE project
   Copyright (C) 2009 Bart Cerneels <bart.cerneels@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include "PlaylistsInGroupsProxy.h"

#include "context/popupdropper/libpud/PopupDropperAction.h"
#include "Debug.h"
#include "meta/Playlist.h"
#include "SvgHandler.h"
#include "UserPlaylistModel.h"

#include <KIcon>
#include <KInputDialog>

PlaylistsInGroupsProxy::PlaylistsInGroupsProxy( PlaylistBrowserNS::MetaPlaylistModel *model )
    : MetaPlaylistModel()
    , m_model( model )
    , m_renameAction( 0 )
    , m_deleteAction( 0 )
    , m_addToGroupAction( 0 )
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
    connect( m_model, SIGNAL( renameIndex( QModelIndex ) ), SLOT( slotRename( QModelIndex ) ) );
    connect( m_model, SIGNAL( layoutChanged() ), SLOT( buildTree() ) );

    buildTree();
}

PlaylistsInGroupsProxy::~PlaylistsInGroupsProxy()
{
}

/* m_groupHash layout
*  group index in m_groupNames: originalRow in m_model of child Playlists (no sub-groups yet)
*  group index = -1  for non-grouped playlists
*
* TODO: sub-groups
*/
void
PlaylistsInGroupsProxy::buildTree()
{
    DEBUG_BLOCK
    if( !m_model )
        return;

    emit layoutAboutToBeChanged();

    m_groupHash.clear();
    m_groupNames.clear();
    m_parentCreateList.clear();

    int max = m_model->rowCount();
    debug() << QString("building tree with %1 leafs.").arg( max );
    for ( int row = 0; row < max; row++ )
    {
        QModelIndex idx = m_model->index( row, 0, QModelIndex() );
        //Playlists can be in multiple groups but we only use the first TODO: multigroup
        QString groupName = idx.data( PlaylistBrowserNS::UserModel::GroupRole ).toStringList().first();
        debug() << QString("index %1 belongs to groupName %2").arg( row ).arg( groupName );

        int groupIndex = m_groupNames.indexOf( groupName ); //groups are added to the end of the existing list
        if( groupIndex == -1 && !groupName.isEmpty() )
        {
            m_groupNames << groupName;
            groupIndex = m_groupNames.count() - 1;
        }

        m_groupHash.insertMulti( groupIndex, row );
    }
    debug() << "m_groupHash: ";
    for( int groupIndex = 0; groupIndex < m_groupNames.count(); groupIndex++ )
        debug() << m_groupNames[groupIndex] << ": " << m_groupHash.values( groupIndex );
    debug() << m_groupHash.values( -1 );

    emit layoutChanged();
}

int
PlaylistsInGroupsProxy::indexOfParentCreate( const QModelIndex &parent ) const
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
    pc.parentCreateIndex = parent.internalId();
    pc.row = parent.row();
    m_parentCreateList << pc;
    return m_parentCreateList.size() - 1;
}

QModelIndex
PlaylistsInGroupsProxy::index( int row, int column, const QModelIndex& parent ) const
{
    if( !hasIndex(row, column, parent) )
        return QModelIndex();

    int parentCreateIndex = indexOfParentCreate( parent );

    return createIndex( row, column, parentCreateIndex );
}

QModelIndex
PlaylistsInGroupsProxy::parent( const QModelIndex &index ) const
{
    if (!index.isValid())
        return QModelIndex();

    int parentCreateIndex = index.internalId();
    if( parentCreateIndex == -1 )
        return QModelIndex();

    struct ParentCreate pc = m_parentCreateList[parentCreateIndex];
    return createIndex( pc.row, index.column(), pc.parentCreateIndex );
}

int
PlaylistsInGroupsProxy::rowCount( const QModelIndex& index ) const
{
    if( !index.isValid() )
    {
        //the number of top level groups + the number of non-grouped playlists
        int rows = m_groupNames.count() + m_groupHash.values( -1 ).count();
        return rows;
    }

    //TODO:group in group support.
    if( isGroup( index ) )
    {
        qint64 groupIndex = index.row();
        return m_groupHash.count( groupIndex );
    }
    else
    {
        QModelIndex originalIndex = mapToSource( index );
        return m_model->rowCount( originalIndex );
    }
}

QVariant
PlaylistsInGroupsProxy::data( const QModelIndex &index, int role ) const
{
    if( !index.isValid() )
        return QVariant();

    int row = index.row();
    if( isGroup( index ) )
    {
        switch( role )
        {
            case Qt::DisplayRole: return QVariant( m_groupNames[row] );
            case Qt::DecorationRole: return QVariant( KIcon( "folder" ) );
            default: return QVariant();
        }
    }

    return mapToSource( index ).data( role );
}

bool
PlaylistsInGroupsProxy::setData( const QModelIndex &index, const QVariant &value, int role )
{
    DEBUG_BLOCK
    if( isGroup( index ) )
        return changeGroupName( index.data( Qt::DisplayRole ).toString(), value.toString() );

    QModelIndex originalIdx = mapToSource( index );
    return m_model->setData( originalIdx, value, role );
}

int
PlaylistsInGroupsProxy::columnCount( const QModelIndex& index ) const
{
    return m_model->columnCount( mapToSource( index ) );
}

bool
PlaylistsInGroupsProxy::isGroup( const QModelIndex &index ) const
{
    int parentCreateIndex = index.internalId();
    if( parentCreateIndex == -1 && index.row() < m_groupNames.count() )
        return true;
    return false;
}

QModelIndex
PlaylistsInGroupsProxy::mapToSource( const QModelIndex& index ) const
{
    //DEBUG_BLOCK
    //debug() << "index: " << index;
    if( !index.isValid() )
        return QModelIndex();

    if( isGroup( index ) )
        return QModelIndex();

    QModelIndex proxyParent = index.parent();
    //debug() << "parent: " << proxyParent;
    QModelIndex originalParent = mapToSource( proxyParent );
    //debug() << "originalParent: " << originalParent;
    int originalRow = index.row();
    if( !originalParent.isValid() )
    {
        int indexInGroup = index.row();
        if( !proxyParent.isValid() )
            indexInGroup -= m_groupNames.count();
        //debug() << "indexInGroup" << indexInGroup;
        originalRow = m_groupHash.values( proxyParent.row() ).at( indexInGroup );
        //debug() << "originalRow: " << originalRow;
    }
    return m_model->index( originalRow, index.column(), originalParent );
}

QModelIndexList
PlaylistsInGroupsProxy::mapToSource( const QModelIndexList& list ) const
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
PlaylistsInGroupsProxy::mapFromSource( const QModelIndex& index ) const
{
    if( !index.isValid() )
        return QModelIndex();

    //TODO: this needs to be extended to work for tree models as well
    int sourceRow = index.row();
    int parentRow = m_groupHash.key( sourceRow, -1 );

    QModelIndex parent = QModelIndex();
    int proxyRow = m_groupNames.count() + m_groupHash.values( -1 ).indexOf( sourceRow );
    if( parentRow != -1 )
    {
        parent = this->index( parentRow, 0, QModelIndex() );
        proxyRow = m_groupHash.values( parentRow ).indexOf( sourceRow );
    }

    return this->index( proxyRow, 0, parent );
}

Qt::ItemFlags
PlaylistsInGroupsProxy::flags( const QModelIndex &index ) const
{
    if( isGroup( index ) )
        return ( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable );

    QModelIndex originalIdx = mapToSource( index );
    return m_model->flags( originalIdx );
}

void
PlaylistsInGroupsProxy::modelDataChanged( const QModelIndex& start, const QModelIndex& end )
{
    Q_UNUSED( start )
    Q_UNUSED( end )
}

void
PlaylistsInGroupsProxy::modelRowsInserted( const QModelIndex& index, int start, int end )
{
    Q_UNUSED( index )
    Q_UNUSED( start )
    Q_UNUSED( end )
}

void
PlaylistsInGroupsProxy::modelRowsRemoved( const QModelIndex& index, int start, int end )
{
    Q_UNUSED( index )
    Q_UNUSED( start )
    Q_UNUSED( end )
}

void
PlaylistsInGroupsProxy::slotRename( QModelIndex sourceIdx )
{
    QModelIndex proxyIdx = mapFromSource( sourceIdx );
    emit renameIndex( proxyIdx );
}

void
PlaylistsInGroupsProxy::slotDeleteGroup()
{
    DEBUG_BLOCK
}

void
PlaylistsInGroupsProxy::slotRenameGroup()
{
    DEBUG_BLOCK
    //get the name for this new group
    //TODO: do inline rename
    const QString newName = KInputDialog::getText( i18n("New name"),
                i18nc("Enter a new name for a group that already exists", "Enter new group name:") );

    foreach( int originalRow, m_groupHash.values( m_selectedGroups.first().row() ) )
    {
        QModelIndex index = m_model->index( originalRow, 0, QModelIndex() );
        Meta::PlaylistPtr playlist = index.data( 0xf00d ).value<Meta::PlaylistPtr>();
        QStringList groups;
        groups << newName;
        if( playlist )
            playlist->setGroups( groups );
    }
    buildTree();
    emit layoutChanged();
}

void
PlaylistsInGroupsProxy::slotAddToGroup()
{
    DEBUG_BLOCK
    const QString name = KInputDialog::getText( i18n("New name"),
                i18n("Enter new group name:") );
    foreach( QModelIndex index, m_selectedPlaylists )
    {
        Meta::PlaylistPtr playlist = index.data( 0xf00d ).value<Meta::PlaylistPtr>();
        QStringList groups;
        groups << name;
        if( playlist )
            playlist->setGroups( groups );
    }
    buildTree();
}

QList<PopupDropperAction *>
PlaylistsInGroupsProxy::createGroupActions()
{
    QList<PopupDropperAction *> actions;

    if ( m_deleteAction == 0 )
    {
        m_deleteAction = new PopupDropperAction( The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ), "delete_group", KIcon( "media-track-remove-amarok" ), i18n( "&Delete group" ), this );
        connect( m_deleteAction, SIGNAL( triggered() ), this, SLOT( slotDeleteGroup() ) );
    }
    actions << m_deleteAction;

    if ( m_renameAction == 0 )
    {
        m_renameAction =  new PopupDropperAction( The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ), "edit_group", KIcon( "media-track-edit-amarok" ), i18n( "&Rename group" ), this );
        connect( m_renameAction, SIGNAL( triggered() ), this, SLOT( slotRenameGroup() ) );
    }
    actions << m_renameAction;

    return actions;
}

bool
PlaylistsInGroupsProxy::isAGroupSelected( const QModelIndexList& list ) const
{
    foreach( const QModelIndex &index, list )
    {
        if( isGroup( index ) )
            return true;
    }
    return false;
}

bool
PlaylistsInGroupsProxy::isAPlaylistSelected( const QModelIndexList& list ) const
{
    return mapToSource( list ).count() > 0;
}

bool
PlaylistsInGroupsProxy::changeGroupName( const QString &from, const QString &to )
{
    DEBUG_BLOCK
    debug() << "to: " << to << " from:" << from;
    if( m_groupNames.contains( from ) )
        return false;

    int groupRow = m_groupNames.indexOf( from );
    QModelIndex groupIdx = this->index( groupRow, 0, QModelIndex() );
    foreach( int childRow, m_groupHash.values( groupRow ) )
    {
        QModelIndex childIndex = this->index( childRow, 0, groupIdx );
        QModelIndex originalIdx = mapToSource( childIndex );
        m_model->setData( originalIdx, to, PlaylistBrowserNS::UserModel::GroupRole );
    }
    buildTree();
    return 0;
}

QList<PopupDropperAction *>
PlaylistsInGroupsProxy::actionsFor( const QModelIndexList &list )
{
    DEBUG_BLOCK
    bool playlistSelected = isAPlaylistSelected( list );
    bool groupSelected = isAGroupSelected( list );

    QList<PopupDropperAction *> actions;
    m_selectedGroups.clear();
    m_selectedPlaylists.clear();

    //only playlists selected
    if( playlistSelected )
    {
        foreach( const QModelIndex &index, list )
        {
            QModelIndexList tempList;
            tempList << index;
            if( isAPlaylistSelected( tempList ) )
                m_selectedPlaylists << index;
        }
        QModelIndexList originalList = mapToSource( list );
        debug() << originalList.count() << "original indices";
        if( !originalList.isEmpty() )
        {
            actions << m_model->actionsFor( originalList );
            if( m_addToGroupAction == 0 )
            {
                 m_addToGroupAction = new PopupDropperAction(
                         The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ),
                         "add_to_group", KIcon( "folder" ), i18n( "&Add to group" ), this
                );
                connect( m_addToGroupAction, SIGNAL( triggered() ), this, SLOT( slotAddToGroup() ) );
            }
            actions << m_addToGroupAction;
        }
    }
    else if( groupSelected )
    {
        QModelIndexList originalList;
        originalList << m_model->index( 0, 0, QModelIndex() );
        originalList << m_model->index( 1, 0, QModelIndex() );
        actions << m_model->actionsFor( originalList );
        actions << createGroupActions();
        foreach( const QModelIndex &index, list )
        {
            QModelIndexList tempList;
            tempList << index;
            if( isAGroupSelected( tempList ) )
                m_selectedGroups << index;
        }
    }

    return actions;
}

void
PlaylistsInGroupsProxy::loadItems( QModelIndexList list, Playlist::AddOptions insertMode )
{
    QModelIndexList originalList = mapToSource( list );

    m_model->loadItems( originalList, insertMode );
}

#include "PlaylistsInGroupsProxy.moc"
