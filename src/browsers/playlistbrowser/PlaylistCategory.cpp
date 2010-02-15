/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include "PlaylistCategory.h"

#include "CollectionManager.h"
#include "PaletteHandler.h"
#include "playlist/PlaylistModel.h"
#include "PlaylistsInGroupsProxy.h"
#include "PlaylistsByProviderProxy.h"
#include "PlaylistTreeItemDelegate.h"
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

using namespace PlaylistBrowserNS;

QString PlaylistCategory::s_configGroup( "Saved Playlists View" );
QString PlaylistCategory::s_mergeViewKey( "Merged View" );

PlaylistCategory::PlaylistCategory( QWidget * parent )
    : BrowserCategory( "user playlists", parent )
{
    setPrettyName( i18n( "Saved Playlists" ) );
    setShortDescription( i18n( "User generated and imported playlists" ) );
    setIcon( KIcon( "amarok_playlist" ) );

    setLongDescription( i18n( "Create, edit, organize and load playlists. "
        "Amarok automatically adds any playlists found when scanning your collection, "
        "and any playlists that you save are also shown here." ) );

    setImagePath( KStandardDirs::locate(
                        "data", "amarok/images/hover_info_user_playlists.png" )
                  );

    setContentsMargins( 0, 0, 0, 0 );
    m_toolBar = new QToolBar( this );
    m_toolBar->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );

    KAction *toggleAction = new KAction( KIcon( "view-list-tree" ),
                                         i18n( "Merged View" ), m_toolBar );
    toggleAction->setCheckable( true );
    toggleAction->setChecked( Amarok::config( s_configGroup ).readEntry( s_mergeViewKey, false ) );
    m_toolBar->addAction( toggleAction );
    connect( toggleAction, SIGNAL( triggered( bool ) ), SLOT( toggleView( bool ) ) );

    m_playlistView = new UserPlaylistTreeView( The::userPlaylistModel(), this );
    m_byProviderProxy = new PlaylistsByProviderProxy( The::userPlaylistModel(),
                                                      UserModel::ProviderColumn );
    m_byProviderDelegate = new PlaylistTreeItemDelegate( m_playlistView );

    m_byFolderProxy = new PlaylistsInGroupsProxy( The::userPlaylistModel() );
    m_defaultItemView = m_playlistView->itemDelegate();

    toggleView( toggleAction->isChecked() );

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
    new PlaylistTreeItemDelegate( m_playlistView );
}

PlaylistCategory::~PlaylistCategory()
{
}

void PlaylistCategory::newPalette(const QPalette & palette)
{
    Q_UNUSED( palette )

    The::paletteHandler()->updateItemView( m_playlistView );
}

void
PlaylistCategory::toggleView( bool merged )
{
    if( merged )
    {
        m_playlistView->setModel( m_byFolderProxy );
        m_playlistView->setItemDelegate( m_defaultItemView );
        m_playlistView->setRootIsDecorated( true );
    }
    else
    {
        m_playlistView->setModel( m_byProviderProxy );
        m_playlistView->setItemDelegate( m_byProviderDelegate );
        m_playlistView->setRootIsDecorated( false );
    }

    Amarok::config( s_configGroup ).writeEntry( s_mergeViewKey, merged );
}

#include "PlaylistCategory.moc"
