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

#include "App.h"
#include "MainWindow.h"
#include "PaletteHandler.h"
#include "amarokconfig.h"
#include "amarokurls/AmarokUrl.h"
#include "core/support/Debug.h"
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistController.h"
#include "playlist/PlaylistDefines.h"
#include "playlist/PlaylistInfoWidget.h"
#include "playlist/PlaylistModelStack.h"
#include "playlist/PlaylistQueueEditor.h"
#include "playlist/PlaylistToolBar.h"
#include "playlist/ProgressiveSearchWidget.h"
#include "playlist/layouts/LayoutManager.h"
#include "playlist/navigators/NavigatorConfigAction.h"
#include "playlistmanager/PlaylistManager.h"
#include "core-impl/playlists/providers/user/UserPlaylistProvider.h"
#include "widgets/HorizontalDivider.h"
#include "widgets/BoxWidget.h"

#include <QActionGroup>
#include <QLabel>
#include <QToolBar>

#include <KActionMenu>
#include <KToolBarSpacerAction>

static const QString s_dynMode( QStringLiteral("dynamic_mode") );
static const QString s_repopulate( QStringLiteral("repopulate") );
static const QString s_turnOff( QStringLiteral("turn_off") );

Playlist::Dock::Dock( QWidget* parent )
    : AmarokDockWidget( i18n( "&Playlist" ), parent )
    , m_barBox( nullptr )
{
    setObjectName( QStringLiteral("Playlist dock") );
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
    m_mainWidget = new BoxWidget( true, this );
    setWidget( m_mainWidget );
    m_mainWidget->setContentsMargins( 0, 0, 0, 0 );
    m_mainWidget->setFrameShape( QFrame::NoFrame );
    m_mainWidget->setMinimumWidth( 200 );
    m_mainWidget->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Ignored );
    m_mainWidget->setFocus( Qt::ActiveWindowFocusReason );

    m_sortWidget = new Playlist::SortWidget( m_mainWidget );
    new HorizontalDivider( m_mainWidget );

    m_searchWidget = new Playlist::ProgressiveSearchWidget( m_mainWidget );

    // show visual indication of dynamic playlists  being enabled
    connect( The::playlistActions(), &Playlist::Actions::navigatorChanged,
             this, &Playlist::Dock::showDynamicHint );
    m_dynamicHintWidget = new QLabel( i18n( "<a href='%1'>Dynamic Mode</a> Enabled. "
        "<a href='%2'>Repopulate</a> | <a href='%3'>Turn off</a>", s_dynMode,
        s_repopulate, s_turnOff ), m_mainWidget );
    m_dynamicHintWidget->setAlignment( Qt::AlignCenter );
    m_dynamicHintWidget->setTextInteractionFlags( Qt::LinksAccessibleByKeyboard | Qt::LinksAccessibleByMouse );
    m_dynamicHintWidget->setMinimumSize( 1, 1 ); // so that it doesn't prevent playlist from shrinking
    connect( m_dynamicHintWidget, &QLabel::linkActivated, this, &Dock::slotDynamicHintLinkActivated );

    QFont dynamicHintWidgetFont = m_dynamicHintWidget->font();
    dynamicHintWidgetFont.setPointSize( dynamicHintWidgetFont.pointSize() + 1 );
    m_dynamicHintWidget->setFont( dynamicHintWidgetFont );


    paletteChanged( pApp->palette() );
    connect( The::paletteHandler(), &PaletteHandler::newPalette,
             this, &Playlist::Dock::paletteChanged );

    QWidget * layoutHolder = new QWidget( m_mainWidget );

    QVBoxLayout* mainPlaylistlayout = new QVBoxLayout( layoutHolder );
    mainPlaylistlayout->setContentsMargins( 0, 0, 0, 0 );

    m_playlistView = new PrettyListView();
    m_playlistView->show();

    connect( m_searchWidget, &Playlist::ProgressiveSearchWidget::filterChanged,
             m_playlistView, &Playlist::PrettyListView::find );
    connect( m_searchWidget, &Playlist::ProgressiveSearchWidget::next,
             m_playlistView, &Playlist::PrettyListView::findNext );
    connect( m_searchWidget, &Playlist::ProgressiveSearchWidget::previous,
             m_playlistView, &Playlist::PrettyListView::findPrevious );
    connect( m_searchWidget, &Playlist::ProgressiveSearchWidget::filterCleared,
             m_playlistView, &Playlist::PrettyListView::clearSearchTerm );
    connect( m_searchWidget, &Playlist::ProgressiveSearchWidget::showOnlyMatches,
             m_playlistView, &Playlist::PrettyListView::showOnlyMatches );
    connect( m_searchWidget, &Playlist::ProgressiveSearchWidget::activateFilterResult,
             m_playlistView, &Playlist::PrettyListView::playFirstSelected );
    connect( m_searchWidget, &Playlist::ProgressiveSearchWidget::downPressed, m_playlistView, &Playlist::PrettyListView::downOneTrack );
    connect( m_searchWidget, &Playlist::ProgressiveSearchWidget::upPressed, m_playlistView, &Playlist::PrettyListView::upOneTrack );

    connect( The::mainWindow(), &MainWindow::switchQueueStateShortcut,
             m_playlistView, &Playlist::PrettyListView::switchQueueState );

    KConfigGroup searchConfig = Amarok::config(QStringLiteral("Playlist Search"));
    m_playlistView->showOnlyMatches( searchConfig.readEntry( "ShowOnlyMatches", false ) );

    connect( m_playlistView, &Playlist::PrettyListView::found, m_searchWidget, &Playlist::ProgressiveSearchWidget::match );
    connect( m_playlistView, &Playlist::PrettyListView::notFound, m_searchWidget, &Playlist::ProgressiveSearchWidget::noMatch );

    connect( LayoutManager::instance(), &LayoutManager::activeLayoutChanged,
             m_playlistView, &Playlist::PrettyListView::reset );

    mainPlaylistlayout->setSpacing( 0 );
    mainPlaylistlayout->addWidget( m_playlistView );

    ModelStack::instance(); //This also creates the Controller.

    { // START: Playlist toolbar
        // action toolbar
        m_barBox = new BoxWidget( false, m_mainWidget );
        m_barBox->setObjectName( QStringLiteral("PlaylistBarBox") );
        m_barBox->setContentsMargins( 0, 0, 4, 0 );
        m_barBox->setFixedHeight( 36 );

        // Use QToolBar instead of KToolBar, see bug 228390
        Playlist::ToolBar *plBar = new Playlist::ToolBar( m_barBox );
        plBar->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Preferred );
        plBar->setMovable( false );

        QActionGroup *playlistActions = new QActionGroup( m_mainWidget );
        playlistActions->addAction( Amarok::actionCollection()->action( QStringLiteral("playlist_clear") ) );

        m_savePlaylistMenu = new KActionMenu( QIcon::fromTheme( QStringLiteral("document-save-amarok") ),
                                              i18n("&Save Current Playlist"), m_mainWidget );
        m_savePlaylistMenu->addAction( Amarok::actionCollection()->action( QStringLiteral("playlist_export") ) );

        m_saveActions = new KActionCollection( m_mainWidget );

        connect( m_savePlaylistMenu, &KActionMenu::triggered,
                 this, &Dock::slotSaveCurrentPlaylist );
        for( Playlists::PlaylistProvider *provider : The::playlistManager()->providersForCategory(
                            PlaylistManager::UserPlaylist ) )
        {
            playlistProviderAdded( provider, PlaylistManager::UserPlaylist );
        }

        connect( The::playlistManager(), &PlaylistManager::providerAdded,
                 this, &Dock::playlistProviderAdded );
        connect( The::playlistManager(), &PlaylistManager::providerRemoved,
                 this, &Dock::playlistProviderRemoved );

        playlistActions->addAction( m_savePlaylistMenu );

        playlistActions->addAction( Amarok::actionCollection()->action( QStringLiteral("playlist_undo") ) );
        //redo action can be accessed from menu > Playlist

        playlistActions->addAction( Amarok::actionCollection()->action( QStringLiteral("show_active_track") ) );

        plBar->addCollapsibleActions( playlistActions );

        m_navigatorConfig = new NavigatorConfigAction( m_mainWidget );
        plBar->addAction( m_navigatorConfig );

        QToolButton *toolButton =
                qobject_cast<QToolButton *>(plBar->widgetForAction( m_navigatorConfig ) );
        if( toolButton )
            toolButton->setPopupMode( QToolButton::InstantPopup );

        plBar->addAction( new KToolBarSpacerAction( m_mainWidget ) );

        // label widget
        new PlaylistInfoWidget( m_barBox );
    } // END Playlist Toolbar
    showDynamicHint();

    //set correct colors
    paletteChanged( QApplication::palette() );

    // If it is active, clear the search filter before replacing the playlist. Fixes Bug #200709.
    connect( The::playlistController(), &Playlist::Controller::replacingPlaylist,
             this, &Playlist::Dock::clearFilterIfActive );

}

QSize
Playlist::Dock::sizeHint() const
{
    return QSize( static_cast<QWidget*>( parent() )->size().width() / 4 , 300 );
}

void
Playlist::Dock::paletteChanged( const QPalette &palette )
{
    const QString backgroundColor = palette.color( QPalette::Active, QPalette::Mid ).name();
    const QString textColor = palette.color( QPalette::Active, QPalette::HighlightedText ).name();
    const QString linkColor = palette.color( QPalette::Active, QPalette::Link ).name();
    const QString ridgeColor = palette.color( QPalette::Active, QPalette::Window ).name();

    QString hintStyle( QStringLiteral( "QLabel { background-color: %1; color: %2; border-radius: 3px; } "
                       "a { color: %3; }" ) );
    hintStyle = hintStyle.arg( backgroundColor, textColor, linkColor );

    QString barStyle( QStringLiteral( "QFrame#PlaylistBarBox { border: 1px ridge %1; background-color: %2; "
                                      " color: %3; border-radius: 3px; } QLabel { color: %4; }" ) );
    barStyle = barStyle.arg( ridgeColor, backgroundColor, textColor, textColor );

    m_dynamicHintWidget->setStyleSheet( hintStyle );
    if( m_barBox )
        m_barBox->setStyleSheet( barStyle );

}

void
Playlist::Dock::playlistProviderAdded( Playlists::PlaylistProvider *provider, int category )
{
    if( category != PlaylistManager::UserPlaylist )
        return;

    debug() << "Adding provider: " << provider->prettyName();
    Playlists::UserPlaylistProvider *userProvider =
            dynamic_cast<Playlists::UserPlaylistProvider *>(provider);
    if( userProvider == nullptr )
        return;
    QAction *action = new QAction( userProvider->icon(),
                                   i18n("&Save playlist to \"%1\"", provider->prettyName() ),
                                   this );
    action->setData( QVariant::fromValue( QPointer<Playlists::UserPlaylistProvider>( userProvider ) ) );
    m_saveActions->addAction( QString::number( (qlonglong) userProvider ), action );

    // insert the playlist provider actions before "export"
    QAction* exportAction = Amarok::actionCollection()->action( QStringLiteral("playlist_export") );
    m_savePlaylistMenu->insertAction( exportAction, action );
    connect( action, &QAction::triggered, this, &Playlist::Dock::slotSaveCurrentPlaylist );
}

void
Playlist::Dock::playlistProviderRemoved( Playlists::PlaylistProvider *provider, int category )
{
    if( category != PlaylistManager::UserPlaylist )
        return;

    QAction *action = m_saveActions->action( QString::number( (qlonglong) provider ) );
    if( action )
        m_savePlaylistMenu->removeAction( action );
    else
        warning() << __PRETTY_FUNCTION__ << ": no save action for provider" << provider->prettyName();
}

void
Playlist::Dock::slotSaveCurrentPlaylist()
{
    DEBUG_BLOCK

    QAction *action = qobject_cast<QAction *>( QObject::sender() );
    if( action == nullptr )
        return;

    QPointer<Playlists::UserPlaylistProvider> pointer =
        action->data().value< QPointer<Playlists::UserPlaylistProvider> >();
    if ( !pointer ) // Probably default save was called, so pick the first saveAction (database)
        pointer = m_saveActions->actions().first()->data()
            .value< QPointer<Playlists::UserPlaylistProvider> >();
    if ( pointer ) {
        Playlists::UserPlaylistProvider* provider = pointer.data();

        const Meta::TrackList tracks = The::playlist()->tracks();
        The::playlistManager()->save( tracks, Amarok::generatePlaylistName( tracks ), provider );
    }
}

void
Playlist::Dock::slotEditQueue()
{
    if( m_playlistQueueEditor ) {
        m_playlistQueueEditor->raise();
        return;
    }
    m_playlistQueueEditor = new PlaylistQueueEditor;
    m_playlistQueueEditor->setAttribute( Qt::WA_DeleteOnClose );
    m_playlistQueueEditor->show();
}

void
Playlist::Dock::showActiveTrack()
{
    ensurePolish();
    m_playlistView->scrollToActiveTrack();
}

void
Playlist::Dock::editTrackInfo()
{
    m_playlistView->editTrackInformation();
}

void
Playlist::Dock::showDynamicHint() // slot
{
    DEBUG_BLOCK

    if( AmarokConfig::dynamicMode() )
    {
        m_dynamicHintWidget->show();
        m_navigatorConfig->setVisible( false );
    }
    else
    {
        m_dynamicHintWidget->hide();
        m_navigatorConfig->setVisible( true );
    }
}

void
Playlist::Dock::clearFilterIfActive() // slot
{
    DEBUG_BLOCK
    KConfigGroup config = Amarok::config( QStringLiteral("Playlist Search") );
    bool filterActive = config.readEntry( "ShowOnlyMatches", true );

    if( filterActive )
        m_searchWidget->slotFilterClear();
}

void
Playlist::Dock::slotDynamicHintLinkActivated( const QString &href )
{
    if( href == s_dynMode )
        AmarokUrl( QStringLiteral("amarok://navigate/playlists/dynamic category") ).run();
    else if( href == s_repopulate )
        The::playlistActions()->repopulateDynamicPlaylist();
    else if( href == s_turnOff )
        The::playlistActions()->enableDynamicMode( false );
}
