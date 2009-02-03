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

PlaylistsInGroupsProxy::PlaylistsInGroupsProxy( PlaylistBrowserNS::MetaPlaylistModel *model )
    : MetaPlaylistModel()
    , m_model( model )
    , m_renameAction( 0 )
    , m_deleteAction( 0 )
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

    m_groupHash.clear();
    m_groupNames.clear();

    int max = m_model->rowCount();
    debug() << QString("building tree with %1 leafs.").arg( max );
    for ( int row = 0; row < max; row++ )
    {
        QModelIndex idx = m_model->index( row, 0, QModelIndex() );
        //Playlists can be in multiple groups but we only use the first TODO: multigroup
        QString groupName = idx.data( PlaylistBrowserNS::UserModel::GroupRole ).toStringList().first();
        debug() << QString("index %1 belongs to groupName %2").arg( row ).arg( groupName );

        qint64 groupIndex = m_groupNames.indexOf( groupName ); //groups are added to the end of the existing list
        if( groupIndex == -1 && !groupName.isEmpty() )
        {
            m_groupNames << groupName;
            groupIndex = m_groupNames.count() - 1;
        }

        m_groupHash.insertMulti( groupIndex, row );
    }
    debug() << "m_groupHash: ";
    for( qint64 groupIndex = 0; groupIndex < m_groupNames.count(); groupIndex++ )
        debug() << m_groupNames[groupIndex] << ": " << m_groupHash.values( groupIndex );
    debug() << m_groupHash.values( -1 );
}

QModelIndex
PlaylistsInGroupsProxy::index( int row, int column, const QModelIndex& parent ) const
{
    if( !hasIndex(row, column, parent) )
        return QModelIndex();

    int groupIndex = parent.row();

    return createIndex( row, column, groupIndex );
}

QModelIndex
PlaylistsInGroupsProxy::parent( const QModelIndex& index ) const
{
    if (!index.isValid())
        return QModelIndex();

    qint64 groupIndex = index.internalId();
    if( groupIndex == -1 )
        return QModelIndex();

    return this->index( (int)groupIndex, 0, QModelIndex() );
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

    //if child of a group return 0
    //TODO:group in group support.
    if( index.parent().isValid() )
        return 0;

    qint64 groupIndex = index.row();
    int rows = m_groupHash.count( groupIndex );
    return rows;
}

QVariant
PlaylistsInGroupsProxy::data( const QModelIndex &index, int role ) const
{
    if( !index.isValid() )
        return QVariant();

    int row = index.row();
    if( index.internalId() == -1 )
    {
        if( row < m_groupNames.count() )
        {
            switch( role )
            {
                case Qt::DisplayRole: return QVariant( m_groupNames[row] );
                case Qt::DecorationRole: return QVariant( KIcon( "folder" ) );
                default: return QVariant();
            }
        }
    }

    return mapToSource( index ).data( role );
}

int
PlaylistsInGroupsProxy::columnCount( const QModelIndex& index ) const
{
    Q_UNUSED( index );
    return 1;
}

QModelIndex
PlaylistsInGroupsProxy::mapToSource( const QModelIndex& index ) const
{
    DEBUG_BLOCK
    if( !index.isValid() )
        return QModelIndex();

    int row = index.row();
    int parentRow = index.internalId();
    if( parentRow == -1 && row < m_groupNames.count() )
        return QModelIndex();

    debug() << "parentRow = " << parentRow;
    if( parentRow == -1 )
        row = row - m_groupNames.count();
    debug() << "need item from " << row;

    row = m_groupHash.values( parentRow )[row];
    debug() << "source row = " << row;
    return m_model->index( row, 0, QModelIndex() );
}

QModelIndexList
PlaylistsInGroupsProxy::mapToSource( const QModelIndexList& list ) const
{
    QModelIndexList originalList;
    foreach( QModelIndex index, list )
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
    DEBUG_BLOCK
    if( !index.isValid() )
        return QModelIndex();

    int sourceRow = index.row();
    debug() << "source row = " << sourceRow;
    int parentRow = m_groupHash.key( sourceRow, -1 );
    debug() << "parentRow = " << parentRow;

    QModelIndex parent = QModelIndex();
    int proxyRow = sourceRow - m_groupHash.count() - m_groupNames.count();
    if( parentRow != -1 )
    {
        parent = this->index( parentRow, 0, QModelIndex() );
        proxyRow = m_groupHash.values( parentRow ).indexOf( sourceRow );
    }

    debug() << "proxyRow = " << proxyRow;
    return this->index( proxyRow, 0, parent );
}

Qt::ItemFlags
PlaylistsInGroupsProxy::flags( const QModelIndex &index ) const
{
    DEBUG_BLOCK
    if( index.isValid() )
        return ( Qt::ItemIsEnabled | Qt::ItemIsSelectable );

    return 0;
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
PlaylistsInGroupsProxy::slotDeleteGroup()
{
    DEBUG_BLOCK
}

void
PlaylistsInGroupsProxy::slotRenameGroup()
{
    DEBUG_BLOCK
    //get the name for this new group
    QString newName = "newName";
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
        if( index.internalId() < 0 )
            if( m_groupHash.keys().contains( index.row() ) )
                return true;
    }
    return false;
}

bool
PlaylistsInGroupsProxy::isAPlaylistSelected( const QModelIndexList& list ) const
{
    return mapToSource( list ).count() > 0;
}

QList<PopupDropperAction *>
PlaylistsInGroupsProxy::actionsFor( const QModelIndexList &list )
{
    DEBUG_BLOCK
    bool playlistSelected = isAPlaylistSelected( list );
    bool groupSelected = isAGroupSelected( list );

    QList<PopupDropperAction *> actions;
    m_selectedGroups.clear();

    //only playlists selected
    if( playlistSelected )
    {
        QModelIndexList originalList = mapToSource( list );
        debug() << originalList.count() << "original indices";
        if( !originalList.isEmpty() )
        {
            actions << m_model->actionsFor( originalList );
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
    DEBUG_BLOCK
    QModelIndexList originalList = mapToSource( list );

    m_model->loadItems( originalList, insertMode );
}

#include "PlaylistsInGroupsProxy.moc"
