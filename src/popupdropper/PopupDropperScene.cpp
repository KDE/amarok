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

#include "PopupDropperScene.h"

#include "math.h"

#include "kstandarddirs.h"

#include "AppendTracksItem.h"
#include "debug.h"
#include "PopupDropperView.h"
#include "PopupDropperBaseItem.h"

#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QList>
#include <QRectF>
#include <QSvgRenderer>

using namespace PopupDropperNS;

static int SPIN_IN_FRAMES = 20;
static char APPEND_TO_PLAYLIST[] = "g12706";

PopupDropperScene::PopupDropperScene( QObject* parent )
                    : QGraphicsScene( parent )
                    , m_fadeInTL( 2000, this )
                    , m_fadeOutTL( 2000, this )
                    , m_spinInTL( 500, this )
                    , m_shown( false )
                    , m_renderer( KStandardDirs::locate( "data", "amarok/images/sources.svg" ) )
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
    connect( &m_fadeInTL, SIGNAL( finished() ), this, SLOT( pdvShown() ) );
    connect( &m_fadeOutTL, SIGNAL( frameChanged(int) ), m_pdv, SLOT( setTransOutValue(int) ) );
    connect( &m_fadeOutTL, SIGNAL( finished() ), this, SLOT( pdvHidden() ) );
    connect( m_pdv, SIGNAL( destroyMe() ), this, SLOT( pdvHidden() ) );
    connect( &m_spinInTL, SIGNAL( frameChanged(int) ), this, SLOT( updateIconSpinIn(int) ) );
}

void
PopupDropperScene::startPDV()
{
    DEBUG_BLOCK
    m_pdv->show();
    pdvShown();
}

void
PopupDropperScene::stopPDV()
{
    DEBUG_BLOCK
    pdvHidden();
}

//SLOT
void
PopupDropperScene::pdvShown()
{
    DEBUG_BLOCK
    PopupDropperBaseItem *temp;
    int totalItems = 5;
    for( int i = 1; i <= totalItems; ++i )
    {
        //temp = new PopupDropperBaseItem( i, totalItems );
        temp = new AppendTracksItem( i, totalItems, APPEND_TO_PLAYLIST, &m_renderer );
        //temp->setPos( width()/2, i * 1.0 / totalItems * height() );
        qreal center = totalItems / 2.0 + 0.5;
        qreal mywidth = i - center;
        mywidth = sqrt( pow( mywidth, 2 ) );
        mywidth = 1 - ( mywidth / center );
        qreal heighttemp = ( ( i - 1.0 ) / totalItems ) * height();
        qreal offset = ( ( height() / totalItems ) - ( 0.8 * ( height() / totalItems ) ) ) / 2;
        temp->setPos( mywidth * temp->boundingRect().width() , heighttemp + offset );
        temp->scale( 1.0 / SPIN_IN_FRAMES, 1.0 / SPIN_IN_FRAMES );
        temp->setScaledPercent( 1.0 / SPIN_IN_FRAMES );
        addItem( temp );
        temp->show();
    }
    m_spinInTL.start();
    m_shown = true;
}

//SLOT
void
PopupDropperScene::pdvHidden()
{
    DEBUG_BLOCK

    //delete all items we've added -- not children()!
    foreach( QGraphicsItem* item, items() ) {
        removeItem( item );
        delete item;
    }

    m_shown = false;
}

//SLOT
void
PopupDropperScene::updateIconSpinIn( int frame )
{
    DEBUG_BLOCK

    debug() << "frame number = " << frame << endl;
    if( frame == 0 )
        return;
    const qreal percentage = frame * 1.0 / SPIN_IN_FRAMES;
    debug() << "percentage being used: " << percentage << endl;

    foreach( QGraphicsItem* i, items() ) {
        PopupDropperBaseItem* item = static_cast<PopupDropperBaseItem *>( i );
        debug() << "currItems's scaledPercent() = " << item->scaledPercent() << endl;
        const qreal scalefactor = ( 1.0 / ( item->scaledPercent() * ( 1.0 / percentage ) ) );
        debug() << "Scaling by " << scalefactor << endl;
        item->scale( scalefactor, scalefactor );
        item->setScaledPercent( percentage );
    }

    m_pdv->repaint();
}

#include "PopupDropperScene.moc"

