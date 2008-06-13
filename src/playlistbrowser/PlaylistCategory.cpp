/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
 
#include "PlaylistCategory.h"

#include "playlist/PlaylistModel.h"
#include "SqlPlaylist.h"
#include "TheInstances.h"

#include <KIcon>
#include <KMenu>

#include <QHeaderView>
#include <QVBoxLayout>

#include <typeinfo>

using namespace PlaylistBrowserNS;

PlaylistCategory::PlaylistCategory( QWidget * parent )
    : Amarok::Widget( parent )
    , m_deleteAction( 0 )
    , m_renameAction( 0 )
{

    setContentsMargins(0,0,0,0);
    
    m_playlistView = new QTreeView( this );
    m_playlistView->setFrameShape( QFrame::NoFrame );
    m_playlistView->setContentsMargins(0,0,0,0);
    m_playlistView->setModel( PlaylistModel::instance() );
    m_playlistView->header()->hide();

    
    m_playlistView->setContextMenuPolicy( Qt::CustomContextMenu );
    m_playlistView->setEditTriggers( QAbstractItemView::NoEditTriggers );

    connect( m_playlistView, SIGNAL( activated( const QModelIndex & ) ), this, SLOT( itemActivated(  const QModelIndex & ) ) );
    connect( m_playlistView, SIGNAL( customContextMenuRequested( const QPoint & ) ), this, SLOT( showContextMenu( const QPoint & ) ) );

    connect( PlaylistModel::instance(), SIGNAL( editIndex( const QModelIndex & ) ), m_playlistView, SLOT( edit( const QModelIndex & ) ) );

    QVBoxLayout *vLayout = new QVBoxLayout( this );
    vLayout->setContentsMargins(0,0,0,0);
    vLayout->addWidget( m_playlistView );

    m_playlistView->setAlternatingRowColors( true );

    //transparency
    QPalette p = m_playlistView->palette();
    QColor c = p.color( QPalette::Base );
    c.setAlpha( 0 );
    p.setColor( QPalette::Base, c );

    c = p.color( QPalette::AlternateBase );
    c.setAlpha( 77 );
    p.setColor( QPalette::AlternateBase, c );

    m_playlistView->setPalette( p );

}


PlaylistCategory::~PlaylistCategory()
{
}

void PlaylistBrowserNS::PlaylistCategory::itemActivated(const QModelIndex & index)
{
    //DEBUG_BLOCK
    if ( !index.isValid() )
        return;

    SqlPlaylistViewItem * item = static_cast< SqlPlaylistViewItem* >( index.internalPointer() );

    if ( typeid( * item ) == typeid( Meta::SqlPlaylist ) ) {
        Meta::SqlPlaylist * playlist = static_cast< Meta::SqlPlaylist* >( index.internalPointer() );
        //debug() << "playlist name: " << playlist->name();
        //The::playlistModel()->insertOptioned( Meta::PlaylistPtr( playlist ), Playlist::Append );
        The::playlistModel()->insertOptioned( playlist->tracks(), Playlist::Append );
    }
}

void PlaylistBrowserNS::PlaylistCategory::showContextMenu( const QPoint & pos )
{

    QModelIndexList indices = m_playlistView->selectionModel()->selectedIndexes();

    KMenu menu;

    if ( m_deleteAction == 0 )
        m_deleteAction = new PopupDropperAction( KIcon("media-track-remove-amarok" ), i18n( "delete" ), this  );

    if ( m_renameAction == 0 )
        m_renameAction = new PopupDropperAction( KIcon("media-track-edit-amarok" ), i18n( "rename" ), this  );

    menu.addAction( m_deleteAction );

    if ( indices.count() == 1 )
        menu.addAction( m_renameAction );

    PopupDropperAction* result = dynamic_cast< PopupDropperAction* > ( menu.exec( m_playlistView->mapToGlobal( pos ) ) );
    if ( result == 0 ) return;


    if( result == m_deleteAction ) {
        foreach( const QModelIndex &idx, indices )
        {
            SqlPlaylistViewItem * item = static_cast<SqlPlaylistViewItem *>( idx.internalPointer() );
            item->removeFromDb();
            item->parent()->deleteChild( item );
        }
        PlaylistModel::instance()->reloadFromDb();
    }

    if( result == m_renameAction ) {
        m_playlistView->edit( indices.first() );
    }
}

#include "PlaylistCategory.moc"


