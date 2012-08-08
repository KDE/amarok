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

#ifndef STATSYNCING_PROVIDER_H
#define STATSYNCING_PROVIDER_H

#include "statsyncing/Track.h"

#include <KIcon>

#include <QMap>
#include <QSet>
#include <QString>

namespace StatSyncing
{
    /**
     * A class that can provide tracks for statistics synchronization. It can be backed
     * by local Amarok collections or by online services such as Last.fm.
     *
     * Instances of subclasses are guaranteed to be created in the main thread.
     * Providers are memory-managed as explicitly shared data, always use ProviderPtr
     * to stora a reference to Provider.
     */
    class Provider : public QObject, public QSharedData
    {
        Q_OBJECT

        public:
            virtual ~Provider();

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
             * Return binary OR of Meta::val* fields that this provider knows about its
             * tracks. Must include at least: Meta::valTitle, Meta::valArtist and
             * Meta::valAlbum. Optional fields: Meta::valComposer, Meta::valYear
             * Meta::valTrackNr and Meta::valDiscNr
             */
            virtual qint64 reliableTrackMetaData() const = 0;

            /**
             * Return binary OR of Meta::val* fields that this provider can write back
             * to tracks. Choose a combination of: Meta::valRating, valFirstPlayed,
             * valLastPlayed, valPlaycount, valLabel.
             */
            virtual qint64 writableTrackStatsData() const = 0;

            enum Preference {
                Never, /// never synchronize automatically
                NoByDefault, /// don't synchronize automatically by default
                Ask, /// ask on first appearance whether to synchronize by default
                YesByDefault, /// enable auto syncing on first appearance without asking
                              /// intended only for Local Collection
            };

            /**
             * Return if this provider should participate in synchronization by
             * default even when the user does not actively add it. User can always
             * disable providers even if they are checked by default.
             */
            virtual Preference defaultPreference() = 0;

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
            virtual TrackList artistTracks( const QString &artistName ) = 0;

        signals:
            /**
             * Emitted when some data such as prettyName() were updated.
             */
            void updated();
    };

    typedef QExplicitlySharedDataPointer<Provider> ProviderPtr;
    typedef QList<ProviderPtr> ProviderPtrList;
    typedef QSet<ProviderPtr> ProviderPtrSet;

    /**
     * Container for a set of track frovider lists, one for each provider
     */
    typedef QMap<ProviderPtr, TrackList> PerProviderTrackList;
} // namespace StatSyncing

/**
 * As of version 4.8.2, Qt doesn't provide operator< for QExplicitlySharedDataPointer<T>.
 * QMap<Key,T> needs operator< for key, however QExplicitlySharedDataPointer<T> silently
 * coerces to bool, which gives really hard to find bugs, because it makes QMap with it
 * useless. Define the operator here, use our type to prevent clashes when Qt adds the
 * operator.
 */
bool operator<( const StatSyncing::ProviderPtr& a, const StatSyncing::ProviderPtr &b );

#endif // STATSYNCING_PROVIDER_H
