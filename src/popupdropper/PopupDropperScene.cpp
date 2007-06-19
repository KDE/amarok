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
                    , m_fadeinTL( 300, this )
                    , m_fadeoutTL( 300, this )
{
    DEBUG_BLOCK
    m_fadeinTL.setFrameRange( 0, 30 );
    m_fadeoutTL.setFrameRange( 0, 30 );
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
    connect( &m_fadeinTL, SIGNAL( frameChanged(int) ), m_pdv, SLOT( setTransInValue(int) ) );
    connect( &m_fadeoutTL, SIGNAL( frameChanged(int) ), m_pdv, SLOT( setTransOutValue(int) ) );
    connect( &m_fadeoutTL, SIGNAL( finished() ), this, SLOT( pdvHidden() ) );
}

void
PopupDropperScene::startPDV()
{
    DEBUG_BLOCK
    debug() << "m_pdv = " << m_pdv << endl;
    m_pdv->show();
    debug() << "got past the show" << endl;
    debug() << "shown: " << (m_pdv->isShown() ? "true" : "false") << endl;
    m_fadeinTL.start();
}

void
PopupDropperScene::stopPDV()
{
    DEBUG_BLOCK
    m_fadeoutTL.start();
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

