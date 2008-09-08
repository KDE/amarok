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
//#include "PlaylistClassicView.h"
#include "PlaylistGraphicsView.h"
#include "PlaylistHeader.h"
#include "PlaylistModel.h"
#include "statusbar/selectLabel.h"
#include "ToolBar.h"
#include "WidgetBackgroundPainter.h"
#include "widgets/Widget.h"

#include <KToolBarSpacerAction>

#include <QHBoxLayout>
#include <QTreeView>
#include <QStackedWidget>


using namespace Playlist;


Widget::Widget( QWidget* parent )
    : KVBox( parent )
{
    
    setContentsMargins( 1, 1, 1, 1 );

    Amarok::Widget * layoutHolder = new Amarok::Widget( this );

    layoutHolder->setMinimumWidth( 100 );
    layoutHolder->setMinimumHeight( 200 );
           

    QVBoxLayout* mainPlaylistlayout = new QVBoxLayout( layoutHolder );
    mainPlaylistlayout->setContentsMargins(0,0,0,0);

    //Playlist::HeaderWidget* header = new Playlist::HeaderWidget( layoutHolder );

    Playlist::Model* playModel = The::playlistModel();
    playModel->init();


    Playlist::GraphicsView* playView = The::playlistView();
    playView->setFrameShape( QFrame::NoFrame );  // Get rid of the redundant border
    playView->setModel( playModel );


    // Classic View disabled for 2.0
    //Playlist::ClassicView * clasicalPlaylistView = new Playlist::ClassicView( this );


    mainPlaylistlayout->setSpacing( 0 );
    //mainPlaylistlayout->addWidget( header );
    mainPlaylistlayout->addWidget( playView );

    m_stackedWidget = new  Amarok::StackedWidget( this );

    m_stackedWidget->addWidget( layoutHolder );
    //m_stackedWidget->addWidget( clasicalPlaylistView );

    m_stackedWidget->setCurrentIndex( 0 );

    KToolBar *plBar = new Amarok::ToolBar( this );
    plBar->setObjectName( "PlaylistToolBar" );

    KAction * action = new KAction( KIcon( "view-media-playlist-amarok" ), i18nc( "switch view", "Switch Playlist &View" ), this );
    connect( action, SIGNAL( triggered( bool ) ), this, SLOT( switchView() ) );
            Amarok::actionCollection()->addAction( "playlist_switch", action );


    { //START Playlist toolbar
//         plBar->setToolButtonStyle( Qt::ToolButtonIconOnly );
        plBar->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
        plBar->setIconDimensions( 22 );
        plBar->setMovable( false );
        plBar->addAction( new KToolBarSpacerAction( this ) );
        plBar->addAction( Amarok::actionCollection()->action( "playlist_clear") );
        plBar->addSeparator();
        plBar->addAction( Amarok::actionCollection()->action( "playlist_undo") );
        plBar->addAction( Amarok::actionCollection()->action( "playlist_redo") );
        plBar->addSeparator();
        plBar->addAction( Amarok::actionCollection()->action( "playlist_save") );
        plBar->addAction( Amarok::actionCollection()->action( "playlist_export") );
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

QSize Widget::sizeHint() const
{
    return QSize( static_cast<QWidget*>(parent())->size().width() / 4 , 300 );
}

void Widget::switchView()
{
    m_stackedWidget->setCurrentIndex( ( m_stackedWidget->currentIndex() + 1 ) % 2 );
}


#include "PlaylistWidget.moc"
