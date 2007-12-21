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
#include "Meta.h"
#include "Capability.h"

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

            /** returns all tracks in this playlist */
            virtual TrackList tracks() = 0;

            /* the following has been copied from Meta.h
            * it is my hope that we can integrate Playlists
            * better into the rest of the Meta framework someday ~Bart Cerneels
            * TODO: Playlist : public MetaBase
            */
            virtual bool hasCapabilityInterface( Meta::Capability::Type type ) const = 0;

            virtual Capability* asCapabilityInterface( Capability::Type type ) = 0;

            /**
             * Retrieves a specialized interface which represents a capability of this
             * MetaBase object.
             *
             * @returns a pointer to the capability interface if it exists, 0 otherwise
             */
            template <class CapIface> CapIface *as()
            {
                Meta::Capability::Type type = CapIface::capabilityInterfaceType();
                Meta::Capability *iface = asCapabilityInterface(type);
                return qobject_cast<CapIface *>(iface);
            }

            /**
             * Tests if a MetaBase object provides a given capability interface.
             *
             * @returns true if the interface is available, false otherwise
             */
            template <class CapIface> bool is() const
            {
                return hasCapabilityInterface( CapIface::capabilityInterfaceType() );
            }
    };

}

Q_DECLARE_METATYPE( Meta::PlaylistPtr )
Q_DECLARE_METATYPE( Meta::PlaylistList )

#endif
