/***************************************************************************
 * Copyright 2007-2008  Seb Ruiz <ruiz@kde.org>                            *
 *                                                                         *
 * This program is free software; you can redistribute it and/or           *
 * modify it under the terms of the GNU General Public License as          *
 * published by the Free Software Foundation; either version 2 of          *
 * the License or (at your option) version 3 or any later version          *
 * accepted by the membership of KDE e.V. (or its successor approved       *
 * by the membership of KDE e.V.), which shall act as a proxy              *
 * defined in Section 14 of version 3 of the license.                      *
 *                                                                         *
 * This program is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "PlaylistDropVis.h"

#include "Debug.h"
#include "PlaylistGraphicsItem.h"
#include "PlaylistGraphicsView.h"
#include "playlist/PlaylistModel.h"

#include <KApplication>

#include <QPen>

QHash<GraphicsView*, DropVis*> Playlist::DropVis::s_instanceHash;

Playlist::DropVis::DropVis( GraphicsView *parent ) : QGraphicsLineItem( 0 )
{
    debug() << "Creating Playlist Drop Indicator with parent at" << ( void* )parent;

    QPalette p = KApplication::palette();
    QColor color = p.color( QPalette::Highlight );

    QPen pen( color, 3, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin );
    setAcceptDrops( true );
    setPen( pen );
    setZValue( 1000 );

    m_playlistView = parent;
}

DropVis*
Playlist::DropVis::instance( GraphicsView* parent )
{
    if ( !parent )
        return 0;
    if ( !s_instanceHash.contains( parent ) )
        s_instanceHash.insert( parent, new DropVis( parent ) );
    return s_instanceHash.value( parent );
}

void
Playlist::DropVis::destroy( GraphicsView* parent )
{
    if ( s_instanceHash.contains( parent ) )
    {
        delete s_instanceHash.take( parent );
    }
}

void
Playlist::DropVis::show( qreal yPosition )
{
    qreal width = m_playlistView->viewport()->size().width();
    setLine( 0, 0, width, 0 );
    setPos( QPointF( 0, yPosition ) );
    QGraphicsItem::show();
}

void
Playlist::DropVis::show( GraphicsItem *item, bool showBelow )
{
    if ( !scene() )
        return;

    qreal yPosition = 0;
    // if we have an item to place it above, then move the indicator,
    // otherwise use the top of the scene
    if ( item )
    {
        yPosition = item->pos().y();
        if ( showBelow )
            yPosition += item->boundingRect().height();
    }
    else // place indicator at end of track listing
    {
        const QList<GraphicsItem*> tracks = m_playlistView->tracks();
        GraphicsItem *below = 0;
        if ( !tracks.isEmpty() && ( below = tracks.last() ) )
        {
            yPosition = below->pos().y() + below->boundingRect().height();
        }
    }
    show( yPosition );
}

