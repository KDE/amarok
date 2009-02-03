/* This file is part of the KDE project
   Copyright (C) 2007 Bart Cerneels <bart.cerneels@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

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
#include <QTextStream>

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

    class AMAROK_EXPORT Playlist : public virtual QSharedData
    {
        public:
            virtual ~Playlist() {}
            virtual QString name() const = 0;
            virtual QString prettyName() const = 0;
            virtual QString description() const { return QString(); }

            /**override showing just the filename */
            virtual void setName( const QString &name ) { m_name = name; }

            /** returns all tracks in this playlist */
            virtual TrackList tracks() = 0;

            virtual void subscribe( PlaylistObserver *observer )
                    { if( observer ) m_observers.insert( observer ); };
            virtual void unsubscribe( PlaylistObserver *observer )
                    { m_observers.remove( observer ); };

            virtual QStringList groups() { return QStringList(); }
            virtual void setGroups( const QStringList &groups ) { Q_UNUSED(groups) };

            /* the following has been copied from Meta.h
            * it is my hope that we can integrate Playlists
            * better into the rest of the Meta framework someday ~Bart Cerneels
            * TODO: Playlist : public MetaBase
            */
            virtual bool hasCapabilityInterface( Meta::Capability::Type type ) const = 0;

            virtual Capability* asCapabilityInterface( Capability::Type type ) = 0;

            virtual KUrl retrievableUrl() { return KUrl(); }

            virtual bool load( QTextStream &stream ) { Q_UNUSED( stream ); return false; }
            virtual bool save( const QString &location, bool relative ) { Q_UNUSED( location); Q_UNUSED( relative); return false; };

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
