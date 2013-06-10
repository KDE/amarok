/****************************************************************************************
 * Copyright (c) 2012 Phalgun Guduthur <me@phalgun.in>                                  *
 * Copyright (c) 2013 Edward Toroshchin <amarok@hades.name>                             *
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

#include "NepomukLabel.h"

#include "../NepomukCache.h"
#include "../NepomukCollection.h"

#include "core/meta/Meta.h"

#include <Nepomuk2/Tag>

using namespace Meta;

NepomukLabel::NepomukLabel( const QString &name )
    : m_nepomukTag( new Nepomuk2::Tag )
{
    m_nepomukTag->setLabel( name );
}

NepomukLabel::NepomukLabel( const QUrl &resourceUri )
    : m_nepomukTag( new Nepomuk2::Tag( resourceUri ) )
{
}

NepomukLabel::~NepomukLabel()
{
}

QString
NepomukLabel::name() const
{
    return m_nepomukTag->label();
}

LabelPtr
NepomukLabel::fromNepomukTag( Collections::NepomukCollection *collection,
                              const Nepomuk2::Tag &tag )
{
    if( !collection ) return LabelPtr();
    if( tag.uri().isEmpty() ) return LabelPtr();
    return collection->cache()->getLabel( tag.uri() );
}
