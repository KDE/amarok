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

#ifndef APG_CONSTRAINTNODE
#define APG_CONSTRAINTNODE

#include "core/meta/Meta.h"

#include <QDomElement>
#include <QList>
#include <QObject>
#include <QString>

namespace Collections {
    class QueryMaker;
}

class QWidget;

/* Abstract base class for all constraints, including ConstraintGroup
 *
 * Inheritance Tree:
 *
 *                       ConstraintNode
 *                              |
 *             +----------------+-----------+
 *             |                            |
 *      ConstraintGroup                 Constraint
 *                                          |
 *          +-----------------+-------------+-----+--------  ... etc
 *          |                 |                   |
 *      Matching     PreventDuplicates     PlaylistLength
 *          |
 *      TagMatch
 *
 *
 * If you want to write a new Constraint, don't subclass this directly.
 * Subclass Constraint or one of its children instead. -- sth
 */

class ConstraintNode : public QObject {
    Q_OBJECT
    public:
        enum NodeType { ConstraintGroupType, ConstraintType };
        enum Operations { OperationInsert, OperationReplace, OperationDelete };

        typedef struct {
            Operations operation;
            int place;
            Meta::TrackPtr track;
        } Vote;

        virtual ~ConstraintNode();

        // Functions used in ConstraintModel
        int row() const;
        int getRowCount() const;

        ConstraintNode* parent() const;
        bool addChild( ConstraintNode*, int );
        ConstraintNode* getChild( int );
        ConstraintNode* pruneChild( int );
        bool removeChild( int );

        virtual QString getName() const = 0;
        virtual int getNodeType() const = 0;
        virtual QWidget* editWidget() const = 0;
        virtual void toXml( QDomDocument&, QDomElement& ) const = 0;

        // Set up the initial query for the ConstraintSolver
        virtual Collections::QueryMaker* initQueryMaker( Collections::QueryMaker* ) const = 0;

        // Mathematical functions called by the ConstraintSolver
        virtual double satisfaction( const Meta::TrackList& ) = 0;
        virtual double deltaS_insert( const Meta::TrackList&, const Meta::TrackPtr, const int ) const = 0;
        virtual double deltaS_replace( const Meta::TrackList&, const Meta::TrackPtr, const int ) const = 0;
        virtual double deltaS_delete( const Meta::TrackList&, const int ) const = 0;
        virtual double deltaS_swap( const Meta::TrackList&, const int, const int ) const = 0;
        virtual void insertTrack( const Meta::TrackList&, const Meta::TrackPtr, const int ) = 0;
        virtual void replaceTrack( const Meta::TrackList&, const Meta::TrackPtr, const int ) = 0;
        virtual void deleteTrack( const Meta::TrackList&, const int ) = 0;
        virtual void swapTracks( const Meta::TrackList&, const int, const int ) = 0;

        // heuristic functions for the ConstraintSolver
        virtual int suggestInitialPlaylistSize() const;
        virtual Vote* vote( const Meta::TrackList&, const Meta::TrackList& ) const;

#ifndef KDE_NO_DEBUG_OUTPUT
        virtual void audit(const Meta::TrackList&) const {}
#endif

    signals:
        void dataChanged();

    protected:
        ConstraintNode(ConstraintNode* parent);
        int where_is(const ConstraintNode*) const;

        QList<ConstraintNode*> m_children;
};

#endif
