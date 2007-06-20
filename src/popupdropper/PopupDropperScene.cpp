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
#include "PopupDropperBaseItem.h"

#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QList>

using namespace PopupDropperNS;

static int SPIN_IN_FRAMES = 10;

PopupDropperScene::PopupDropperScene( QObject* parent )
                    : QGraphicsScene( parent )
                    , m_fadeInTL( 300, this )
                    , m_fadeOutTL( 300, this )
                    , m_spinInTL( 300, this )
{
    DEBUG_BLOCK
    m_fadeInTL.setFrameRange( 0, 10 );
    m_fadeOutTL.setFrameRange( 0, 10 );
    m_spinInTL.setFrameRange( 0, SPIN_IN_FRAMES );
    debug() << "SPIN IN FRAMES = " << SPIN_IN_FRAMES << endl;
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
    connect( &m_spinInTL, SIGNAL( frameChanged(int) ), this, SLOT( updateIconSpinIn(int) ) );
}

void
PopupDropperScene::startPDV()
{
    DEBUG_BLOCK
//     PopupDropperBaseItem *temp;
//     for( int i = 1; i <= 5; ++i )
//     {
//         temp = new PopupDropperBaseItem( i );
//         temp->scale( 100, 100 );
//         addItem( temp );
//         temp->show();
//     }
//     m_pdv->show();
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
    QGraphicsItem *temp;
    QList<QGraphicsItem *> itemlist = items();
    for( int i = 0; i < itemlist.size(); ++i )
    {
        temp = itemlist.at(i);
        removeItem( temp );
        delete temp;
    }

}

//SLOT
void
PopupDropperScene::updateIconSpinIn( int frame )
{
    DEBUG_BLOCK
    debug() << "frame number = " << frame << endl;
    QGraphicsItem * currItem;
    qreal percentage = frame * 100.0 / SPIN_IN_FRAMES;
    QList<QGraphicsItem *> itemlist = items();
    for( int i = 0; i < itemlist.size(); ++i )
    {
        currItem = itemlist.at(i);
        currItem->scale( percentage, percentage );
    }
    m_pdv->update();
}

#include "PopupDropperScene.moc"

