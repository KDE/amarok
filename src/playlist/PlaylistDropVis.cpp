/***************************************************************************
 * copyright            : (C) 2007 Seb Ruiz <ruiz@kde.org>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#include "PlaylistDropVis.h"

#include "debug.h"
#include "PlaylistGraphicsItem.h"
#include "PlaylistGraphicsView.h"
#include "PlaylistModel.h"
#include "TheInstances.h"

#include <QPen>
#include <QGraphicsScene>

Playlist::DropVis* Playlist::DropVis::s_instance = 0;

Playlist::DropVis::DropVis( QGraphicsItem *parent )
    : QGraphicsLineItem( parent )
{
    debug() << "Creating Playlist Drop Indicator";
    QPen pen( Qt::green, 3, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin );
    setPen( pen );
    setZValue( 1000 );
}

Playlist::DropVis*
Playlist::DropVis::instance()
{
    if( !s_instance )
        s_instance = new Playlist::DropVis();
    return s_instance;
}

void
Playlist::DropVis::show( qreal yPosition )
{
    qreal width = The::playlistView()->viewport()->size().width();
    setLine( 0, 0, width, 0 );
    setPos( QPointF( 0, yPosition ) );
    QGraphicsItem::show();
}

void
Playlist::DropVis::show( Playlist::GraphicsItem *above )
{
    if( !scene() )
        return;
    
    qreal yPosition = 0;
    // if we have an item to place it above, then move the indicator,
    // otherwise use the top of the scene
    if( above )
    {
        yPosition = above->pos().y() - 5;
    }
    else // place indicator at end of track listing
    {
        const QList<Playlist::GraphicsItem*> tracks = The::playlistView()->tracks();
        Playlist::GraphicsItem *below = 0;
        if( !tracks.isEmpty() && ( below = tracks.last() ) )
        {
            yPosition = below->pos().y() + below->boundingRect().height();
        }
    }
    show( yPosition );    
}

