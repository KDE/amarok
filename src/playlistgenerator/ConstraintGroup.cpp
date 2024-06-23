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
    DEBUG_BLOCK
    if ( xmlelem.tagName() == QLatin1String("group") ) {
        if ( xmlelem.attribute( QStringLiteral("matchtype") ) == QLatin1String("any") ) {
            m_matchtype = MatchAny;
        } else {
            m_matchtype = MatchAll;
        }
    } else if ( xmlelem.tagName() == QLatin1String("constrainttree") ) {
        // root node of a constraint tree
        m_matchtype = MatchAll;
    } else {
        m_matchtype = MatchAll;
    }
    debug() << getName();
}

ConstraintGroup::ConstraintGroup( ConstraintNode* p ) : ConstraintNode( p ), m_matchtype( MatchAll )
{
    DEBUG_BLOCK
    debug() << "new default ConstraintGroup";
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
            if ( childXmlElem.tagName() == QLatin1String("group") ) {
                cfact->createGroup( childXmlElem, cg );
            } else if ( childXmlElem.tagName() == QLatin1String("constraint") ) {
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
        return i18nc("name of a type of constraint group", "\"Match Any\" group");
    } else if ( m_matchtype == MatchAll ) {
        return i18nc("name of a type of constraint group", "\"Match All\" group");
    } else {
        return i18nc("name of a type of constraint group", "Unknown match group");
    }
}

QWidget*
ConstraintGroup::editWidget() const
{
    ConstraintGroupEditWidget* e = new ConstraintGroupEditWidget( m_matchtype );
    connect( e, &ConstraintGroupEditWidget::clickedMatchAny, this, &ConstraintGroup::setMatchAny );
    connect( e, &ConstraintGroupEditWidget::clickedMatchAll, this, &ConstraintGroup::setMatchAll );
    return e;
}

void
ConstraintGroup::toXml( QDomDocument& doc, QDomElement& elem ) const
{
    QDomElement group;

    if ( elem.tagName() == QLatin1String("generatorpreset") ) {
        group = doc.createElement( QStringLiteral("constrainttree") ); // unmodifiable root element of the constraint tree
    } else {
        group = doc.createElement( QStringLiteral("group") );
        if ( m_matchtype == MatchAny ) {
            group.setAttribute( QStringLiteral("matchtype"), QStringLiteral("any") );
        } else {
            group.setAttribute( QStringLiteral("matchtype"), QStringLiteral("all") );
        }
    }

    for( ConstraintNode* child : m_children ) {
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

        for( ConstraintNode* child : m_children ) {
            child->initQueryMaker( qm );
        }

        return qm->endAndOr();
    } else {
        return qm;
    }
}

double
ConstraintGroup::satisfaction( const Meta::TrackList& l ) const
{
    // shortcut if the playlist is empty
    if ( l.size() <= 0 ) {
        return 1.0;
    }

    if ( m_children.isEmpty() ) {
        return 1.0;
    }

    double s;
    if ( m_matchtype == MatchAny ) {
        s = 0.0;
    } else if ( m_matchtype == MatchAll ) {
        s = 1.0;
    } else {
        return 1.0;
    }

    QMultiHash<int,int> constraintMatchTypes;

    // TODO: there's got to be a more efficient way of handling interdependent constraints
    for ( int i = 0; i < m_children.size(); i++ ) {
        ConstraintNode* child = m_children[i];
        double chS = child->satisfaction( l );
        if ( m_matchtype == MatchAny ) {
            s = qMax( s, chS );
        } else if ( m_matchtype == MatchAll ) {
            s = qMin( s, chS );
        }

        // prepare for proper handling of non-independent constraints
        ConstraintTypes::MatchingConstraint* cge = dynamic_cast<ConstraintTypes::MatchingConstraint*>( child );
        if ( cge ) {
            constraintMatchTypes.insert( cge->constraintMatchType(), i );
        }
    }

    // remove the independent constraints from the hash
    foreach( int key, constraintMatchTypes.uniqueKeys() ) {
        QList<int> vals = constraintMatchTypes.values( key );
        if ( vals.size() <= 1 ) {
            constraintMatchTypes.remove( key );
        }
    }

    return combineInterdependentConstraints( l, s, constraintMatchTypes );
}

quint32
ConstraintGroup::suggestPlaylistSize() const
{
    quint32 s = 0;
    quint32 c = 0;
    for( ConstraintNode* child : m_children ) {
        quint32 x = child->suggestPlaylistSize();
        if ( x > 0 ) {
            s += x;
            c++;
        }
    }
    if ( c > 0 ) {
        return s / c;
    } else {
        return 0;
    }
}

double
ConstraintGroup::combineInterdependentConstraints( const Meta::TrackList& l, const double s, const QMultiHash<int,int>& cmt ) const
{
    /* Handle interdependent constraints properly.
     * See constraints/Matching.h for a description of why this is necessary. */
    foreach( int key, cmt.uniqueKeys() ) {
        QList<int> vals = cmt.values( key );
        // set up the blank matching array
        QBitArray m;
        if ( m_matchtype == MatchAny ) {
            m = QBitArray( l.size(), false );
        } else {
            m = QBitArray( l.size(), true );
        }

        // combine the arrays from the appropriate constraints
        for( int v : vals ) {
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
    Q_EMIT dataChanged();
}

void
ConstraintGroup::setMatchAll()
{
    m_matchtype = MatchAll;
    Q_EMIT dataChanged();
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
        Q_EMIT clickedMatchAll();
        Q_EMIT updated();
    }
}

void
ConstraintGroupEditWidget::on_radioButton_MatchAny_clicked( bool c )
{
    if ( c ) {
        Q_EMIT clickedMatchAny();
        Q_EMIT updated();
    }
}
