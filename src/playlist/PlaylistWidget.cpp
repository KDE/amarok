/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#include "app.h"
#include "MainWindow.h" 
#include "PlaylistGraphicsView.h"
#include "PlaylistHeader.h"
#include "PlaylistModel.h"
//#include "PlaylistView.h"
#include "PlaylistWidget.h"
#include "TheInstances.h"

#include <QHBoxLayout>
#include <QTreeView>
#include <QStackedWidget>

#include <KAction>
#include <KIcon>
#include <KToolBar>


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

    KActionCollection* const ac = App::instance()->mainWindow()->actionCollection();

    KAction * action = new KAction( KIcon( Amarok::icon( "download" ) ), i18nc( "switch view", "&View" ), this );
    connect( action, SIGNAL( triggered( bool ) ), this, SLOT( switchView() ) );
    ac->addAction( "playlist_switch", action );

    KToolBar *plBar = App::instance()->mainWindow()->findChild<KToolBar *> ( "PlaylistToolBar" );

    if ( plBar != 0 ) {
        plBar->addSeparator();
        plBar->addAction( ac->action( "playlist_switch") );
        debug() << "PlaylistToolBar found! :-)";

    } else
        debug() << "PlaylistToolBar not found";

}

void Widget::switchView()
{
    m_stackedWidget->setCurrentIndex( ( m_stackedWidget->currentIndex() + 1 ) % 2 );
}



#include "PlaylistWidget.moc"
