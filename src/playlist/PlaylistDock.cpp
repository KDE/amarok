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

#include "PlaylistDock.h"

#include "ActionClasses.h"
#include "amarokconfig.h"
#include "App.h"
#include "core/support/Debug.h"
#include "DynamicModel.h"
#include "layouts/LayoutManager.h"
#include "MainWindow.h"
#include "navigators/NavigatorConfigAction.h"
#include "PaletteHandler.h"
#include "PlaylistController.h"
#include "PlaylistDefines.h"
#include "PlaylistManager.h"
#include "PlaylistModelStack.h"
#include "ProgressiveSearchWidget.h"
#include "core-impl/playlists/providers/user/UserPlaylistProvider.h"
#include "widgets/HorizontalDivider.h"

#include <KActionMenu>
#include <KToolBarSpacerAction>

#include <QHBoxLayout>

Playlist::Dock::Dock( QWidget* parent )
    : AmarokDockWidget( i18n( "Playlist" ), parent )
{
    DEBUG_BLOCK

    setObjectName( "Playlist dock" );
    setAllowedAreas( Qt::AllDockWidgetAreas );
}

void
Playlist::Dock::polish()
{
    DEBUG_BLOCK
    m_mainWidget = new KVBox( this );
    setWidget( m_mainWidget );
    m_mainWidget->setContentsMargins( 0, 0, 0, 0 );
    m_mainWidget->setFrameShape( QFrame::NoFrame );

    m_mainWidget->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Ignored );
    m_mainWidget->setFocus( Qt::ActiveWindowFocusReason );

    m_sortWidget = new Playlist::SortWidget( m_mainWidget );
    new HorizontalDivider( m_mainWidget );

    m_searchWidget = new Playlist::ProgressiveSearchWidget( m_mainWidget );

    // show visual indication of dynamic playlists  being enabled
    connect( PlaylistBrowserNS::DynamicModel::instance(), SIGNAL( enableDynamicMode( bool ) ), SLOT( showDynamicHint( bool ) ) );
    m_dynamicHintWidget = new QLabel( i18n( "Dynamic Mode Enabled" ), m_mainWidget );
    m_dynamicHintWidget->setAlignment( Qt::AlignCenter );
    m_dynamicHintWidget->setStyleSheet( QString( "QLabel { background-color: %1; color: %2; border-radius: 3px; } " )
                                                 .arg( PaletteHandler::highlightColor().name() )
                                                 .arg( The::paletteHandler()->palette().highlightedText().color().name() ) );
    QFont dynamicHintWidgetFont = m_dynamicHintWidget->font();
    dynamicHintWidgetFont.setPointSize( dynamicHintWidgetFont.pointSize() + 1 );
    m_dynamicHintWidget->setFont( dynamicHintWidgetFont );

    showDynamicHint( AmarokConfig::dynamicMode() );

    paletteChanged( App::instance()->palette() );
    connect( The::paletteHandler(), SIGNAL( newPalette( const QPalette& ) ), SLOT(  paletteChanged( const QPalette &  ) ) );

    QWidget * layoutHolder = new QWidget( m_mainWidget );

    QVBoxLayout* mainPlaylistlayout = new QVBoxLayout( layoutHolder );
    mainPlaylistlayout->setContentsMargins( 0, 0, 0, 0 );

    m_playlistView = new PrettyListView( m_mainWidget );
    m_playlistView->show();

    connect( m_searchWidget, SIGNAL( filterChanged( const QString &, int, bool ) ), m_playlistView, SLOT( find( const QString &, int, bool ) ) );
    connect( m_searchWidget, SIGNAL( next( const QString &, int ) ), m_playlistView, SLOT( findNext( const QString &, int ) ) );
    connect( m_searchWidget, SIGNAL( previous( const QString &, int ) ), m_playlistView, SLOT( findPrevious( const QString &, int ) ) );
    connect( m_searchWidget, SIGNAL( filterCleared() ), m_playlistView, SLOT( clearSearchTerm() ) );
    connect( m_searchWidget, SIGNAL( showOnlyMatches( bool ) ), m_playlistView, SLOT( showOnlyMatches( bool ) ) );
    connect( m_searchWidget, SIGNAL( activateFilterResult() ), m_playlistView, SLOT( playFirstSelected() ) );
    connect( m_searchWidget, SIGNAL( downPressed() ), m_playlistView, SLOT( setFocus() ) );

    connect( The::mainWindow(), SIGNAL( switchQueueStateShortcut() ), m_playlistView, SLOT( switchQueueState() ) );

    KConfigGroup searchConfig = Amarok::config("Playlist Search");
    m_playlistView->showOnlyMatches( searchConfig.readEntry( "ShowOnlyMatches", false ) );

    connect( m_playlistView, SIGNAL( found() ), m_searchWidget, SLOT( match() ) );
    connect( m_playlistView, SIGNAL( notFound() ), m_searchWidget, SLOT( noMatch() ) );

    connect( LayoutManager::instance(), SIGNAL( activeLayoutChanged() ), m_playlistView, SLOT( reset() ) );

    mainPlaylistlayout->setSpacing( 0 );
    mainPlaylistlayout->addWidget( m_playlistView );

    KHBox *barBox = new KHBox( m_mainWidget );
    barBox->setMargin( 0 );
    barBox->setContentsMargins( 0, 0, 0, 0 );

    // Use QToolBar instead of KToolBar, see bug 228390
    QToolBar *plBar = new QToolBar( barBox );
    plBar->setFixedHeight( 30 );
    plBar->setObjectName( "PlaylistToolBar" );

    ModelStack::instance(); //This also creates the Controller.

    { // START Playlist toolbar
        plBar->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Preferred );
        plBar->setMovable( false );
        plBar->addAction( new KToolBarSpacerAction( m_mainWidget ) );

        plBar->addAction( Amarok::actionCollection()->action( "playlist_clear" ) );

        plBar->addSeparator();

        m_savePlaylistMenu = new KActionMenu( KIcon( "document-save-amarok" ), i18n("&Save Current Playlist"), m_mainWidget );
        m_saveActions = new KActionCollection( m_mainWidget );
        connect( m_savePlaylistMenu, SIGNAL( triggered( bool ) ),
                 SLOT( slotSaveCurrentPlaylist() ) );
        foreach( Playlists::PlaylistProvider *provider, The::playlistManager()->providersForCategory(
                            PlaylistManager::UserPlaylist ) )
        {
            playlistProviderAdded( provider, PlaylistManager::UserPlaylist );
        }

        connect( The::playlistManager(),
                 SIGNAL( providerAdded( Playlists::PlaylistProvider *, int ) ),
                 SLOT( playlistProviderAdded( Playlists::PlaylistProvider *, int ) )
                 );
        connect( The::playlistManager(),
                 SIGNAL( providerRemoved( Playlists::PlaylistProvider *, int ) ),
                 SLOT( playlistProviderRemoved( Playlists::PlaylistProvider *, int ) )
                 );

        plBar->addAction( m_savePlaylistMenu );

        plBar->addSeparator();
        plBar->addAction( Amarok::actionCollection()->action( "playlist_undo" ) );
        plBar->addAction( Amarok::actionCollection()->action( "playlist_redo" ) );
        plBar->addSeparator();

        plBar->addAction( Amarok::actionCollection()->action( "show_active_track" ) );
        plBar->addSeparator();

        NavigatorConfigAction * navigatorConfig = new NavigatorConfigAction( m_mainWidget );
        plBar->addAction( navigatorConfig );
        plBar->addAction( new KToolBarSpacerAction( m_mainWidget ) );

        QToolButton *toolButton = qobject_cast<QToolButton*>(plBar->widgetForAction( navigatorConfig ) );
        if( toolButton )
            toolButton->setPopupMode( QToolButton::InstantPopup );
    } // END Playlist Toolbar

    // If it is active, clear the search filter before replacing the playlist. Fixes Bug #200709.
    connect( The::playlistController(), SIGNAL( replacingPlaylist() ), this, SLOT( clearFilterIfActive() ) );

}

QSize
Playlist::Dock::sizeHint() const
{
    return QSize( static_cast<QWidget*>( parent() )->size().width() / 4 , 300 );
}


void
Playlist::Dock::paletteChanged( const QPalette& palette )
{
    m_dynamicHintWidget->setStyleSheet( QString( "QLabel { background-color: %1; color: %2; } " )
                                        .arg( PaletteHandler::highlightColor().name() )
                                        .arg( palette.highlightedText().color().name() ) );

}

void
Playlist::Dock::playlistProviderAdded( Playlists::PlaylistProvider *provider, int category )
{
    if( category != PlaylistManager::UserPlaylist )
        return;

    debug() << "Adding provider: " << provider->objectName();
    Playlists::UserPlaylistProvider *userProvider =
            dynamic_cast<Playlists::UserPlaylistProvider *>(provider);
    if( userProvider == 0 )
        return;
    QAction *action = new KAction( userProvider->icon(), i18n("&Save playlist to \"%1\"", provider->prettyName() ), this );
    action->setData( QVariant::fromValue( QWeakPointer<Playlists::UserPlaylistProvider>( userProvider ) ) );
    m_saveActions->addAction( provider->objectName(), action );

    m_savePlaylistMenu->addAction( action );
    connect( action, SIGNAL( triggered(bool) ), SLOT( slotSaveCurrentPlaylist() ) );
}

void
Playlist::Dock::playlistProviderRemoved( Playlists::PlaylistProvider *provider, int category )
{
    if( category != PlaylistManager::UserPlaylist )
        return;

    QAction *action = m_saveActions->action( provider->objectName() );
    if( action )
        m_savePlaylistMenu->removeAction( action );
}

void
Playlist::Dock::slotSaveCurrentPlaylist()
{
    DEBUG_BLOCK

    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == 0 )
        return;

    QWeakPointer<Playlists::UserPlaylistProvider> pointer = action->data().value<Playlists::UserPlaylistProvider>;
    Playlists::UserPlaylistProvider* provider = pointer.data();

    The::playlistManager()->save( The::playlist()->tracks(), QString(), provider );
}

void
Playlist::Dock::showActiveTrack()
{
    ensurePolish();
    m_playlistView->scrollToActiveTrack();
}

void
Playlist::Dock::showDynamicHint( bool enabled ) // slot
{
    DEBUG_BLOCK

    if( enabled )
        m_dynamicHintWidget->show();
    else
        m_dynamicHintWidget->hide();
}

void
Playlist::Dock::clearFilterIfActive() // slot
{
    DEBUG_BLOCK
    KConfigGroup config = Amarok::config( "Playlist Search" );
    bool filterActive = config.readEntry( "ShowOnlyMatches", true );

    if( filterActive )
        m_searchWidget->slotFilterClear();
}
