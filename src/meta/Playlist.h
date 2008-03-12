/***************************************************************************
 * Copyright (C) 2007 Bart Cerneels <bart.cerneels@gmail.com>             *
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

class QTextStream;

namespace Meta
{

    class Playlist;

    typedef KSharedPtr<Playlist> PlaylistPtr;
    typedef QList<PlaylistPtr> PlaylistList;

    class AMAROK_EXPORT PlaylistObserver
    {
        public:
            /** This method is called when  a playlist has changed.
             */
            virtual void trackListChanged( Playlist* playlist ) = 0;
            virtual ~PlaylistObserver() {};
    };

    class AMAROK_EXPORT Playlist : public QSharedData
    {
        public:
            virtual ~Playlist() {}
            virtual QString name() const = 0;
            virtual QString prettyName() const = 0;

            /**override showing just the filename */
            void setName( QString name ) { m_name = name; }

            /** returns all tracks in this playlist */
            virtual TrackList tracks() = 0;

            virtual void subscribe( PlaylistObserver *observer )
                    { if( observer ) m_observers.insert( observer ); };
            virtual void unsubscribe( PlaylistObserver *observer )
                    { m_observers.remove( observer ); };


            /* the following has been copied from Meta.h
            * it is my hope that we can integrate Playlists
            * better into the rest of the Meta framework someday ~Bart Cerneels
            * TODO: Playlist : public MetaBase
            */
            virtual bool hasCapabilityInterface( Meta::Capability::Type type ) const = 0;

            virtual Capability* asCapabilityInterface( Capability::Type type ) = 0;

            virtual KUrl retrievableUrl() = 0;

            virtual bool load( QTextStream &stream ) = 0;
            virtual bool save( const QString &location, bool relative ) { return false; };

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

        protected:
            virtual void notifyObservers() const {
                foreach( PlaylistObserver *observer, m_observers )
                    observer->trackListChanged( const_cast<Meta::Playlist*>( this ) );
            };

        protected:
            QSet<Meta::PlaylistObserver*> m_observers;
            QString m_name;
    };

}

Q_DECLARE_METATYPE( Meta::PlaylistPtr )
Q_DECLARE_METATYPE( Meta::PlaylistList )

#endif
