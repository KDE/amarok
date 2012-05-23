/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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
 ****************************************************************************************/

#ifndef TRACKDELEGATEPROVIDER_H
#define TRACKDELEGATEPROVIDER_H

#include "statsyncing/TrackDelegate.h"

#include <KIcon>

#include <QSet>
#include <QString>

namespace StatSyncing
{
    /**
     * Something that can provide tracks for statistics syncronization. It can be backed
     * by local Amarok collections or by online services such as Last.fm.
     *
     * All methods from this class must be (at least) reentrant. This class may be
     * created and used in a non-gui thread without its event loop.
     */
    class TrackDelegateProvider
    {
        public:
            TrackDelegateProvider();
            virtual ~TrackDelegateProvider();

            /**
            * Unique identifier for this collection; may be used as a key to store
            * configuration; must be thread-safe
            */
            virtual QString id() const = 0;

            /**
            * User-visible name of the provider; must be thread-safe
            */
            virtual QString prettyName() const = 0;

            /**
            * Icon of this provider; must be thread-safe
            */
            virtual KIcon icon() const = 0;

            /**
            * Return a set of (track) artist names that appear in this provider. This method
            * is allowed block for a longer time.
            */
            virtual QSet<QString> artistNames() = 0;

            /**
            * Return a list of track delegates from (track) artist @param artistName that
            * appear in this provider. This method is allowed to block for a longer time.
            */
            virtual QList<TrackDelegatePtr> artistTracks( const QString artistName ) = 0;

        private:
            Q_DISABLE_COPY(TrackDelegateProvider)
    };

} // namespace StatSyncing

#endif // TRACKDELEGATEPROVIDER_H
