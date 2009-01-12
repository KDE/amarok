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
#include "Debug.h"
#include "MainWindow.h"
#include "PlaylistDefines.h"
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
    new LayoutConfigWidget( this );

    m_sortBox = new QComboBox( this );
    m_sortBox->insertItem( 0, "Album", Album);
    m_sortBox->insertItem( 1, "AlbumArtist", Album);
    m_sortBox->insertItem( 2, "Artist", Artist );
    m_sortBox->insertItem( 3, "Bitrate", Bitrate );
    m_sortBox->insertItem( 4, "Bpm", Bpm );
    m_sortBox->insertItem( 5, "Comment", Comment );
    m_sortBox->insertItem( 6, "Composer", Composer );
    m_sortBox->insertItem( 7, "Directory", Directory );
    m_sortBox->insertItem( 8, "DiscNumber", DiscNumber );
    m_sortBox->insertItem( 9, "Filename", Filename );
    m_sortBox->insertItem( 10, "Filesize", Filesize );
    m_sortBox->insertItem( 11, "Genre", Genre );
    m_sortBox->insertItem( 12, "LastPlayed", LastPlayed );
    m_sortBox->insertItem( 13, "Length", Length );
    m_sortBox->insertItem( 14, "Mood", Mood );
    m_sortBox->insertItem( 15, "PlayCount", PlayCount );
    m_sortBox->insertItem( 16, "Rating", Rating );
    m_sortBox->insertItem( 17, "SampleRate", SampleRate );
    m_sortBox->insertItem( 18, "Score", Score );
    m_sortBox->insertItem( 29, "Source", Source );
    m_sortBox->insertItem( 30, "Title", Title );
    m_sortBox->insertItem( 31, "TrackNumber", TrackNumber );
    m_sortBox->insertItem( 32, "Type", Type );
    m_sortBox->insertItem( 33, "Year", Year );

    connect( m_sortBox, SIGNAL( activated( int ) ), this, SLOT( sort( int ) ) );
    
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

void Playlist::Widget::sort( int index )
{
    DEBUG_BLOCK
    int field = m_sortBox->itemData( index ).toInt();
    debug() << "Field: " << field;
    The::playlistModel()->sort( field );
}


