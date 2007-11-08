/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "PlaylistClassicView.h"

#include "debug.h"
#include "PlaylistModel.h"



Playlist::ClassicView::ClassicView(QWidget * parent)
    : QTreeView( parent )
{
    DEBUG_BLOCK

    connect ( this, SIGNAL( activated ( const QModelIndex & ) ), this, SLOT( play(const QModelIndex & ) ) );   
    //connect ( this, SIGNAL( doubleClicked ( const QModelIndex & ) ), this, SLOT( play(const QModelIndex & ) ) );   
}

Playlist::ClassicView::~ ClassicView()
{
}



void Playlist::ClassicView::setModel( Playlist::Model *model )
{
    DEBUG_BLOCK

    m_model = model;

    QTreeView::setModel( model );

}

void Playlist::ClassicView::play(const QModelIndex & index)
{
    DEBUG_BLOCK

    debug() << " Here!!";
    m_model->play( index.row() );

}

#include "PlaylistClassicView.moc"




