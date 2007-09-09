/***************************************************************************
 * copyright            : (C) 2007 Seb Ruiz <ruiz@kde.org>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#ifndef AMAROK_PLAYLISTDROPVIS_H
#define AMAROK_PLAYLISTDROPVIS_H

#include <QGraphicsLineItem>
#include "PlaylistGraphicsItem.h"

namespace Playlist
{
    class DropVis : public QGraphicsLineItem
    {
        // we can only have one drop visualiser so it is a singleton class
        public:
            static DropVis *instance();
            ~DropVis() { }

            void showDropIndicator( qreal yPosition );
            void showDropIndicator( Playlist::GraphicsItem *above = 0 );

        private:
            DropVis( QGraphicsItem *parent = 0 );

            static DropVis *s_instance;
    };
}

#endif

