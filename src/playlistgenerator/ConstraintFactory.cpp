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

#define DEBUG_PREFIX "APG::ConstraintFactory"

#include "Constraint.h"
#include "ConstraintFactory.h"
#include "ConstraintGroup.h"
#include "ConstraintNode.h"
#include "constraints/Checkpoint.h"
#include "constraints/PlaylistDuration.h"
#include "constraints/PlaylistFileSize.h"
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

ConstraintFactoryEntry::ConstraintFactoryEntry( const QString& name,
                                                const QString& i18nN,
                                                const QString& desc,
                                                Constraint*( *xmlf )( QDomElement&, ConstraintNode* ),
                                                Constraint*( *nf )( ConstraintNode* ) )
        : m_name( name )
        , m_i18nName( i18nN )
        , m_description( desc )
        , m_createFromXmlFunc( xmlf )
        , m_createNewFunc( nf )
{
}

/******************************************
 * Constraint Factory Singleton           *
 ******************************************/

ConstraintFactory* ConstraintFactory::s_self = nullptr;

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
    ConstraintFactory::s_self = nullptr;
}

ConstraintFactory::ConstraintFactory()
{
    ConstraintFactoryEntry* r = nullptr;

    r = ConstraintTypes::TagMatch::registerMe();
    m_registryIds[0] = r;
    m_registryNames[r->m_name] = r;
    m_registryUntranslateNames[r->m_i18nName] = r->m_name;

    r = ConstraintTypes::PlaylistDuration::registerMe();
    m_registryIds[1] = r;
    m_registryNames[r->m_name] = r;
    m_registryUntranslateNames[r->m_i18nName] = r->m_name;

    r = ConstraintTypes::PlaylistLength::registerMe();
    m_registryIds[2] = r;
    m_registryNames[r->m_name] = r;
    m_registryUntranslateNames[r->m_i18nName] = r->m_name;

    r = ConstraintTypes::PreventDuplicates::registerMe();
    m_registryIds[3] = r;
    m_registryNames[r->m_name] = r;
    m_registryUntranslateNames[r->m_i18nName] = r->m_name;

    r = ConstraintTypes::Checkpoint::registerMe();
    m_registryIds[4] = r;
    m_registryNames[r->m_name] = r;
    m_registryUntranslateNames[r->m_i18nName] = r->m_name;

    r = ConstraintTypes::PlaylistFileSize::registerMe();
    m_registryIds[5] = r;
    m_registryNames[r->m_name] = r;
    m_registryUntranslateNames[r->m_i18nName] = r->m_name;

    // ADD NEW CONSTRAINT TYPES HERE FOLLOWING SAME PATTERN (DON'T FORGET TO INCREMENT ID)
}

ConstraintFactory::~ConstraintFactory()
{
    qDeleteAll( m_registryIds );
}

ConstraintNode* ConstraintFactory::createConstraint( QDomElement& xmlelem, ConstraintNode* parent, int row ) const
{
    QString t = xmlelem.attributeNode( QStringLiteral("type") ).value();
    if ( !m_registryNames.contains( t ) || !parent )
        return nullptr;

    ConstraintNode* n = ( *( m_registryNames[t]->m_createFromXmlFunc ) )( xmlelem, parent );
    parent->addChild( n, row );
    return n;
}

ConstraintNode* ConstraintFactory::createConstraint( const QString& name, ConstraintNode* parent, int row ) const
{
    if ( !m_registryNames.contains( name ) || !parent )
        return nullptr;

    ConstraintNode* n = ( *( m_registryNames[name]->m_createNewFunc ) )( parent );
    parent->addChild( n, row );
    return n;
}

ConstraintNode* ConstraintFactory::createConstraint( const int idx, ConstraintNode* parent, int row ) const
{
    if ( !m_registryIds.contains( idx ) || !parent )
        return nullptr;

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

const QStringList ConstraintFactory::i18nNames() const
{
    return m_registryUntranslateNames.keys();
}

QList< QPair<int, QString> > ConstraintFactory::registeredConstraints() const
{
    QList< QPair<int, QString> > d;
    for( int i : m_registryIds.keys() ) {
        d.append( QPair<int, QString>( i, m_registryIds[i]->m_name ) );
    }
    return d;
}

const QString ConstraintFactory::untranslateName( const QString& trn ) const
{
    return m_registryUntranslateNames.value( trn );
}
