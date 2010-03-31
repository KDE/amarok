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

#define DEBUG_PREFIX "APG::ConstraintGroup"

#include "ConstraintGroup.h"
#include "ConstraintFactory.h"
#include "constraints/Matching.h"

#include "core/meta/Meta.h"
#include "core/collections/QueryMaker.h"
#include "core/support/Debug.h"

#include <QList>
#include <QString>
#include <QtGlobal>

ConstraintGroup::ConstraintGroup( QDomElement& xmlelem, ConstraintNode* p ) : ConstraintNode( p )
{
    if ( xmlelem.tagName() == "group" ) {
        if ( xmlelem.attribute( "matchtype" ) == "any" ) {
            m_matchtype = MatchAny;
        } else {
            m_matchtype = MatchAll;
        }
    } else if ( xmlelem.tagName() == "constrainttree" ) {
        // root node of a constraint tree
        m_matchtype = MatchAll;
    } else {
        m_matchtype = MatchAll;
    }
}

ConstraintGroup::ConstraintGroup( ConstraintNode* p ) : ConstraintNode( p )
{
    m_matchtype = MatchAll;
}

ConstraintGroup*
ConstraintGroup::createFromXml( QDomElement& xmlelem, ConstraintNode* p )
{
    ConstraintGroup* cg = new ConstraintGroup( xmlelem, p );
    ConstraintFactory* cfact = ConstraintFactory::instance();

    // Load the children, which can be either groups or constraints
    for ( int i = 0; i < xmlelem.childNodes().count(); i++ ) {
        QDomElement childXmlElem = xmlelem.childNodes().item( i ).toElement();
        if ( !childXmlElem.isNull() ) {
            if ( childXmlElem.tagName() == "group" ) {
                cfact->createGroup( childXmlElem, cg );
            } else if ( childXmlElem.tagName() == "constraint" ) {
                cfact->createConstraint( childXmlElem, cg );
            } else {
                debug() << "unknown child: " << childXmlElem.nodeName();
            }
        }
    }
    return cg;
}

ConstraintGroup*
ConstraintGroup::createNew( ConstraintNode* p )
{
    return new ConstraintGroup( p );
}

QString
ConstraintGroup::getName() const
{
    if ( m_matchtype == MatchAny ) {
        return QString( "\"Match Any\" group" );
    } else if ( m_matchtype == MatchAll ) {
        return QString( "\"Match All\" group" );
    } else {
        return QString( "Unknown match group" );
    }
}

QWidget*
ConstraintGroup::editWidget() const
{
    ConstraintGroupEditWidget* e = new ConstraintGroupEditWidget( m_matchtype );
    connect( e, SIGNAL( clickedMatchAny() ), this, SLOT( setMatchAny() ) );
    connect( e, SIGNAL( clickedMatchAll() ), this, SLOT( setMatchAll() ) );
    return e;
}

void
ConstraintGroup::toXml( QDomDocument& doc, QDomElement& elem ) const
{
    QDomElement group;

    if ( elem.tagName() == "generatorpreset" ) {
        group = doc.createElement( "constrainttree" ); // unmodifiable root element of the constraint tree
    } else {
        group = doc.createElement( "group" );
        if ( m_matchtype == MatchAny ) {
            group.setAttribute( "matchtype", "any" );
        } else {
            group.setAttribute( "matchtype", "all" );
        }
    }

    foreach( ConstraintNode* child, m_children ) {
        child->toXml( doc, group );
    }

    elem.appendChild( group );
}

Collections::QueryMaker*
ConstraintGroup::initQueryMaker( Collections::QueryMaker* qm ) const
{
    if ( m_children.size() > 0 ) {
        if ( m_matchtype == MatchAny ) {
            qm->beginOr();
        } else if ( m_matchtype == MatchAll ) {
            qm->beginAnd();
        } else {
            return qm;
        }

        foreach( ConstraintNode* child, m_children ) {
            child->initQueryMaker( qm );
        }

        return qm->endAndOr();
    } else {
        return qm;
    }
}

double
ConstraintGroup::satisfaction( const Meta::TrackList& l )
{
    // shortcut if the playlist is empty
    if ( l.size() <= 0 ) {
        m_satisfaction = 1.0;
        return 1.0;
    }

    if ( m_children.isEmpty() ) {
        m_satisfaction = 1.0;
        return 1.0;
    }

    double s;
    if ( m_matchtype == MatchAny ) {
        s = 0.0;
    } else if ( m_matchtype == MatchAll ) {
        s = 1.0;
    } else {
        m_satisfaction = 1.0;
        return 1.0;
    }

    m_childSatisfactions.clear();
    m_constraintMatchTypes.clear();

    for ( int i = 0; i < m_children.size(); i++ ) {
        ConstraintNode* child = m_children[i];
        double chS = child->satisfaction( l );
        if ( m_matchtype == MatchAny ) {
            s = qMax( s, chS );
        } else if ( m_matchtype == MatchAll ) {
            s = qMin( s, chS );
        }
        m_childSatisfactions.append( chS );

        // prepare for proper handling of non-independent constraints
        // TODO: this should be moved out of the satisfaction function, and into some kind of "child satisfaction has changed" slot
        ConstraintTypes::MatchingConstraint* cge = dynamic_cast<ConstraintTypes::MatchingConstraint*>( child );
        if ( cge ) {
            m_constraintMatchTypes.insertMulti( cge->constraintMatchType(), i );
        }
    }

    // remove the independent constraints from the hash
    foreach( int key, m_constraintMatchTypes.uniqueKeys() ) {
        QList<int> vals = m_constraintMatchTypes.values( key );
        if ( vals.size() <= 1 ) {
            m_constraintMatchTypes.remove( key );
        }
    }

    m_satisfaction = combineNonIndependentConstraints( l, s );
    return m_satisfaction;
}

double
ConstraintGroup::deltaS_insert( const Meta::TrackList& tl, const Meta::TrackPtr t, const int idx ) const
{
    double newS;
    if ( m_matchtype == MatchAny ) {
        newS = 0.0;
    } else if ( m_matchtype == MatchAll ) {
        newS = 1.0;
    } else {
        return 0.0;
    }

    for ( int i = 0; i < m_children.size(); i++ ) {
        ConstraintNode* child = m_children.at( i );
        double chS = m_childSatisfactions.at( i ) + child->deltaS_insert( tl, t, idx );
        if ( m_matchtype == MatchAny ) {
            newS = qMax( newS, chS );
        } else if ( m_matchtype == MatchAll ) {
            newS = qMin( newS, chS );
        }
    }

    Meta::TrackList newTl( tl );
    newTl.insert( idx, t );
    double d = combineNonIndependentConstraints( newTl, newS ) - m_satisfaction;
    return d;
}

double
ConstraintGroup::deltaS_replace( const Meta::TrackList& tl, const Meta::TrackPtr t, const int place ) const
{
    double newS;
    if ( m_matchtype == MatchAny ) {
        newS = 0.0;
    } else if ( m_matchtype == MatchAll ) {
        newS = 1.0;
    } else {
        return 0.0;
    }

    for ( int i = 0; i < m_children.size(); i++ ) {
        ConstraintNode* child = m_children.at( i );
        double chS = m_childSatisfactions.at( i ) + child->deltaS_replace( tl, t, place );
        if ( m_matchtype == MatchAny ) {
            newS = qMax( newS, chS );
        } else if ( m_matchtype == MatchAll ) {
            newS = qMin( newS, chS );
        }
    }

    Meta::TrackList newTl( tl );
    newTl.replace( place, t );
    double d = combineNonIndependentConstraints( newTl, newS ) - m_satisfaction;
    return d;
}

double
ConstraintGroup::deltaS_delete( const Meta::TrackList& tl, const int place ) const
{
    double newS;
    if ( m_matchtype == MatchAny ) {
        newS = 0.0;
    } else if ( m_matchtype == MatchAll ) {
        newS = 1.0;
    } else {
        return 0.0;
    }

    for ( int i = 0; i < m_children.size(); i++ ) {
        ConstraintNode* child = m_children.at( i );
        double chS = m_childSatisfactions.at( i ) + child->deltaS_delete( tl, place );
        if ( m_matchtype == MatchAny ) {
            newS = qMax( newS, chS );
        } else if ( m_matchtype == MatchAll ) {
            newS = qMin( newS, chS );
        }
    }

    Meta::TrackList newTl( tl );
    newTl.removeAt( place );
    double d = combineNonIndependentConstraints( newTl, newS ) - m_satisfaction;
    return d;
}

double
ConstraintGroup::deltaS_swap( const Meta::TrackList& tl, const int place, const int other ) const
{
    double newS;
    if ( m_matchtype == MatchAny ) {
        newS = 0.0;
    } else if ( m_matchtype == MatchAll ) {
        newS = 1.0;
    } else {
        return 0.0;
    }

    for ( int i = 0; i < m_children.size(); i++ ) {
        ConstraintNode* child = m_children.at( i );
        double chS = m_childSatisfactions.at( i ) + child->deltaS_swap( tl, place, other );
        if ( m_matchtype == MatchAny ) {
            newS = qMax( newS, chS );
        } else if ( m_matchtype == MatchAll ) {
            newS = qMin( newS, chS );
        }
    }

    Meta::TrackList newTl( tl );
    newTl.swap( place, other );
    double d = combineNonIndependentConstraints( newTl, newS ) - m_satisfaction;
    return d;
}

void
ConstraintGroup::insertTrack( const Meta::TrackList& tl, Meta::TrackPtr t, const int place )
{
    double newS;
    if ( m_matchtype == MatchAny ) {
        newS = 0.0;
    } else if ( m_matchtype == MatchAll ) {
        newS = 1.0;
    } else {
        return;
    }

    for ( int i = 0; i < m_children.size(); i++ ) {
        ConstraintNode* child = m_children.at( i );
        m_childSatisfactions[i] += child->deltaS_insert( tl, t, place );
        child->insertTrack( tl, t, place );
        double chS = m_childSatisfactions[i];
        if ( m_matchtype == MatchAny ) {
            newS = qMax( newS, chS );
        } else if ( m_matchtype == MatchAll ) {
            newS = qMin( newS, chS );
        }
    }

    Meta::TrackList newTl( tl );
    newTl.insert( place, t );
    m_satisfaction = combineNonIndependentConstraints( newTl, newS );
}

void
ConstraintGroup::replaceTrack( const Meta::TrackList& tl, const Meta::TrackPtr t, const int place )
{
    double newS;
    if ( m_matchtype == MatchAny ) {
        newS = 0.0;
    } else if ( m_matchtype == MatchAll ) {
        newS = 1.0;
    } else {
        return;
    }

    for ( int i = 0; i < m_children.size(); i++ ) {
        ConstraintNode* child = m_children.at( i );
        m_childSatisfactions[i] += child->deltaS_replace( tl, t, place );
        child->replaceTrack( tl, t, place );
        double chS = m_childSatisfactions[i];
        if ( m_matchtype == MatchAny ) {
            newS = qMax( newS, chS );
        } else if ( m_matchtype == MatchAll ) {
            newS = qMin( newS, chS );
        }
    }

    Meta::TrackList newTl( tl );
    newTl.replace( place, t );
    m_satisfaction = combineNonIndependentConstraints( newTl, newS );
}

void
ConstraintGroup::deleteTrack( const Meta::TrackList& tl, const int place )
{
    double newS;
    if ( m_matchtype == MatchAny ) {
        newS = 0.0;
    } else if ( m_matchtype == MatchAll ) {
        newS = 1.0;
    } else {
        return;
    }

    for ( int i = 0; i < m_children.size(); i++ ) {
        ConstraintNode* child = m_children.at( i );
        m_childSatisfactions[i] += child->deltaS_delete( tl, place );
        child->deleteTrack( tl, place );
        double chS = m_childSatisfactions[i];
        if ( m_matchtype == MatchAny ) {
            newS = qMax( newS, chS );
        } else if ( m_matchtype == MatchAll ) {
            newS = qMin( newS, chS );
        }
    }

    Meta::TrackList newTl( tl );
    newTl.removeAt( place );
    m_satisfaction = combineNonIndependentConstraints( newTl, newS );
}

void
ConstraintGroup::swapTracks( const Meta::TrackList& tl, const int place, const int other )
{
    double newS;
    if ( m_matchtype == MatchAny ) {
        newS = 0.0;
    } else if ( m_matchtype == MatchAll ) {
        newS = 1.0;
    } else {
        return;
    }

    for ( int i = 0; i < m_children.size(); i++ ) {
        ConstraintNode* child = m_children.at( i );
        m_childSatisfactions[i] += child->deltaS_swap( tl, place, other );
        child->swapTracks( tl, place, other );
        double chS = m_childSatisfactions[i];
        if ( m_matchtype == MatchAny ) {
            newS = qMax( newS, chS );
        } else if ( m_matchtype == MatchAll ) {
            newS = qMin( newS, chS );
        }
    }

    Meta::TrackList newTl( tl );
    newTl.swap( place, other );
    m_satisfaction = combineNonIndependentConstraints( newTl, newS );
}

int
ConstraintGroup::suggestInitialPlaylistSize() const
{
    int s = 0;
    int c = 0;
    foreach( ConstraintNode* child, m_children ) {
        int r = child->suggestInitialPlaylistSize();
        if ( r >= 0 ) {
            s += r;
            c++;
        }
    }
    if ( c > 0 ) {
        return s / c;
    } else {
        return 0;
    }
}

ConstraintNode::Vote*
ConstraintGroup::vote( const Meta::TrackList& playlist, const Meta::TrackList& domain ) const
{
    ConstraintNode* worstNode = 0;
    double worstSatisfaction = 2.0;
    for ( int i = 0; i < m_children.size(); i++ ) {
        if ( m_childSatisfactions.at( i ) < worstSatisfaction ) {
            worstSatisfaction = m_childSatisfactions.at( i );
            worstNode = m_children.at( i );
        }
    }
    if ( worstNode != 0 )
        return worstNode->vote( playlist, domain );
    else
        return 0;
}

#ifndef KDE_NO_DEBUG_OUTPUT
void
ConstraintGroup::audit( const Meta::TrackList& tl ) const
{
    foreach( ConstraintNode* child, m_children ) {
        child->audit( tl );
    }
}
#endif

double
ConstraintGroup::combineNonIndependentConstraints( const Meta::TrackList& l, const double s ) const
{
    /* Handle non-independent constraints properly.
     * See constraints/Matching.h for a description of why this is necessary. */
    foreach( int key, m_constraintMatchTypes.uniqueKeys() ) {
        QList<int> vals = m_constraintMatchTypes.values( key );
        // set up the blank matching array
        QBitArray m;
        if ( m_matchtype == MatchAny ) {
            m = QBitArray( l.size(), false );
        } else {
            m = QBitArray( l.size(), true );
        }

        // combine the arrays from the appropriate constraints
        foreach( int v, vals ) {
            ConstraintTypes::MatchingConstraint* cge = dynamic_cast<ConstraintTypes::MatchingConstraint*>( m_children[v] );
            if ( m_matchtype == MatchAny ) {
                m |= cge->whatTracksMatch( l );
            } else if ( m_matchtype == MatchAll ) {
                m &= cge->whatTracksMatch( l );
            }
        }

        // turn the array into a satisfaction value
        double chS = 0.0;
        for ( int j = 0; j < l.size(); j++ ) {
            if ( m.testBit( j ) ) {
                chS += 1.0;
            }
        }
        chS /= ( double )l.size();

        // choose the appropriate satisfaction value
        if ( m_matchtype == MatchAny ) {
            return qMax( s, chS );
        } else if ( m_matchtype == MatchAll ) {
            return qMin( s, chS );
        } else {
            return s;
        }
    }

    return s;
}

void
ConstraintGroup::setMatchAny()
{
    m_matchtype = MatchAny;
    emit dataChanged();
}

void
ConstraintGroup::setMatchAll()
{
    m_matchtype = MatchAll;
    emit dataChanged();
}

/******************************
 * Edit Widget                *
 ******************************/

ConstraintGroupEditWidget::ConstraintGroupEditWidget( const ConstraintGroup::MatchType t )
{
    ui.setupUi( this );
    QMetaObject::connectSlotsByName( this );

    switch( t ) {
        case ConstraintGroup::MatchAny:
            ui.radioButton_MatchAny->setChecked( true );
            break;
        case ConstraintGroup::MatchAll:
            ui.radioButton_MatchAll->setChecked( true );
            break;
    }
}

void
ConstraintGroupEditWidget::on_radioButton_MatchAll_clicked( bool c )
{
    if ( c ) {
        emit clickedMatchAll();
        emit updated();
    }
}

void
ConstraintGroupEditWidget::on_radioButton_MatchAny_clicked( bool c )
{
    if ( c ) {
        emit clickedMatchAny();
        emit updated();
    }
}
