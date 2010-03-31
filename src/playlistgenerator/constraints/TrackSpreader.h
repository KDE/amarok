/****************************************************************************************
 * Copyright (c) 2008-2010 Soren Harward <stharward@gmail.com>                          *
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

#ifndef APG_TRACKSPREADER_CONSTRAINT
#define APG_TRACKSPREADER_CONSTRAINT

#include "playlistgenerator/Constraint.h"

#include "core/meta/Meta.h"

#include <QHash>
#include <QList>
#include <QString>
#include <QVariant>

class Constraint;
class ConstraintFactoryEntry;

namespace Collections {
    class QueryMaker;
}

namespace ConstraintTypes {

    /* This constraint tries to prevent duplicate tracks from ending up too
     * close to each other in the playlist.  It is a stripped-down constraint
     * that is never registered with the ConstraintFactory, has no
     * user-configurable options, and cannot be exported to an XML file.  It is
     * added to a constraint tree at the root when the Constraint Solver
     * starts, and it is then removed from the tree as soon as the Solver
     * finishes. -- sth */

    class TrackSpreader : public Constraint {
        Q_OBJECT

        public:
            static Constraint* createNew(ConstraintNode*);
            static ConstraintFactoryEntry* registerMe();

            virtual QWidget* editWidget() const;
            virtual void toXml(QDomDocument&, QDomElement&) const;

            virtual QString getName() const { return QString(); }
            
            virtual Collections::QueryMaker* initQueryMaker(Collections::QueryMaker*) const;
            virtual double satisfaction(const Meta::TrackList&);
            virtual double deltaS_insert(const Meta::TrackList&, const Meta::TrackPtr, const int) const;
            virtual double deltaS_replace(const Meta::TrackList&, const Meta::TrackPtr, const int) const;
            virtual double deltaS_delete(const Meta::TrackList&, const int) const;
            virtual double deltaS_swap(const Meta::TrackList&, const int, const int) const;
            virtual void insertTrack(const Meta::TrackList&, const Meta::TrackPtr, const int);
            virtual void replaceTrack(const Meta::TrackList&, const Meta::TrackPtr, const int);
            virtual void deleteTrack(const Meta::TrackList&, const int);
            virtual void swapTracks(const Meta::TrackList&, const int, const int);

#ifndef KDE_NO_DEBUG_OUTPUT
            virtual void audit(const Meta::TrackList&) const;
#endif

        private:
            /* Helper class that follows the playlist and allows fast lookups
             * of the indices of duplicate tracks.  I encapsulated this in its
             * own class because I've rewritten the caching/lookup part of the
             * algorithm at least three times, and got sick of rewriting 80% of
             * the TrackSpreader each time I did.  -- stharward */

            class TrackLocations {
                public:
                    TrackLocations() {}
                    TrackLocations( const TrackLocations& );

                    const QList<Meta::TrackPtr> keys() const { return m_locations.uniqueKeys(); }
                    const QList<int> values( const Meta::TrackPtr t ) const { return m_locations.values( t ); }

                    void clear() { m_counts.clear(); m_locations.clear(); }
                    void fill( const Meta::TrackList& );

                    void insertTrack( const Meta::TrackList&, const Meta::TrackPtr, const int );
                    void replaceTrack( const Meta::TrackList&, const Meta::TrackPtr, const int );
                    void deleteTrack( const Meta::TrackList&, const int );
                    void swapTracks( const Meta::TrackList&, const int, const int );

                private:
                    void insertTrackInternal( const Meta::TrackList&, const Meta::TrackPtr, const int );
                    void dumpTracked( const Meta::TrackList& ) const;

                    QHash< Meta::TrackPtr, int > m_counts;
                    QMultiHash< Meta::TrackPtr, int > m_locations;
            };

            TrackSpreader(ConstraintNode*);

            // internal mathematical functions
            double satisfactionFromTrackLocations( const TrackLocations& ) const;
            double listToDist( const QList<int>& ) const;
            double distance( const int i, const int j ) const;
            double sumToSat( double ) const;

            // internal state data
            TrackLocations m_trackLocations;
            double m_satisfaction;
    };
} // namespace ConstraintTypes

#endif // PLAYLIST_GENERATOR_CHECKPOINT
