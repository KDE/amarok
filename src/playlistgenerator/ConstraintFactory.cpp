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

#define DEBUG_PREFIX "APG::ConstraintFactory"

#include "Constraint.h"
#include "ConstraintFactory.h"
#include "ConstraintGroup.h"
#include "ConstraintNode.h"
#include "constraints/Checkpoint.h"
#include "constraints/PlaylistLength.h"
#include "constraints/PreventDuplicates.h"
#include "constraints/TagMatch.h"

#include "core/support/Debug.h"

#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>

/******************************************
 * Constraint Factory Registry Entries    *
 ******************************************/

ConstraintFactoryEntry::ConstraintFactoryEntry( const QString& name, const QString& desc,
        Constraint*( *xmlf )( QDomElement&, ConstraintNode* ), Constraint*( *nf )( ConstraintNode* ) )
        : m_name( name )
        , m_description( desc )
        , m_createFromXmlFunc( xmlf )
        , m_createNewFunc( nf )
{
}

/******************************************
 * Constraint Factory Singleton           *
 ******************************************/

ConstraintFactory* ConstraintFactory::s_self = 0;

ConstraintFactory* ConstraintFactory::instance()
{
    if ( !ConstraintFactory::s_self ) {
        ConstraintFactory::s_self = new ConstraintFactory();
    }
    return ConstraintFactory::s_self;
}

void ConstraintFactory::destroy()
{
    delete ConstraintFactory::s_self;
    ConstraintFactory::s_self = 0;
}

ConstraintFactory::ConstraintFactory()
{
    ConstraintFactoryEntry* r = 0;

    r = ConstraintTypes::TagMatch::registerMe();
    m_registryIds[0] = r;
    m_registryNames[r->m_name] = r;

    r = ConstraintTypes::PlaylistLength::registerMe();
    m_registryIds[1] = r;
    m_registryNames[r->m_name] = r;

    r = ConstraintTypes::PreventDuplicates::registerMe();
    m_registryIds[2] = r;
    m_registryNames[r->m_name] = r;

    /*
     * This is far from being implemented, so comment out for now. -- sth
    r = ConstraintTypes::Checkpoint::registerMe();
    m_registryIds[4] = r;
    m_registryNames[r->m_name] = r;
    */

    // ADD NEW CONSTRAINT TYPES HERE FOLLOWING SAME PATTERN (DON'T FORGET TO INCREMENT ID)
}

ConstraintFactory::~ConstraintFactory()
{
    foreach( ConstraintFactoryEntry* e, m_registryIds ) {
        delete e;
    }
}

ConstraintNode* ConstraintFactory::createConstraint( QDomElement& xmlelem, ConstraintNode* parent, int row ) const
{
    QString t = xmlelem.attributeNode( "type" ).value();
    if ( !m_registryNames.contains( t ) || !parent )
        return 0;

    ConstraintNode* n = ( *( m_registryNames[t]->m_createFromXmlFunc ) )( xmlelem, parent );
    parent->addChild( n, row );
    return n;
}

ConstraintNode* ConstraintFactory::createConstraint( const QString& name, ConstraintNode* parent, int row ) const
{
    if ( !m_registryNames.contains( name ) || !parent )
        return 0;

    ConstraintNode* n = ( *( m_registryNames[name]->m_createNewFunc ) )( parent );
    parent->addChild( n, row );
    return n;
}

ConstraintNode* ConstraintFactory::createConstraint( const int idx, ConstraintNode* parent, int row ) const
{
    if ( !m_registryIds.contains( idx ) || !parent )
        return 0;

    ConstraintNode* n = ( *( m_registryIds[idx]->m_createNewFunc ) )( parent );
    parent->addChild( n, row );
    return n;
}

ConstraintNode* ConstraintFactory::createGroup( QDomElement& xmlelem, ConstraintNode* parent, int row ) const
{
    ConstraintNode* n = ConstraintGroup::createFromXml( xmlelem, parent );
    if ( parent )
        parent->addChild( n, row );
    return n;
}

ConstraintNode* ConstraintFactory::createGroup( ConstraintNode* parent, int row ) const
{
    ConstraintNode* n = ConstraintGroup::createNew( parent );
    if ( parent )
        parent->addChild( n, row );
    return n;
}

const QStringList ConstraintFactory::names() const
{
    return m_registryNames.keys();
}

QList< QPair<int, QString> > ConstraintFactory::registeredConstraints() const
{
    QList< QPair<int, QString> > d;
    foreach( int i, m_registryIds.keys() ) {
        d.append( QPair<int, QString>( i, m_registryIds[i]->m_name ) );
    }
    return d;
}

int ConstraintFactory::getTypeId( const QString& name ) const
{
    foreach( int i, m_registryIds.keys() ) {
        if ( m_registryIds[i]->m_name == name )
            return i;
    }
    return -1;
}
