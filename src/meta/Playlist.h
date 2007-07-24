/***************************************************************************
 * copyright: (C) 2007 Bart Cerneels <bart.cerneels@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#ifndef AMAROK_META_PLAYLIST_H
#define AMAROK_META_PLAYLIST_H

#include "amarok_export.h"
#include "meta.h"

#include <QList>
#include <QMetaType>
#include <QPixmap>
#include <QSet>
#include <QSharedData>
#include <QString>

#include <ksharedptr.h>
#include <kurl.h>

namespace Meta
{

    class Playlist;

    typedef KSharedPtr<Playlist> PlaylistPtr;
    typedef QList<PlaylistPtr> PlaylistList;

    class AMAROK_EXPORT Playlist : public QSharedData
    {
        public:
            virtual ~Playlist() {}
            virtual QString name() const = 0;
            virtual QString prettyName() const = 0;
            virtual QString fullPrettyName() const { return prettyName(); };
            virtual QString sortableName() const { return prettyName(); };

            /** returns all tracks in this playlist */
            virtual TrackList tracks() = 0;
    };

};

Q_DECLARE_METATYPE( Meta::PlaylistPtr )
Q_DECLARE_METATYPE( Meta::PlaylistList )

#endif
