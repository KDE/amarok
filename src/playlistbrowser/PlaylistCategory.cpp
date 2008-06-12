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

#include <QHeaderView>
#include <QVBoxLayout>

#include <typeinfo>

using namespace PlaylistBrowserNS;

PlaylistCategory::PlaylistCategory( QWidget * parent )
    : Amarok::Widget( parent )
{

    setContentsMargins(0,0,0,0);
    
    QTreeView * playlistView = new QTreeView( this );
    playlistView->setFrameShape( QFrame::NoFrame );
    playlistView->setContentsMargins(0,0,0,0);
    playlistView->setModel( PlaylistModel::instance() );
    playlistView->header()->hide();

    connect( playlistView, SIGNAL( clicked( const QModelIndex & ) ), this, SLOT( itemActivated(  const QModelIndex & ) ) );

    QVBoxLayout *vLayout = new QVBoxLayout( this );
    vLayout->setContentsMargins(0,0,0,0);
    vLayout->addWidget( playlistView );

    playlistView->setAlternatingRowColors( true );

    //transparency
    QPalette p = playlistView->palette();
    QColor c = p.color( QPalette::Base );
    c.setAlpha( 0 );
    p.setColor( QPalette::Base, c );

    c = p.color( QPalette::AlternateBase );
    c.setAlpha( 77 );
    p.setColor( QPalette::AlternateBase, c );

    playlistView->setPalette( p );


    /*QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    playlistView->setSizePolicy(sizePolicy);
    setSizePolicy(sizePolicy);*/
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

#include "PlaylistCategory.moc"


