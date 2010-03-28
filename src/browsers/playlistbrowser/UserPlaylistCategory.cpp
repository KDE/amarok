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

#include "UserPlaylistCategory.h"

#include "CollectionManager.h"
#include "core/support/Debug.h"
#include "PaletteHandler.h"
#include "playlist/PlaylistModel.h"
#include "playlistmanager/PlaylistManager.h"
#include "PlaylistsInGroupsProxy.h"
#include "PlaylistsByProviderProxy.h"
#include "PlaylistTreeItemDelegate.h"
#include "SvgHandler.h"
#include "statusbar/StatusBar.h"
#include "UserPlaylistModel.h"

#include <KAction>
#include <KActionMenu>
#include <KButtonGroup>
#include <KIcon>
#include <KLineEdit>

#include <QButtonGroup>
#include <QCheckBox>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <KStandardDirs>
#include <QToolBar>
#include <QVBoxLayout>

#include <typeinfo>

using namespace PlaylistBrowserNS;

QString UserPlaylistCategory::s_configGroup( "Saved Playlists View" );
QString UserPlaylistCategory::s_mergeViewKey( "Merged View" );

UserPlaylistCategory::UserPlaylistCategory( QWidget * parent )
    : BrowserCategory( "user playlists", parent )
{
    setPrettyName( i18n( "Saved Playlists" ) );
    setShortDescription( i18n( "User generated and imported playlists" ) );
    setIcon( KIcon( "amarok_playlist" ) );

    setLongDescription( i18n( "Create, edit, organize and load playlists. "
        "Amarok automatically adds any playlists found when scanning your collection, "
        "and any playlists that you save are also shown here." ) );

    setImagePath( KStandardDirs::locate( "data", "amarok/images/hover_info_user_playlists.png" ) );

    setContentsMargins( 0, 0, 0, 0 );
    KToolBar *toolBar = new KToolBar( this, false, false );
    toolBar->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );

    m_byProviderProxy = new PlaylistsByProviderProxy( The::userPlaylistModel(),
                                                      UserModel::ProviderColumn );
    m_byFolderProxy = new PlaylistsInGroupsProxy( The::userPlaylistModel() );

    m_filterProxy = new QSortFilterProxyModel( this );
    m_filterProxy->setDynamicSortFilter( true );
    m_filterProxy->setFilterKeyColumn( PlaylistBrowserNS::UserModel::ProviderColumn );

    m_playlistView = new UserPlaylistTreeView( m_filterProxy, this );
    m_defaultItemView = m_playlistView->itemDelegate();
    m_byProviderDelegate = new PlaylistTreeItemDelegate( m_playlistView );

    m_addGroupAction = new KAction( KIcon( "folder-new" ), i18n( "Add Folder" ), this  );
    toolBar->addAction( m_addGroupAction );
    connect( m_addGroupAction, SIGNAL( triggered( bool ) ), SLOT( createNewGroup() ) );

    m_playlistView->setNewGroupAction( m_addGroupAction );

    //a QWidget with minimumExpanding makes the next button right aligned.
    QWidget *spacerWidget = new QWidget( this );
    spacerWidget->setSizePolicy( QSizePolicy::MinimumExpanding,
                                 QSizePolicy::MinimumExpanding );
    toolBar->addWidget( spacerWidget );

    m_providerMenu = new KActionMenu( KIcon( "checkbox" ), i18n( "Visible Sources"), this );
    m_providerMenu->setDelayed( false );
    toolBar->addAction( m_providerMenu );

    KAction *toggleAction = new KAction( KIcon( "view-list-tree" ), QString(), toolBar );
    toggleAction->setToolTip( i18n( "Merged View" ) );
    toggleAction->setCheckable( true );
    toggleAction->setChecked( Amarok::config( s_configGroup ).readEntry( s_mergeViewKey, false ) );
    toolBar->addAction( toggleAction );
    connect( toggleAction, SIGNAL( triggered( bool ) ), SLOT( toggleView( bool ) ) );

    toggleView( toggleAction->isChecked() );

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

    foreach( const Playlists::PlaylistProvider *provider,
             The::playlistManager()->providersForCategory( Playlists::UserPlaylist ) )
    {
        createProviderButton( provider );
    }
}

UserPlaylistCategory::~UserPlaylistCategory()
{
}

void UserPlaylistCategory::newPalette(const QPalette & palette)
{
    Q_UNUSED( palette )

    The::paletteHandler()->updateItemView( m_playlistView );
}

void
UserPlaylistCategory::toggleView( bool merged )
{
    if( merged )
    {
        m_filterProxy->setSourceModel( m_byFolderProxy );
        m_playlistView->setItemDelegate( m_defaultItemView );
        m_playlistView->setRootIsDecorated( true );
    }
    else
    {
        m_filterProxy->setSourceModel( m_byProviderProxy );
        m_playlistView->setItemDelegate( m_byProviderDelegate );
        m_playlistView->setRootIsDecorated( false );
    }

    //folders don't make sense in per-provider view
    m_addGroupAction->setEnabled( merged );
    //TODO: set a tooltip saying why it's disabled mention labels

    Amarok::config( s_configGroup ).writeEntry( s_mergeViewKey, merged );
}

void
UserPlaylistCategory::slotProviderAdded( Playlists::PlaylistProvider *provider, int category )
{
    Q_UNUSED( category )

    if( !m_providerActions.keys().contains( provider ) )
        createProviderButton( provider );
}

void
UserPlaylistCategory::slotProviderRemoved( Playlists::PlaylistProvider *provider, int category )
{
    Q_UNUSED( category )

    if( m_providerActions.keys().contains( provider ) )
    {
        QAction *providerToggle = m_providerActions.take( provider );
        m_providerMenu->removeAction( providerToggle );
    }
}

void
UserPlaylistCategory::createProviderButton( const Playlists::PlaylistProvider *provider )
{
    QAction *providerToggle = new QAction( provider->icon(), provider->prettyName(), this );
    providerToggle->setCheckable( true );
    providerToggle->setChecked( true );
    providerToggle->setData( QVariant::fromValue( provider ) );
    connect( providerToggle, SIGNAL(toggled(bool)), SLOT(slotToggleProviderButton(bool)) );
    m_providerMenu->addAction( providerToggle );
    m_providerActions.insert( provider, providerToggle );
}

void
UserPlaylistCategory::slotToggleProviderButton( bool enabled )
{
    Q_UNUSED( enabled )
    DEBUG_BLOCK

    QAction * const action = qobject_cast<QAction *>( QObject::sender() );
    const Playlists::PlaylistProvider *provider = action->data().value<const Playlists::PlaylistProvider *>();
    if( !m_providerActions.keys().contains( provider ) )
        return;

    QString filter;
    foreach( const Playlists::PlaylistProvider *p, m_providerActions.keys() )
    {
        QAction *action = m_providerActions.value( p );
        if( action->isChecked() )
        {
            QString escapedName = QRegExp::escape( p->prettyName() ).replace( " ", "\\ " );
            filter += QString( filter.isEmpty() ? "^%1" : "|^%1" ).arg( escapedName );
        }
    }
    m_filterProxy->setFilterRegExp( filter );
}

void
UserPlaylistCategory::createNewGroup()
{
    QModelIndex idx = m_byFolderProxy->createNewGroup( i18np( "New Folder", "New Folder (%1)", 1 ) );
    m_playlistView->edit( m_filterProxy->mapFromSource( idx ) );
}
