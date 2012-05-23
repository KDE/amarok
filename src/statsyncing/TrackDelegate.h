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

#ifndef TRACKDELEGATE_H
#define TRACKDELEGATE_H

#include <KSharedPtr>

#include <QDateTime>
#include <QSet>
#include <QSharedData>
#include <QStringList>

namespace StatSyncing
{
    /**
     * Representation of a delegate of a track (either track from Amarok or from
     * scrobbling services), abstract.
     *
     * This class is used to perform track matching and synchronization. It must be
     * implemented in a thread-safe way.
     */
    class TrackDelegate : public QSharedData
    {
        public:
            TrackDelegate();
            virtual ~TrackDelegate();

            /**
             * Get track title
             */
            virtual QString name() const = 0;

            /**
             * Get title of the album of this track
             */
            virtual QString album() const = 0;

            /**
             * Get track artist name
             */
            virtual QString artist() const = 0;

            /**
             * Get composer name; empty composer means it is unknown and as such compares
             * as equal to all other composers
             */
            virtual QString composer() const = 0;

            /**
             * Get track release year, 0 if it is unknown; if year <= 0, it compares as
             * equal to all other years
             */
            virtual int year() const = 0;

            /**
             * Get track number within its disc, if trackNumber <= 0, it compares as
             * equal to all other trackNumbers
             */
            virtual int trackNumber() const = 0;

            /**
             * Get disc number, 0 if it is unknown; if discNumber <= 0, it compares as
             * equal to all other discNumbers
             */
            virtual int discNumber() const = 0;

            /**
             * Return true if 2 tracks delegated by this and @param other are similar
             * enough to be considered equal. Takes special values of composer, year,
             * track and disc number into account.
             */
            bool operator==( const TrackDelegate &other ) const;

            /**
             * Return true if this track delegate is considered smaller than @param other.
             * Beware that this used only name, album and artist keys to do the comparison.
             * As a result following implication does _not_ hold:
             * !(A < B) && !(B < A)  =>   A == B
             * while the other one still holds:
             * A == B  =>  !(A < B) && !(B < A)
             */
            bool operator<( const TrackDelegate &other ) const;

            /**
             * Get user-assigned track labels or empty set if there are none
             */
            virtual QSet<QString> labels() const = 0;

            /**
             * Get user-assigned rating on scale from 0 to 10; 0 means the track is not
             * rated - return this value if you don't know the rating.
             */
            virtual int rating() const = 0;

            /**
             * Get date when the track was first played or invalid date if this is not
             * known or the track was not yet played
             */
            virtual QDateTime firstPlayed() const = 0;

            /**
             * Get date when the track was last played or invalid date if this is not
             * known or the track was not yet played
             */
            virtual QDateTime lastPlayed() const = 0;

            /**
             * Get count of the track plays; return 0 in doubt
             */
            virtual int playcount() const = 0;

        private:
            Q_DISABLE_COPY(TrackDelegate)
    };

    typedef KSharedPtr<TrackDelegate> TrackDelegatePtr;

} // namespace StatSyncing

#endif // TRACKDELEGATE_H
