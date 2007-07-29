/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#ifndef AMAROK_PLAYLIST_ITEMSCENE_H
#define AMAROK_PLAYLIST_ITEMSCENE_H

#include "meta.h"

#include <QGraphicsScene>

class QFontMetricsF;

namespace PlaylistNS {

    class ItemScene : public QGraphicsScene
    {
        public:
            ItemScene( Meta::TrackPtr track );
            void resize( int totalWidth = -1 );
    
            static qreal height() { return s_height; }
        private:
            Meta::TrackPtr m_track;

            QGraphicsTextItem* m_topLeftText;
            QGraphicsTextItem* m_bottomLeftText;
            QGraphicsTextItem* m_topRightText;
            QGraphicsTextItem* m_bottomRightText;

            static const qreal ALBUM_WIDTH;
            static const qreal MARGIN;
            static qreal s_height;
            static QFontMetricsF* s_fm;
    };

}

#endif