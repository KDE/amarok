/****************************************************************************************
 * Copyright (c) 2008-2012 Soren Harward <stharward@gmail.com>                          *
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

#include "core/meta/forward_declarations.h"

class Constraint;
class ConstraintFactoryEntry;

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
            static Constraint* createNew( ConstraintNode* );
            static ConstraintFactoryEntry* registerMe();

            virtual QWidget* editWidget() const;
            virtual void toXml( QDomDocument&, QDomElement& ) const;

            virtual QString getName() const { return QString(); }
            
            virtual double satisfaction( const Meta::TrackList& ) const;

        private:
            explicit TrackSpreader(ConstraintNode*);

            double distance( const int, const int ) const;
    };
} // namespace ConstraintTypes

#endif // PLAYLIST_GENERATOR_CHECKPOINT
