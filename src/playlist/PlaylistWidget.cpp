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
//#include "view/graphic/PlaylistClassicView.h"
#include "view/graphic/PlaylistGraphicsView.h"
#include "PlaylistController.h"
#include "view/listview/PrettyListView.h"
#include "PlaylistHeader.h"
#include "statusbar/selectLabel.h"
#include "ToolBar.h"
#include "PlaylistModel.h"
#include "widgets/Widget.h"

#include <KToolBarSpacerAction>

#include <QHBoxLayout>
#include <QTreeView>
#include <QStackedWidget>
#include <QTime>

Playlist::Widget::Widget( QWidget* parent )
        : KVBox( parent )
{
    setContentsMargins( 1, 1, 1, 1 );

    Amarok::Widget * layoutHolder = new Amarok::Widget( this );

    layoutHolder->setMinimumWidth( 100 );
    layoutHolder->setMinimumHeight( 200 );


    QVBoxLayout* mainPlaylistlayout = new QVBoxLayout( layoutHolder );
    mainPlaylistlayout->setContentsMargins( 0, 0, 0, 0 );

    //Playlist::HeaderWidget* header = new Playlist::HeaderWidget( layoutHolder );

    QWidget* playView = new PrettyListView( this );
    //playView->setFrameShape( QFrame::NoFrame );  // Get rid of the redundant border
    //playView->setModel();
    playView->show();
    m_playlistView = playView;

    // Classic View disabled for 2.0
    //Playlist::ClassicView * clasicalPlaylistView = new Playlist::ClassicView( this );

    mainPlaylistlayout->setSpacing( 0 );
    //mainPlaylistlayout->addWidget( header );
    mainPlaylistlayout->addWidget( playView );

    m_stackedWidget = new Amarok::StackedWidget( this );

    m_stackedWidget->addWidget( layoutHolder );
    //m_stackedWidget->addWidget( clasicalPlaylistView );

    m_stackedWidget->setCurrentIndex( 0 );

    KHBox *barBox = new KHBox( this );
    barBox->setMargin( 6 );
    //QHBoxLayout *barAndLength = new QHBoxLayout( barBox );

    //barAndLength->addStretch();

    KToolBar *plBar = new Amarok::ToolBar( barBox );
    //barAndLength->addWidget( plBar );
    plBar->setObjectName( "PlaylistToolBar" );


    Model::instance();

    KAction * action = new KAction( KIcon( "view-media-playlist-amarok" ), i18nc( "switch view", "Switch Playlist &View" ), this );
    connect( action, SIGNAL( triggered( bool ) ), this, SLOT( switchView() ) );
    Amarok::actionCollection()->addAction( "playlist_switch", action );

    // the Controller ctor creates the undo/redo actions that we use below, so we want
    // to make sure that it's been constructed and the the actions registered
    Controller::instance();

    {
        //START Playlist toolbar
//         plBar->setToolButtonStyle( Qt::ToolButtonIconOnly );
        plBar->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Preferred );
        plBar->setIconDimensions( 22 );
        plBar->setMovable( false );
        plBar->addAction( new KToolBarSpacerAction( this ) );
        plBar->addAction( Amarok::actionCollection()->action( "playlist_clear" ) );
        plBar->addSeparator();
        plBar->addAction( Amarok::actionCollection()->action( "playlist_undo" ) );
        plBar->addAction( Amarok::actionCollection()->action( "playlist_redo" ) );
        plBar->addSeparator();
        plBar->addAction( Amarok::actionCollection()->action( "playlist_save" ) );
        plBar->addAction( Amarok::actionCollection()->action( "playlist_export" ) );
//TODO: Re add when these work...
//         plBar->addSeparator();

// //         plBar->addWidget( new SelectLabel( static_cast<Amarok::SelectAction*>( Amarok::actionCollection()->action("repeat") ), plBar ) );
// //         plBar->addWidget( new SelectLabel( static_cast<Amarok::SelectAction*>( Amarok::actionCollection()->action("random_mode") ), plBar ) );
//         plBar->addSeparator();

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
    const QString size = Amarok::config( "Panel_Sizes" ).readEntry( "playlist_width" );

    if ( size.isNull() )
        return QSize( static_cast<QWidget*>( parent() )->size().width() / 4 , 300 );
    else
        return QSize( size.toInt(), 300 );
}

void
Playlist::Widget::switchView()
{
    m_stackedWidget->setCurrentIndex(( m_stackedWidget->currentIndex() + 1 ) % 2 );
}


