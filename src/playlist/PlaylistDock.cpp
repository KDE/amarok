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

#define DEBUG_PREFIX "PlaylistDock"

#include "PlaylistDock.h"

#include "ActionClasses.h"
#include "amarokconfig.h"
#include "App.h"
#include "core/support/Debug.h"
#include "layouts/LayoutManager.h"
#include "MainWindow.h"
#include "navigators/NavigatorConfigAction.h"
#include "PaletteHandler.h"
#include "PlaylistController.h"
#include "PlaylistDefines.h"
#include "PlaylistInfoWidget.h"
#include "PlaylistManager.h"
#include "PlaylistModelStack.h"
#include "PlaylistQueueEditor.h"
#include "PlaylistToolBar.h"
#include "ProgressiveSearchWidget.h"
#include "playlist/PlaylistActions.h"
#include "core-impl/playlists/providers/user/UserPlaylistProvider.h"
#include "widgets/HorizontalDivider.h"

#include <KActionMenu>
#include <KStandardDirs>
#include <KToolBarSpacerAction>
#include <KVBox>

#include <QLabel>
#include <QToolBar>
#include <QHBoxLayout>

Playlist::Dock::Dock( QWidget* parent )
    : AmarokDockWidget( i18n( "&Playlist" ), parent )
    , m_barBox( 0 )
{
    DEBUG_BLOCK

    setObjectName( "Playlist dock" );
    setAllowedAreas( Qt::AllDockWidgetAreas );
}

Playlist::PrettyListView *
Playlist::Dock::currentView()
{
    ensurePolish();
    return m_playlistView;
}

Playlist::SortWidget *
Playlist::Dock::sortWidget()
{
    ensurePolish();
    return m_sortWidget;
}

Playlist::ProgressiveSearchWidget *
Playlist::Dock::searchWidget()
{
    ensurePolish();
    return m_searchWidget;
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
    connect( The::playlistActions(), SIGNAL( navigatorChanged() ),
             SLOT( showDynamicHint() ) );
    m_dynamicHintWidget = new QLabel( i18n( "Dynamic Mode Enabled" ), m_mainWidget );
    m_dynamicHintWidget->setAlignment( Qt::AlignCenter );

    QFont dynamicHintWidgetFont = m_dynamicHintWidget->font();
    dynamicHintWidgetFont.setPointSize( dynamicHintWidgetFont.pointSize() + 1 );
    m_dynamicHintWidget->setFont( dynamicHintWidgetFont );

    showDynamicHint();

    paletteChanged( App::instance()->palette() );
    connect( The::paletteHandler(), SIGNAL( newPalette( const QPalette& ) ),
             SLOT( paletteChanged( const QPalette &  ) ) );

    QWidget * layoutHolder = new QWidget( m_mainWidget );

    QVBoxLayout* mainPlaylistlayout = new QVBoxLayout( layoutHolder );
    mainPlaylistlayout->setContentsMargins( 0, 0, 0, 0 );

    m_playlistView = new PrettyListView();
    m_playlistView->show();

    connect( m_searchWidget, SIGNAL( filterChanged( const QString &, int, bool ) ),
             m_playlistView, SLOT( find( const QString &, int, bool ) ) );
    connect( m_searchWidget, SIGNAL( next( const QString &, int ) ),
             m_playlistView, SLOT( findNext( const QString &, int ) ) );
    connect( m_searchWidget, SIGNAL( previous( const QString &, int ) ),
             m_playlistView, SLOT( findPrevious( const QString &, int ) ) );
    connect( m_searchWidget, SIGNAL( filterCleared() ),
             m_playlistView, SLOT( clearSearchTerm() ) );
    connect( m_searchWidget, SIGNAL( showOnlyMatches( bool ) ),
             m_playlistView, SLOT( showOnlyMatches( bool ) ) );
    connect( m_searchWidget, SIGNAL( activateFilterResult() ),
             m_playlistView, SLOT( playFirstSelected() ) );
    connect( m_searchWidget, SIGNAL( downPressed() ), m_playlistView, SLOT( downOneTrack() ) );
    connect( m_searchWidget, SIGNAL( upPressed() ), m_playlistView, SLOT( upOneTrack() ) );

    connect( The::mainWindow(), SIGNAL( switchQueueStateShortcut() ),
             m_playlistView, SLOT( switchQueueState() ) );

    KConfigGroup searchConfig = Amarok::config("Playlist Search");
    m_playlistView->showOnlyMatches( searchConfig.readEntry( "ShowOnlyMatches", false ) );

    connect( m_playlistView, SIGNAL( found() ), m_searchWidget, SLOT( match() ) );
    connect( m_playlistView, SIGNAL( notFound() ), m_searchWidget, SLOT( noMatch() ) );

    connect( LayoutManager::instance(), SIGNAL( activeLayoutChanged() ),
             m_playlistView, SLOT( reset() ) );

    mainPlaylistlayout->setSpacing( 0 );
    mainPlaylistlayout->addWidget( m_playlistView );

    ModelStack::instance(); //This also creates the Controller.

    { // START: Playlist toolbar
        // action toolbar
        m_barBox = new KHBox( m_mainWidget );
        m_barBox->setObjectName( "PlaylistBarBox" );
        m_barBox->setContentsMargins( 0, 0, 0, 0 );
        m_barBox->setFixedHeight( 36 );

        // Use QToolBar instead of KToolBar, see bug 228390
        Playlist::ToolBar *plBar = new Playlist::ToolBar( m_barBox );
        plBar->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Preferred );
        plBar->setMovable( false );


        QActionGroup *playlistActions = new QActionGroup( m_mainWidget );
        playlistActions->addAction( Amarok::actionCollection()->action( "playlist_clear" ) );

        m_savePlaylistMenu = new KActionMenu( KIcon( "document-save-amarok" ),
                                              i18n("&Save Current Playlist"), m_mainWidget );

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

        playlistActions->addAction( m_savePlaylistMenu );

        playlistActions->addAction( Amarok::actionCollection()->action( "playlist_undo" ) );
        //redo action can be accessed from menu > Playlist

        playlistActions->addAction( Amarok::actionCollection()->action( "show_active_track" ) );

        plBar->addCollapsibleActions( playlistActions );

        NavigatorConfigAction *navigatorConfig = new NavigatorConfigAction( m_mainWidget );
        plBar->addAction( navigatorConfig );

        QToolButton *toolButton =
                qobject_cast<QToolButton *>(plBar->widgetForAction( navigatorConfig ) );
        if( toolButton )
            toolButton->setPopupMode( QToolButton::InstantPopup );

        plBar->addAction( new KToolBarSpacerAction( m_mainWidget ) );

        // label widget
        new PlaylistInfoWidget( m_barBox );
    } // END Playlist Toolbar

    //set correct colors
    paletteChanged( QApplication::palette() );

    // If it is active, clear the search filter before replacing the playlist. Fixes Bug #200709.
    connect( The::playlistController(), SIGNAL( replacingPlaylist() ),
             SLOT( clearFilterIfActive() ) );

}

QSize
Playlist::Dock::sizeHint() const
{
    return QSize( static_cast<QWidget*>( parent() )->size().width() / 4 , 300 );
}


void
Playlist::Dock::paletteChanged( const QPalette &palette )
{
    m_dynamicHintWidget->setStyleSheet(
                QString( "QLabel { background-color: %1; color: %2; ; border-radius: 3px; } " )
                                .arg( PaletteHandler::highlightColor().name() )
                                .arg( palette.highlightedText().color().name() )
                        );
    if( m_barBox )
        m_barBox->setStyleSheet(
                    QString( "QFrame#PlaylistBarBox { border: 1px ridge %1; " \
                             "background-color: %2; color: %3; border-radius: 3px; }" \
                             "QLabel { color: %3; }" )
                            .arg( palette.color( QPalette::Window ).name() )
                            .arg( The::paletteHandler()->highlightColor().name() )
                            .arg( palette.color( QPalette::HighlightedText ).name() )
                    );

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
    QAction *action = new KAction( userProvider->icon(),
                                   i18n("&Save playlist to \"%1\"", provider->prettyName() ),
                                   this );
    action->setData( QVariant::fromValue(
            QWeakPointer<Playlists::UserPlaylistProvider>( userProvider ) ) );
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

    QWeakPointer<Playlists::UserPlaylistProvider> pointer =
            action->data().value< QWeakPointer<Playlists::UserPlaylistProvider> >();
    Playlists::UserPlaylistProvider* provider = pointer.data();

    const Meta::TrackList tracks = The::playlist()->tracks();
    The::playlistManager()->save( tracks, Amarok::generatePlaylistName( tracks ), provider );
}

void
Playlist::Dock::slotEditQueue()
{
    if( m_playlistQueueEditor ) {
        m_playlistQueueEditor.data()->raise();
        return;
    }
    m_playlistQueueEditor = new PlaylistQueueEditor;
    m_playlistQueueEditor.data()->setAttribute( Qt::WA_DeleteOnClose );
    m_playlistQueueEditor.data()->show();
}

void
Playlist::Dock::showActiveTrack()
{
    ensurePolish();
    m_playlistView->scrollToActiveTrack();
}

void
Playlist::Dock::showDynamicHint() // slot
{
    DEBUG_BLOCK

    if( AmarokConfig::dynamicMode() )
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
