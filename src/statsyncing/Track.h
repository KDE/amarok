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

#ifndef STATSYNCING_TRACK_H
#define STATSYNCING_TRACK_H

#include "amarok_export.h"

#include <KSharedPtr>

#include <QDateTime>
#include <QSet>
#include <QSharedData>
#include <QStringList>

namespace Meta {
    class Track;
    typedef KSharedPtr<Track> TrackPtr;
}

namespace StatSyncing
{
    /**
     * Abstract representation of a track (either from Amarok or from scrobbling services).
     *
     * This class is used to perform track matching and synchronization. It must be
     * implemented in a thread-safe way. For optimal track matching, all string fields
     * should be trimmed of whitespace.
     *
     * Note: This will be probably morphed into Meta::Track someday, keep the interface
     * compatible as much as possible.
     */
    class AMAROK_EXPORT Track : public QSharedData
    {
        public:
            Track();
            virtual ~Track();

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
             * Get composer name; default implementation returns empty string that also
             * serves as a value for 'unknown'
             */
            virtual QString composer() const;

            /**
             * Get track release year, default implementation returns value of 0 that also
             * serves as a value for 'unknown'
             */
            virtual int year() const;

            /**
             * Get track number within its disc, default implementation returns value of 0
             * that also serves as a value for 'unknown'
             */
            virtual int trackNumber() const;

            /**
             * Get disc number, default implementation returns value of 0 that also serves
             * as a value for 'unknown'
             */
            virtual int discNumber() const;

            /**
             * Return true if 2 tracks delegated by this and @param other are equal based
             * on field mask @param fieldMask (binary OR of MetaValue.h values)
             */
            bool equals( const Track &other, qint64 fieldMask ) const;

            /**
             * Return true if this track delegate is considered smaller than @param other
             * based on field mask @param fieldMask (binary OR of MetaValue.h values)
             */
            bool lessThan( const Track &other, qint64 fieldMask ) const;

            /**
             * Get user-assigned rating on scale from 0 to 10; 0 means the track is not
             * rated - return this value if you don't know the rating
             */
            virtual int rating() const = 0;
            /**
             * Set user-assigned rating on scale from 0 to 10. Default implementation
             * does nothing.
             * @param rating the user-assigned rating
             */
            virtual void setRating( int rating );

            /**
             * Get date when the track was first played or invalid date if this is not
             * known or the track was not yet played
             */
            virtual QDateTime firstPlayed() const = 0;
            /**
             * Set date when the track was first played. Default implementation does
             * nothing.
             * @param firstPlayed the date the track was first played
             */
            virtual void setFirstPlayed( const QDateTime &firstPlayed );

            /**
             * Get date when the track was last played or invalid date if this is not
             * known or the track was not yet played
             */
            virtual QDateTime lastPlayed() const = 0;
            /**
             * Set date when the track was last played. Default implementation does
             * nothing.
             * @param lastPlayed() the date the track was last played
             */
            virtual void setLastPlayed( const QDateTime &lastPlayed );

            /**
             * Get count of the track plays; return 0 in doubt
             */
            virtual int playCount() const = 0;
            /**
             * Return play count on device since it has been last connected to a computer.
             * This number is _already_ _included_ in playcount()!
             */
            virtual int recentPlayCount() const;
            /**
             * Set count of the track plays. Setting playcount must reset recent
             * playcount to 0. Default implementation does nothing.
             * @param playCount() the count of the track plays
             */
            virtual void setPlayCount( int playCount );

            /**
             * Get user-assigned track labels or empty set if there are none
             */
            virtual QSet<QString> labels() const = 0;
            /**
             * Set user-assigned track labels. Default implementation does nothing.
             * @param labels the track labels
             */
            virtual void setLabels( const QSet<QString> &labels );

            /**
             * If this StatSyncing::Track represents a Meta::Track, this method returns a
             * pointer to it, otherwise it should return return a null pointer.
             *
             * This is used to enable drag in views and to enable scrobbling.
             *
             * Default implementation returns null pointer.
             */
            virtual Meta::TrackPtr metaTrack() const;

            /**
             * Write back statistics to the underlying storage. You must call this function
             * after calling any of the set* methods. The track may decide whether the
             * actual writeback happens in set* or in commit(). Default implementation does
             * nothing.
             *
             * Guaranteed to be (and must be) called from non-main thread. Can block for
             * a longer time.
             */
            virtual void commit();

        private:
            Q_DISABLE_COPY(Track)
    };

    typedef KSharedPtr<Track> TrackPtr;
    typedef QList<TrackPtr> TrackList;

    /**
     * Comparison function that compares track delegate pointer by pointed value.
     * Useful if you want to semantically sort TrackDelegateList using qSort()
     *
     * @template param ControllingClass: class name that implements static
     *      ::comparisonFields() method that returns binary OR of Meta::val* fields
     *      (as qint64) that should be used when comparing tracks.
     */
    template <class ControllingClass>
    bool trackDelegatePtrLessThan( const TrackPtr &first, const TrackPtr &second )
    {
        return first->lessThan( *second, ControllingClass::comparisonFields() );
    }

} // namespace StatSyncing

#endif // STATSYNCING_TRACK_H
