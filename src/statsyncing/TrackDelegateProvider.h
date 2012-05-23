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
     * A class that can provide tracks for statistics synchronization. It can be backed
     * by local Amarok collections or by online services such as Last.fm.
     *
     * Instances of inheriters are guaranteed to be created in the main thread.
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
             * Return binary OR of Meta::val* types that this provider knows about its
             * tracks. Must include at least: Meta::valTitle, Meta::valArtist and
             * Meta::valAlbum. Optional fields: Meta::valComposer, Meta::valYear
             * Meta::valTrackNr and Meta::valDiscNr
             */
            virtual qint64 reliableTrackMetaData() const = 0;

            /**
             * Return a set of lowercased (all characters lowercased, not just ASCII)
             * track artist names that appear in this provider. This method is guaranteed
             * to be called in non-main thread and is allowed block for a longer time; it
             * must be implemented in a reentrant manner.
             */
            virtual QSet<QString> artists() = 0;

            /**
             * Return a list of track delegates from (track) artist @param artistName that
             * appear in this provider; the matching should be performed case-isensitively
             * but should match the whole string, not just substring. This method is
             * guaranteed to be called in non-main thread and is allowed block for
             * a longer time; it must be implemented in a reentrant manner.
             */
            virtual TrackDelegateList artistTracks( const QString &artistName ) = 0;

        private:
            Q_DISABLE_COPY(TrackDelegateProvider)
    };

} // namespace StatSyncing

#endif // TRACKDELEGATEPROVIDER_H
