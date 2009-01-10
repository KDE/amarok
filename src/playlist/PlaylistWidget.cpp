/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

#include "PlaylistWidget.h"

#include "ActionClasses.h"
#include "App.h"
#include "MainWindow.h"
#include "view/graphic/PlaylistGraphicsView.h"
#include "PlaylistController.h"
#include "view/listview/PrettyListView.h"
#include "view/listview/LayoutManager.h"
#include "view/listview/LayoutConfigWidget.h"
#include "PlaylistHeader.h"
#include "ToolBar.h"
#include "PlaylistModel.h"
#include "widgets/Widget.h"
#include "widgets/ProgressiveSearchWidget.h"

#include <KToolBarSpacerAction>

#include <QHBoxLayout>

Playlist::Widget::Widget( QWidget* parent )
        : KVBox( parent )
{
    setContentsMargins( 1, 1, 1, 1 );

    m_searchWidget = new ProgressiveSearchWidget( this );
    
    Amarok::Widget * layoutHolder = new Amarok::Widget( this );

    layoutHolder->setMinimumWidth( 100 );
    layoutHolder->setMinimumHeight( 200 );

    QVBoxLayout* mainPlaylistlayout = new QVBoxLayout( layoutHolder );
    mainPlaylistlayout->setContentsMargins( 0, 0, 0, 0 );

    PrettyListView* playView = new PrettyListView( this );
    playView->show();
    m_playlistView = qobject_cast<QWidget*>( playView );

    connect( m_searchWidget, SIGNAL( filterChanged( const QString &, int ) ), playView, SLOT( find( const QString &, int ) ) );
    connect( m_searchWidget, SIGNAL( next( const QString &, int ) ), playView, SLOT( findNext( const QString &, int ) ) );
    connect( m_searchWidget, SIGNAL( previous( const QString &, int ) ), playView, SLOT( findPrevious( const QString &, int ) ) );
    connect( m_searchWidget, SIGNAL( filterCleared() ), playView, SLOT( clearSearchTerm() ) );
    connect( m_searchWidget, SIGNAL( showOnlyMatches( bool ) ), playView, SLOT( showOnlyMatches( bool ) ) );

    connect( playView, SIGNAL( found() ), m_searchWidget, SLOT( match() ) );
    connect( playView, SIGNAL( notFound() ), m_searchWidget, SLOT( noMatch() ) );

    connect( LayoutManager::instance(), SIGNAL( activeLayoutChanged() ), playView, SLOT( reset() ) );

    mainPlaylistlayout->setSpacing( 0 );
    mainPlaylistlayout->addWidget( playView );

    m_stackedWidget = new Amarok::StackedWidget( this );

    m_stackedWidget->addWidget( layoutHolder );

    m_stackedWidget->setCurrentIndex( 0 );

    KHBox *barBox = new KHBox( this );
    barBox->setMargin( 0 );

    KToolBar *plBar = new Amarok::ToolBar( barBox );
    plBar->setObjectName( "PlaylistToolBar" );

    Model::instance();

    KAction *action = new KAction( KIcon( "view-media-playlist-amarok" ), i18nc( "switch view", "Switch Playlist &View" ), this );
    connect( action, SIGNAL( triggered( bool ) ), this, SLOT( switchView() ) );
    Amarok::actionCollection()->addAction( "playlist_switch", action );

    // the Controller ctor creates the undo/redo actions that we use below, so we want
    // to make sure that it's been constructed and the the actions registered
    Controller::instance();

    {
        //START Playlist toolbar
        plBar->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Preferred );
        plBar->setIconDimensions( 22 );
        plBar->setMovable( false );
        plBar->addAction( new KToolBarSpacerAction( this ) );

        plBar->addAction( Amarok::actionCollection()->action( "playlist_clear" ) );
        
        //FIXME this action should go in ActionController, but we don't have any visibility to the view
        KAction *action = new KAction( KIcon( "music-amarok" ), i18n("Show active track"), this );
        connect( action, SIGNAL( triggered( bool ) ), playView, SLOT( scrollToActiveTrack() ) );
        plBar->addAction( action );

        plBar->addSeparator();
        plBar->addAction( Amarok::actionCollection()->action( "playlist_undo" ) );
        plBar->addAction( Amarok::actionCollection()->action( "playlist_redo" ) );
        plBar->addSeparator();
        plBar->addAction( Amarok::actionCollection()->action( "playlist_save" ) );
        plBar->addAction( Amarok::actionCollection()->action( "playlist_export" ) );

        // Alternate playlist view disabled for 2.0
        //plBar->addSeparator();
        //plBar->addAction( Amarok::actionCollection()->action( "playlist_switch") );
        plBar->addAction( new KToolBarSpacerAction( this ) );

    } //END Playlist Toolbar

    setFrameShape( QFrame::StyledPanel );
    setFrameShadow( QFrame::Sunken );
}

QSize
Playlist::Widget::sizeHint() const
{
    return QSize( static_cast<QWidget*>( parent() )->size().width() / 4 , 300 );
}

void
Playlist::Widget::switchView()
{
    m_stackedWidget->setCurrentIndex(( m_stackedWidget->currentIndex() + 1 ) % 2 );
}


