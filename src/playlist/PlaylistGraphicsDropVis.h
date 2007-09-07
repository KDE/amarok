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

            void showAboveItem( Playlist::GraphicsItem *item )
            {
                qreal width = item->boundingRect().width();
                setLine( 0, 0, width, 0 );

                QPointF itemPos = item->pos();
                itemPos.setY( itemPos.y() - 1 );
                setPos( itemPos );
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

