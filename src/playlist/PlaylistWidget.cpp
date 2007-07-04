/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#include "PlaylistHeader.h"
#include "PlaylistModel.h"
#include "PlaylistView.h"
#include "PlaylistWidget.h"
#include "TheInstances.h"

#include <QHBoxLayout>

using namespace PlaylistNS;

Widget::Widget( QWidget* parent )
    : QWidget( parent )
{
    QVBoxLayout* layout = new QVBoxLayout( this );

    PlaylistNS::HeaderWidget* header = new PlaylistNS::HeaderWidget( this );

    PlaylistNS::Model* playmodel = The::playlistModel();
    playmodel->init();
    playmodel->testData();
    PlaylistNS::View* playView = new PlaylistNS::View( this );
    playView->setModel( playmodel );

    layout->addWidget( header );
    layout->addWidget( playView );
}

#include "PlaylistWidget.moc"
