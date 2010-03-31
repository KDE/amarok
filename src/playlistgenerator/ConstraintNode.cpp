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

#define DEBUG_PREFIX "APG::ConstraintNode"

#include "ConstraintNode.h"

#include "core/support/Debug.h"

#include <QList>

ConstraintNode::ConstraintNode( ConstraintNode* p ) : QObject( p ) { }

ConstraintNode::~ConstraintNode() { }

int ConstraintNode::row() const
{
    ConstraintNode* p = parent();
    if ( !p ) {
        return 0;
    } else {
        return p->where_is( this );
    }
}

int ConstraintNode::getRowCount() const
{
    return m_children.size();
}

ConstraintNode*
ConstraintNode::parent() const
{
    return static_cast<ConstraintNode*>( QObject::parent() );
}

bool ConstraintNode::addChild( ConstraintNode* nc, int idx )
{
    if ( !m_children.contains( nc ) ) {
        if ( idx <= m_children.size() ) {
            m_children.insert( idx, nc );
        } else {
            m_children.append( nc );
        }
        return true;
    } else {
        debug() << "Tried to add a node that's already a child";
        return false;
    }
}

ConstraintNode*
ConstraintNode::getChild( int idx )
{
    if ( idx < 0 )
        return 0;

    if ( idx < m_children.size() )
        return m_children.at( idx );

    return 0;
}

ConstraintNode*
ConstraintNode::pruneChild( int idx )
{
    if ( idx < 0 )
        return 0;
    if ( idx < m_children.size() ) {
        return m_children.takeAt( idx );
    }
    return 0;
}

bool
ConstraintNode::removeChild( int idx )
{
    if ( idx < 0 )
        return false;
    if ( idx < m_children.size() ) {
        ConstraintNode* n = m_children.takeAt( idx );
        n->deleteLater();
        return true;
    }
    return false;
}

int
ConstraintNode::where_is( const ConstraintNode* n ) const
{
    // some nasty juju because indexOf isn't treating n as const
    ConstraintNode* x = const_cast<ConstraintNode*>( n );
    return m_children.indexOf( x );
}

int
ConstraintNode::suggestInitialPlaylistSize() const
{
    return -1;
}

ConstraintNode::Vote*
ConstraintNode::vote( const Meta::TrackList&, const Meta::TrackList& ) const
{
    return 0;
}
