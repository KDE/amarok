/***************************************************************************
 * copyright            : (C) 2007 Seb Ruiz <ruiz@kde.org>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#ifndef AMAROK_PLAYLISTDROPVIS_H
#define AMAROK_PLAYLISTDROPVIS_H

#include "debug.h"
#include "PlaylistGraphicsItem.h"

#include <QGraphicsLineItem>
#include <QGraphicsScene>
#include <QPen>

namespace Playlist
{
    class PlaylistGraphicsView;
    class DropVis : public QGraphicsLineItem
    {
        // we can only have one drop visualiser so it is a singleton class
        public:
            static DropVis *instance()
            {
                if( !s_instance )
                    s_instance = new DropVis();
                return s_instance;
            }
            ~DropVis() { }

            void showDropIndicator( Playlist::GraphicsItem *above = 0 )
            {
                if( !scene() )
                    return;

                QPointF indicatorPosition( 0.0, 0.0 );
                // if we have an item to place it above, then move the indicator, 
                // otherwise use the top of the scene
                if( above )
                {
                    indicatorPosition = above->pos();
                    indicatorPosition.setY( indicatorPosition.y() - 5 );
                }

                setLine( 0, 0, scene()->width(), 0 );
                setPos( indicatorPosition );
                show();
            }

        private:
            friend class PlaylistGraphicsView;
            DropVis( QGraphicsItem *parent = 0 )
                : QGraphicsLineItem( parent )
            {
                debug() << "Creating Playlist Drop Indicator";
                QPen pen( Qt::green, 3, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin );
                setPen( pen );
                setZValue( 1000 );
            }

            static DropVis *s_instance;
    };
}

#endif

