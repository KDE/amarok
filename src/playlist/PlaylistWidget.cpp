/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#include "app.h"
#include "actionclasses.h"
#include "MainWindow.h" 
#include "PlaylistGraphicsView.h"
#include "PlaylistHeader.h"
#include "PlaylistModel.h"
//#include "PlaylistView.h"
#include "PlaylistWidget.h"
#include "statusbar/selectLabel.h"
#include "TheInstances.h"
#include "toolbar.h"

#include <KToolBarSpacerAction>

#include <QHBoxLayout>
#include <QTreeView>
#include <QStackedWidget>


using namespace Playlist;


Widget::Widget( QWidget* parent )
    : QWidget( parent )
{

    QVBoxLayout* layout = new QVBoxLayout( this );
    layout->setContentsMargins(0,0,0,0);

    QWidget * layoutHolder = new QWidget( this );

    QVBoxLayout* mainPlaylistlayout = new QVBoxLayout( layoutHolder );
    mainPlaylistlayout->setContentsMargins(0,0,0,0);


    Playlist::HeaderWidget* header = new Playlist::HeaderWidget( layoutHolder );

    Playlist::Model* playModel = The::playlistModel();
    playModel->init();
    playModel->testData();

    Playlist::GraphicsView* playView = The::playlistView();
    playView->setModel( playModel );

    QTreeView * clasicalPlaylistView = new QTreeView( this );
    clasicalPlaylistView->setRootIsDecorated( false );
    clasicalPlaylistView->setAlternatingRowColors ( true );
    clasicalPlaylistView->setSelectionMode( QAbstractItemView::ExtendedSelection );
    clasicalPlaylistView->setModel( playModel );


    mainPlaylistlayout->setSpacing( 0 );
    mainPlaylistlayout->addWidget( header );
    mainPlaylistlayout->addWidget( playView );

    m_stackedWidget = new QStackedWidget( this );
    m_stackedWidget->addWidget( layoutHolder );
    m_stackedWidget->addWidget( clasicalPlaylistView );

    m_stackedWidget->setCurrentIndex( 0 );

    layout->setSpacing( 0 );
    layout->addWidget( m_stackedWidget );

    KToolBar *plBar = new Amarok::ToolBar( this );
    layout->addWidget( plBar );
    plBar->setObjectName( "PlaylistToolBar" );

    KAction * action = new KAction( KIcon( Amarok::icon( "download" ) ), i18nc( "switch view", "&View" ), this );
    connect( action, SIGNAL( triggered( bool ) ), this, SLOT( switchView() ) );
            Amarok::actionCollection()->addAction( "playlist_switch", action );


    { //START Playlist toolbar
        plBar->setToolButtonStyle( Qt::ToolButtonIconOnly );
        plBar->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
        plBar->setIconDimensions( 22 );
        plBar->setMovable( false );
        plBar->addAction( new KToolBarSpacerAction( this ) );
        plBar->addAction( Amarok::actionCollection()->action( "playlist_clear") );
        plBar->addAction( Amarok::actionCollection()->action( "playlist_save") );
        plBar->addSeparator();
        plBar->addAction( Amarok::actionCollection()->action( "playlist_undo") );
        plBar->addAction( Amarok::actionCollection()->action( "playlist_redo") );
        plBar->addSeparator();
        plBar->addWidget( new SelectLabel( static_cast<Amarok::SelectAction*>( Amarok::actionCollection()->action("repeat") ), plBar ) );
        plBar->addWidget( new SelectLabel( static_cast<Amarok::SelectAction*>( Amarok::actionCollection()->action("random_mode") ), plBar ) );
        plBar->addAction( new KToolBarSpacerAction( this ) );
        plBar->addSeparator();
        plBar->addAction( Amarok::actionCollection()->action( "playlist_switch") );
    } //END Playlist Toolbar
}

void Widget::switchView()
{
    m_stackedWidget->setCurrentIndex( ( m_stackedWidget->currentIndex() + 1 ) % 2 );
}



#include "PlaylistWidget.moc"
