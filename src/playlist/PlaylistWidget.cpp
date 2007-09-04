/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#include "PlaylistGraphicsView.h"
#include "PlaylistHeader.h"
#include "PlaylistModel.h"
//#include "PlaylistView.h"
#include "PlaylistWidget.h"
#include "TheInstances.h"

#include <QHBoxLayout>

using namespace Playlist;

Widget::Widget( QWidget* parent )
    : QWidget( parent )
{
    QVBoxLayout* layout = new QVBoxLayout( this );
    layout->setContentsMargins(0,0,0,0);

    Playlist::HeaderWidget* header = new Playlist::HeaderWidget( this );

    Playlist::Model* playmodel = The::playlistModel();
    playmodel->init();
    playmodel->testData();
 //   Playlist::View* playView = new Playlist::View( this );
 //   playView->setModel( playmodel );
    Playlist::GraphicsView* playView = new Playlist::GraphicsView( this, playmodel );
    layout->setSpacing( 0 );
    layout->addWidget( header );
    layout->addWidget( playView );
}

#include "PlaylistWidget.moc"
