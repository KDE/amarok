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

#include "amarok_export.h"

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
            bool equals( const TrackDelegate &other, qint64 fieldMask ) const;

            /**
             * Return true if this track delegate is considered smaller than @param other
             * based on field mask @param fieldMask (binary OR of MetaValue.h values)
             */
            bool lessThan( const TrackDelegate &other, qint64 fieldMask ) const;

            /**
             * Get user-assigned track labels or empty set if there are none
             */
            virtual QSet<QString> labels() const = 0;

            /**
             * Get user-assigned rating on scale from 0 to 10; 0 means the track is not
             * rated - return this value if you don't know the rating
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
    typedef QList<TrackDelegatePtr> TrackDelegateList;

    /**
     * Comparison function that compares track delegate pointer by pointed value.
     * Useful if you want to semantically sort TrackDelegateList using qSort()
     *
     * @template param ControllingClass: class name that implements static
     *      ::comparisonFields() method that returns binary OR of Meta::val* fields
     *      (as qint64) that should be used when comparing tracks.
     */
    template <class ControllingClass>
    bool trackDelegatePtrLessThan( const TrackDelegatePtr &first, const TrackDelegatePtr &second )
    {
        return first->lessThan( *second, ControllingClass::comparisonFields() );
    }

} // namespace StatSyncing

#endif // TRACKDELEGATE_H
