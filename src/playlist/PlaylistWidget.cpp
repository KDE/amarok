/****************************************************************************************
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "PlaylistWidget.h"

#include "ActionClasses.h"
#include "amarokconfig.h"
#include "App.h"
#include "Debug.h"
#include "DynamicModel.h"
#include "layouts/LayoutConfigAction.h"
#include "layouts/LayoutManager.h"
#include "MainWindow.h"
#include "PaletteHandler.h"
#include "PlaylistController.h"
#include "PlaylistDefines.h"
#include "PlaylistHeader.h"
#include "PlaylistManager.h"
#include "PlaylistModelStack.h"
#include "ProgressiveSearchWidget.h"
#include "UserPlaylistProvider.h"
#include "widgets/HorizontalDivider.h"

#include <KActionMenu>
#include <KToolBarSpacerAction>

#include <QHBoxLayout>

Playlist::Widget::Widget( QWidget* parent )
        : KVBox( parent )
{
    DEBUG_BLOCK
    setContentsMargins( 0, 0, 0, 0 );

    m_sortWidget = new Playlist::SortWidget( this );
    new HorizontalDivider( this );

    m_searchWidget = new Playlist::ProgressiveSearchWidget( this );

    // show visual indication of dynamic playlists  being enabled
    connect( PlaylistBrowserNS::DynamicModel::instance(), SIGNAL( enableDynamicMode( bool ) ), SLOT( showDynamicHint( bool ) ) );
    m_dynamicHintWidget = new QLabel( i18n( "Dynamic Mode Enabled" ), this );
    m_dynamicHintWidget->setAlignment( Qt::AlignCenter );
    m_dynamicHintWidget->setStyleSheet( QString( "QLabel { background-color: %1; } " ).arg( PaletteHandler::highlightColor().name() ) );
    showDynamicHint( AmarokConfig::dynamicMode() );

    QWidget * layoutHolder = new QWidget( this );

    QVBoxLayout* mainPlaylistlayout = new QVBoxLayout( layoutHolder );
    mainPlaylistlayout->setContentsMargins( 0, 0, 0, 0 );

    m_playlistView = new PrettyListView( this );
    m_playlistView->show();

    connect( m_searchWidget, SIGNAL( filterChanged( const QString &, int, bool ) ), m_playlistView, SLOT( find( const QString &, int, bool ) ) );
    connect( m_searchWidget, SIGNAL( next( const QString &, int ) ), m_playlistView, SLOT( findNext( const QString &, int ) ) );
    connect( m_searchWidget, SIGNAL( previous( const QString &, int ) ), m_playlistView, SLOT( findPrevious( const QString &, int ) ) );
    connect( m_searchWidget, SIGNAL( filterCleared() ), m_playlistView, SLOT( clearSearchTerm() ) );
    connect( m_searchWidget, SIGNAL( showOnlyMatches( bool ) ), m_playlistView, SLOT( showOnlyMatches( bool ) ) );
    connect( m_searchWidget, SIGNAL( activateFilterResult() ), m_playlistView, SLOT( playFirstSelected() ) );
    connect( m_searchWidget, SIGNAL( downPressed() ), m_playlistView, SLOT( setFocus() ) );

    KConfigGroup searchConfig = Amarok::config("Playlist Search");
    m_playlistView->showOnlyMatches( searchConfig.readEntry( "ShowOnlyMatches", false ) );

    connect( m_playlistView, SIGNAL( found() ), m_searchWidget, SLOT( match() ) );
    connect( m_playlistView, SIGNAL( notFound() ), m_searchWidget, SLOT( noMatch() ) );

    connect( LayoutManager::instance(), SIGNAL( activeLayoutChanged() ), m_playlistView, SLOT( reset() ) );

    mainPlaylistlayout->setSpacing( 0 );
    mainPlaylistlayout->addWidget( m_playlistView );

    KHBox *barBox = new KHBox( this );
    barBox->setMargin( 0 );
    barBox->setContentsMargins( 0, 0, 0, 0 );

    KToolBar *plBar = new KToolBar( barBox, false, false );
    plBar->setFixedHeight( 30 );
    plBar->setObjectName( "PlaylistToolBar" );

    ModelStack::instance();

    // the Controller ctor creates the undo/redo actions that we use below, so we want
    // to make sure that it's been constructed and the the actions registered
    Controller::instance();

    { // START Playlist toolbar
        plBar->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Preferred );
        plBar->setIconDimensions( 22 );
        plBar->setMovable( false );
        plBar->addAction( new KToolBarSpacerAction( this ) );

        plBar->addAction( Amarok::actionCollection()->action( "playlist_clear" ) );

        //FIXME this action should go in ActionController, but we don't have any visibility to the view
        KAction *action = new KAction( KIcon( "music-amarok" ), i18n("Show active track"), this );
        connect( action, SIGNAL( triggered( bool ) ), m_playlistView, SLOT( scrollToActiveTrack() ) );
        plBar->addAction( action );

        plBar->addSeparator();
        plBar->addAction( Amarok::actionCollection()->action( "playlist_undo" ) );
        plBar->addAction( Amarok::actionCollection()->action( "playlist_redo" ) );
        plBar->addSeparator();

        m_savePlaylistMenu = new KActionMenu( KIcon( "document-save-amarok" ), i18n("&Save Current Playlist"), this );
        m_saveActions = new KActionCollection( this );
        connect( m_savePlaylistMenu, SIGNAL( triggered(bool) ), The::playlistManager(),
                 SLOT( saveCurrentPlaylist() ) );
        foreach( PlaylistProvider *provider, The::playlistManager()->providersForCategory(
                            PlaylistManager::UserPlaylist ) )
        {
            playlistProviderAdded( provider, PlaylistManager::UserPlaylist );
        }

        connect( The::playlistManager(),
                 SIGNAL( providerAdded( PlaylistProvider *, int ) ),
                 SLOT( playlistProviderAdded( PlaylistProvider *, int ) )
                 );
        connect( The::playlistManager(),
                 SIGNAL( providerRemoved( PlaylistProvider *, int ) ),
                 SLOT( playlistProviderRemoved( PlaylistProvider *, int ) )
                 );

        plBar->addAction( m_savePlaylistMenu );

        Playlist::LayoutConfigAction *layoutConfigAction = new Playlist::LayoutConfigAction( this );
        plBar->addAction( layoutConfigAction );
        QToolButton *tbutton = qobject_cast<QToolButton*>(plBar->widgetForAction( layoutConfigAction ) );
        if( tbutton )
            tbutton->setPopupMode( QToolButton::InstantPopup );

        plBar->addAction( new KToolBarSpacerAction( this ) );
    } // END Playlist Toolbar

    setFrameShape( QFrame::NoFrame );

    // If it is active, clear the search filter before replacing the playlist. Fixes Bug #200709.
    connect( The::playlistController(), SIGNAL( replacingPlaylist() ), this, SLOT( clearFilterIfActive() ) );
}

QSize
Playlist::Widget::sizeHint() const
{
    return QSize( static_cast<QWidget*>( parent() )->size().width() / 4 , 300 );
}

void
Playlist::Widget::playlistProviderAdded( PlaylistProvider *provider, int category )
{
    if( category != PlaylistManager::UserPlaylist )
        return;

    debug() << "Adding provider: " << provider->objectName();
    UserPlaylistProvider *userProvider =
            dynamic_cast<UserPlaylistProvider *>(provider);
    if( userProvider == 0 )
        return;
    QAction *action = new KAction( userProvider->icon(),
                        i18n("&Save playlist to \"%1\"").arg( provider->prettyName() ),
                        this
                    );
    action->setData( QVariant::fromValue( QPointer<UserPlaylistProvider>( userProvider ) ) );
    m_saveActions->addAction( provider->objectName(), action );

    m_savePlaylistMenu->addAction( action );
    connect( action, SIGNAL( triggered(bool) ), SLOT( slotSaveCurrentPlaylist() ) );
}

void
Playlist::Widget::playlistProviderRemoved( PlaylistProvider *provider, int category )
{
    if( category != PlaylistManager::UserPlaylist )
        return;

    QAction *action = m_saveActions->action( provider->objectName() );
    if( action )
        m_savePlaylistMenu->removeAction( action );
}

void
Playlist::Widget::slotSaveCurrentPlaylist()
{
    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == 0 )
        return;

    UserPlaylistProvider *provider =
            action->data().value< QPointer<UserPlaylistProvider> >();
    if( provider == 0 )
        return;

    The::playlistManager()->save( The::playlist()->tracks(), QString(), provider );
}

void
Playlist::Widget::showDynamicHint( bool enabled ) // slot
{
    DEBUG_BLOCK

    if( enabled )
        m_dynamicHintWidget->show();
    else
        m_dynamicHintWidget->hide();
}

void
Playlist::Widget::clearFilterIfActive() // slot
{
    DEBUG_BLOCK
    KConfigGroup config = Amarok::config( "Playlist Search" );
    bool filterActive = config.readEntry( "ShowOnlyMatches", true );

    if( filterActive )
        m_searchWidget->slotFilterClear();
}
