/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
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

#ifndef META_BASE_H
#define META_BASE_H

#include "core/amarokcore_export.h"
#include "core/interfaces/MetaCapability.h"
#include "core/meta/forward_declarations.h"

#include <QMetaType>
#include <QReadWriteLock>
#include <QSet>

namespace Meta {
    class Observer;

    class AMAROKCORE_EXPORT Base : public virtual QSharedData, public MetaCapability
    // virtual inherit. so that implementations can be both Meta::Track and Meta::Statistics
    {
        public:
            Base();
            ~Base() override;

            /**
             * The textual label for this object.
             *
             * For a track this is the track title, for an album it is the album name.
             * If the name is unknown or unset then this returns an empty string.
             */
            virtual QString name() const = 0;

            /**
             * This is a nicer representation of the object.
             *
             * We will try to prevent this name from being empty. E.g. a track will fall
             * back to the filename if possible.
             */
            virtual QString prettyName() const { return name(); }

            /**
             * A name that can be used for sorting.
             *
             * This should usually mean that "The Beatles" is returned as "Beatles, The"
             */
            virtual QString sortableName() const { return name(); }

        protected:
            /**
             * Helper so that notifyObservers() implementation can be shared. Template
             * parameter Obs is just Observer, we add it so that Observer.h doesn't need
             * to be included in this header.
             */
            template <typename T, typename Obs>
            void notifyObserversHelper( const T *self ) const;

        private:
            // no copy allowed, since it's not safe with observer list
            Q_DISABLE_COPY( Base )

            friend class Observer; // so that Observer can call (un)subscribe()

            /**
             * Subscribe @param observer for change updates. Don't ever think of calling
             * this method yourself or overriding it, it's highly coupled with Observer.
             */
            void subscribe( Observer *observer );

            /**
             * Unsubscribe @param observer from change updates. Don't ever think of
             * calling this method yourself or overriding it, it's highly coupled with
             * Observer.
             */
            void unsubscribe( Observer *observer );

            QSet<Observer *> m_observers;
            mutable QReadWriteLock m_observersLock; // guards access to m_observers
    };

    template <typename T, typename Obs>
    void
    Base::notifyObserversHelper( const T *self ) const
    {
        // observers ale allowed to remove themselves during metadataChanged() call. That's
        // why the lock needs to be recursive AND the lock needs to be for writing, because
        // a lock for reading cannot be recursively relocked for writing.
        QWriteLocker locker( &m_observersLock );
        for( Obs *observer : QSet<Observer *>(m_observers) )
        {
            // observers can potentially remove or even destroy other observers during
            // metadataChanged() call. Guard against it. The guarding doesn't need to be
            // thread-safe,  because we already hold m_observersLock (which is recursive),
            // so other threads wait on potential unsubscribe().
            if( m_observers.contains( observer ) )
                observer->metadataChanged( AmarokSharedPointer<T>( const_cast<T *>( self ) ) );
        }
    }
}

Q_DECLARE_METATYPE( Meta::DataPtr )
Q_DECLARE_METATYPE( Meta::DataList )

AMAROKCORE_EXPORT QDebug operator<<( QDebug dbg, const Meta::Base &base );

#endif // META_BASE_H
