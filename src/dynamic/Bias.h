/***************************************************************************
 * copyright            : (C) 2008 Daniel Jones <danielcjones@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

#ifndef AMAROK_BIAS_H
#define AMAROK_BIAS_H

#include "Meta.h"

#include <QObject>
#include <QSet>

class Collection;
class QueryMaker;

namespace Dynamic
{

    /**
     * A bias is essentially just a function that evaluates the suitability of a
     * playlist in some arbitrary way.
     */
    class Bias
    {
        public:
            virtual ~Bias() {}

            /**
             * Returns a value in the range [0,1]. Playlist generation is being
             * treated as a minimization problem, so 0 means the bias is completely
             * satisfied, 1 that it is not satisfied at all.
             */
            virtual double energy( Meta::TrackList playlist ) = 0;


            /**
             * When a track is swaped in the playlist, avoid completely reevaluating
             * the energy function if possible.
             */
            virtual double reevaluate( double oldEnergy, Meta::TrackList oldPlaylist,
                    Meta::TrackPtr newTrack, int newTrackPos );
    };



    /**
     * A bias that depends on the state of the collection.
     */
    class CollectionDependantBias : public QObject, public Bias
    {
        Q_OBJECT

        public:
            CollectionDependantBias( Collection* );

            /**
             * This gets called when the collection changes.
             */
            virtual void update() = 0;
            bool needsUpdating();

        public slots:
            void collectionUpdated();

        protected:
            bool m_needsUpdating;
    };


    /**
     * This a bias in which the order and size of the playlist are not
     * considered. Instead we want a given proportion (weight) of the tracks to
     * have a certain property (or belong to a certain set).
     */
    class GlobalBias : public CollectionDependantBias
    {
        public:
            GlobalBias( Collection* coll, double weight, QueryMaker* propertyQuery );

            double energy( Meta::TrackList playlist );
            double reevaluate( double oldEnergy, Meta::TrackList oldPlaylist,
                    Meta::TrackPtr newTrack, int newTrackPos );
            bool trackSatisfies( Meta::TrackPtr );
            void update();

            double weight() const;
            void setWeight( double );

        private:
            double m_weight; /// range: [0,1]
            QSet<Meta::TrackPtr> m_property;
            QueryMaker* m_propertyQuery;
    };
}

#endif
