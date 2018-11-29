/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2013 Matěj Laitl <matej@laitl.cz>                                      *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ***************************************************************************************/

#ifndef META_OBSERVER_H
#define META_OBSERVER_H

#include "core/amarokcore_export.h"
#include "core/meta/forward_declarations.h"

#include <QMutex>
#include <QSet>

class PersistentStatisticsStore;

namespace Meta {
    /**
     * Subclass this class to be able to listen to changes of track, artist, album, genre,
     * composer and year metadata. Must useful just for tracks and albums though.
     *
     * If you want to override just one metadataChanged() and want to get rid of "method
     * hidden compiler warnings", use following pattern in your class declaration:
     *
     * using Observer::metadataChanged;
     * void metadataChanged( AlbumPtr album );
     *
     * This class is thread-safe.
     */
    class AMAROK_CORE_EXPORT Observer
    {
        friend class Base; // so that it can call destroyedNotify()

        public:
            virtual ~Observer();

            /**
             * Subscribe to changes made by @param entity .
             *
             * Changed in 2.7: being subscribed to an entity no longer prevents its
             * destruction.
             */
            template <typename T>
            void subscribeTo( AmarokSharedPointer<T> entity ) { subscribeTo( entity.data() ); }
            template <typename T>
            void unsubscribeFrom( AmarokSharedPointer<T> entity ) { unsubscribeFrom( entity.data() ); }

            /**
             * This method is called when the metadata of a track has changed.
             * The called class may not cache the pointer.
             */
            virtual void metadataChanged( TrackPtr track );
            virtual void metadataChanged( ArtistPtr artist );
            virtual void metadataChanged( AlbumPtr album );
            virtual void metadataChanged( GenrePtr genre );
            virtual void metadataChanged( ComposerPtr composer );
            virtual void metadataChanged( YearPtr year );

            /**
             * One of the subscribed entities was destroyed. You don't get which one
             * because it is already invalid.
             */
            virtual void entityDestroyed();

        private:
            friend class ::PersistentStatisticsStore; // so that it can call AmarokSharedPointer-free subscribe:
            void subscribeTo( Base *ptr );
            void unsubscribeFrom( Base *ptr );

            /**
             * Called in Meta::Base destructor so that Observer doesn't have a stale pointer.
             */
            void destroyedNotify( Base *ptr );

            QSet<Base *> m_subscriptions;
            QMutex m_subscriptionsMutex; /// mutex guarding access to m_subscriptions
    };
}

#endif // META_OBSERVER_H
