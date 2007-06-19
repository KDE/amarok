/*
 *  Copyright (c) 2007 Jeff Mitchell <kde-dev@emailgoeshere.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


#include "debug.h"
#include "PopupDropperScene.h"
#include "PopupDropperView.h"

#include <QGraphicsScene>

using namespace PopupDropperNS;

PopupDropperScene::PopupDropperScene( QObject* parent )
                    : QGraphicsScene( parent )
                    , m_fadeInTL( 300, this )
                    , m_fadeOutTL( 300, this )
                    , m_spinInTL( 300, this )
{
    DEBUG_BLOCK
    m_fadeInTL.setFrameRange( 0, 10 );
    m_fadeOutTL.setFrameRange( 0, 10 );
    m_spinInTL.setFrameRange( 0, 10 );
}

PopupDropperScene::~PopupDropperScene()
{
    DEBUG_BLOCK
}

void
PopupDropperScene::setPDV( PopupDropperView *pdv  )
{
    DEBUG_BLOCK
    if( !pdv ) return;
    m_pdv = pdv;
    connect( &m_fadeInTL, SIGNAL( frameChanged(int) ), m_pdv, SLOT( setTransInValue(int) ) );
    connect( &m_fadeOutTL, SIGNAL( frameChanged(int) ), m_pdv, SLOT( setTransOutValue(int) ) );
    connect( &m_fadeOutTL, SIGNAL( finished() ), this, SLOT( pdvHidden() ) );
}

void
PopupDropperScene::startPDV()
{
    DEBUG_BLOCK
    m_pdv->show();
    m_fadeInTL.start();
    m_spinInTL.start();
}

void
PopupDropperScene::stopPDV()
{
    DEBUG_BLOCK
    m_fadeOutTL.start();
}

//SLOT
void
PopupDropperScene::pdvHidden()
{
    DEBUG_BLOCK
    m_pdv->hide();
    //delete all items we've added -- not children()!
}

#include "PopupDropperScene.moc"

