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

#ifndef APG_GENERIC_MATCHING_CONSTRAINT
#define APG_GENERIC_MATCHING_CONSTRAINT

#include "playlistgenerator/Constraint.h"

#include "core/meta/Meta.h"

#include <QBitArray>

/* The basic ConstraintGroup logic works properly only if all the
 * constraints in the group are independent.  Constraints that work by
 * matching track attributes to fixed values need additional logic, because
 * it is possible (and even likely) that the user will create a group in
 * which two or more constraints match against the same attribute, and are
 * thus non-independent.
 *
 * For example, let's say that the user creates a "match any" group, and
 * adds two constraints: one for "genre: Rock" and the other for "genre:
 * Rap".  These two constraints are non-independent.  Without this
 * workaround, the solver algorithm will fill up the playlist with either
 * Rock tracks or Rap tracks, but not a mixture of both, because it is
 * trying to maximize the individual satisfactions and not the joint
 * satisfaction.  For "match all" groups, the solver adds tracks that
 * improve only the least-satisfied constraint, instead of adding tracks
 * that improve all constraints.
 * 
 * Non-independent constraints should have their results combined (using the
 * appropriate boolean operator) and the satisfaction value should be the
 * value of the combined results.  From a programming standpoint, it would
 * make sense just to prohibit the user from adding non-independent
 * constraints to a group.  But that approach would add a restriction to
 * constraint trees which is difficult for the user to understand, and an
 * even bigger departure from other audio player programs than the
 * constraint tree already is.  It might not even be any easier to program
 * than this logic workaround.  Thus the decision to make the code the way
 * it is.
 *
 * Thus far, the only Constraint that actually needs this workaround is
 * TagMatch.  More (like SimilarityMatch) might come later.  -- sth */

namespace ConstraintTypes {
	class MatchingConstraint : public Constraint {
        public:
            virtual const QBitArray whatTracksMatch(const Meta::TrackList&) = 0;
            virtual int constraintMatchType() const = 0;

        protected:
            MatchingConstraint(ConstraintNode*);
    };
}
#endif // MATCHINGCONSTRAINT_H
