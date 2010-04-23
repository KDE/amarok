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

#ifndef APG_CONSTRAINT
#define APG_CONSTRAINT

#include "ConstraintNode.h"

#include "core/meta/Meta.h"

#include <QDomElement>
#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <QtGlobal>
#include <math.h>

/* ABC for all Constraints */
class Constraint : public ConstraintNode {
    Q_OBJECT
    public:
        enum NumComparison { CompareNumLessThan, CompareNumEquals, CompareNumGreaterThan };
        enum StrComparison { CompareStrEquals, CompareStrStartsWith, CompareStrEndsWith, CompareStrContains, CompareStrRegExp };
        enum DateComparison { CompareDateBefore, CompareDateOn, CompareDateAfter, CompareDateWithin };

        static const double magicStrictnessWeight = 3.0;

        virtual int getNodeType() const { return ConstraintNode::ConstraintType; }

    protected:
        Constraint( ConstraintNode* );

        // A couple of helper functions for subclasses
        double compare(const QString&, const int, const QString&, const double strictness=1.0) const;
        template <typename T> double compare(const T, const int, const T, const double strictness=1.0) const;
};

// Templated function from Constraint
template <typename T> double Constraint::compare( const T a, const int comparison, const T b, const double strictness ) const {

    /* There's no mathematical rigor to this factor; I came up with some test
     * cases and what answers I expected, and tried a bunch of different
     * factors.  This one behaved closest to what I wanted.  -- sth */

    double factor = ( exp( magicStrictnessWeight * strictness) / ( sqrt( (double) b ) + 1.0 ) );
    if ( comparison == CompareNumEquals ) {
        // fuzzy equals -- within 1%
        if ( qAbs( a - b ) < ( ( a + b ) / 200.0 ) ) {
            return 1.0;
        } else if ( a > b ) {
            return exp( factor * ( b - a ) );
        } else {
            return exp( factor * ( a - b ) );
        }
    } else if ( comparison == CompareNumGreaterThan ) {
        return (a > b) ? 1.0 : exp( factor * ( a - ( b * 1.05 ) ) );
    } else if ( comparison == CompareNumLessThan ) {
        return (a < b) ? 1.0 : exp( factor * ( b - ( a * 1.05 ) ) );
    } else {
        return 0.0;
    }
    return 0.0;
}

#endif
