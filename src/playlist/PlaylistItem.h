/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#ifndef AMAROKPLAYLISTITEM_H
#define AMAROKPLAYLISTITEM_H

#include "meta.h"

#include <QMetaType>

class QFontMetricsF;
class QGraphicsScene;

namespace PlaylistNS {

    class Item
    {
        public:
            Item() : m_scene( 0 ) { }
            Item( Meta::TrackPtr track );
            ~Item();
            Meta::TrackPtr track() const { return m_track; }
            QGraphicsScene* scene( int totalWidth );

            static qreal height() { return s_height; }
        private:
            Meta::TrackPtr m_track;
            QGraphicsScene* m_scene;

            static const qreal ALBUM_WIDTH;
            static const qreal MARGIN;
            static qreal s_height;
            static QFontMetricsF* s_fm;
    };

}

Q_DECLARE_METATYPE( PlaylistNS::Item* )

#endif
