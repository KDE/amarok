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

class QGraphicsScene;

namespace PlaylistNS {

    class Item
    {
        public:
            Item( Meta::TrackPtr track );
            ~Item();
            Meta::TrackPtr track() const { return m_track; }
            QGraphicsScene* scene() const;
        private:
            Meta::TrackPtr m_track;
            QGraphicsScene* m_scene;
    };

}

Q_DECLARE_METATYPE( PlaylistNS::Item )

#endif
