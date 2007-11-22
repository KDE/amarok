/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#ifndef AMAROKPLAYLISTITEM_H
#define AMAROKPLAYLISTITEM_H

#include "meta/Meta.h"

namespace Playlist {

    class Item
    {
        public:
            Item() { }
            Item( Meta::TrackPtr track );
            ~Item();
            Meta::TrackPtr track() const { return m_track; }

        private:
            Meta::TrackPtr m_track;
    };

}

Q_DECLARE_METATYPE( Playlist::Item* )

#endif
