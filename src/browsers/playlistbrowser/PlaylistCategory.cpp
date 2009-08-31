/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (r) 2009 Bart Cerneels <bart.cerneels@kde.org>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "PlaylistCategory.h"

#include "CollectionManager.h"
#include "PaletteHandler.h"
#include "playlist/PlaylistModel.h"
#include "PlaylistsInGroupsProxy.h"
#include "SvgHandler.h"
#include "statusbar/StatusBar.h"
#include "UserPlaylistModel.h"

#include <KAction>
#include <KIcon>
#include <KLineEdit>

#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <KStandardDirs>
#include <QToolBar>
#include <QVBoxLayout>

#include <typeinfo>

PlaylistBrowserNS::PlaylistCategory::PlaylistCategory( QWidget * parent )
    : BrowserCategory( "user playlists", parent )
{
    setPrettyName( i18n( "Saved Playlists" ) );
    setShortDescription( i18n( "User generated and imported playlists" ) );
    setIcon( KIcon( "amarok_playlist" ) );

    setLongDescription( i18n( "Create, edit, organize and load playlists. "
        "Amarok automatically adds any playlists found when scanning your collection, "
        " and any playlists that you save are also shown here." ) );

    setImagePath( KStandardDirs::locate(
                        "data", "amarok/images/hover_info_user_playlists.png" )
                  );

    setContentsMargins( 0, 0, 0, 0 );
    m_toolBar = new QToolBar( this );
    m_toolBar->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );

    m_groupedProxy = new PlaylistsInGroupsProxy( The::userPlaylistModel() );

    m_playlistView = new UserPlaylistTreeView( m_groupedProxy, this );
//    m_playlistView = new UserPlaylistTreeView( The::userPlaylistModel(), this );
    m_playlistView->setFrameShape( QFrame::NoFrame );
    m_playlistView->setContentsMargins( 0, 0, 0, 0 );
    m_playlistView->header()->hide();
    //hide all columns except the first.
    for( int i = 1; i < m_playlistView->model()->columnCount(); i++)
      m_playlistView->hideColumn( i );

    m_playlistView->setDragEnabled( true );
    m_playlistView->setAcceptDrops( true );
    m_playlistView->setDropIndicatorShown( true );

    connect( The::paletteHandler(), SIGNAL( newPalette( const QPalette & ) ),
             SLOT( newPalette( const QPalette & ) ) );

    m_playlistView->setAlternatingRowColors( true );

    m_addGroupAction = new KAction( KIcon( "folder-new" ), i18n( "Add Folder" ), this  );
    m_toolBar->addAction( m_addGroupAction );
    connect( m_addGroupAction, SIGNAL( triggered( bool ) ),
             m_playlistView, SLOT( createNewGroup() ) );

    m_playlistView->setNewGroupAction( m_addGroupAction );
}

PlaylistBrowserNS::PlaylistCategory::~PlaylistCategory()
{
}

void PlaylistBrowserNS::PlaylistCategory::newPalette(const QPalette & palette)
{
    Q_UNUSED( palette )

    The::paletteHandler()->updateItemView( m_playlistView );
}

#include "PlaylistCategory.moc"
