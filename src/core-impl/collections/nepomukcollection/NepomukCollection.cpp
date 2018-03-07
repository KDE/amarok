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

#define DEBUG_PREFIX "NepomukCollection"

#include "NepomukCollection.h"

#include "NepomukCache.h"
#include "NepomukQueryMaker.h"

#include "core/meta/Meta.h"
#include "core/meta/support/MetaKeys.h"
#include "core/support/Debug.h"

#include <Nepomuk2/Resource>
#include <Nepomuk2/ResourceManager>
#include <Nepomuk2/Vocabulary/NFO>

#include <QIcon>

namespace Collections
{

NepomukCollection::NepomukCollection()
    : m_cache( new NepomukCache( this ) )
{
}

NepomukCollection::~NepomukCollection()
{
}

QueryMaker*
NepomukCollection::queryMaker()
{
    return new NepomukQueryMaker( this );
}

QString
NepomukCollection::uidUrlProtocol() const
{
    return QLatin1String( "amarok-nepomuk" );
}

QString
NepomukCollection::collectionId() const
{
    return QString( "%1://" ).arg( uidUrlProtocol() );
}

QString
NepomukCollection::prettyName() const
{
    return i18n( "Nepomuk Collection" );
}

QIcon
NepomukCollection::icon() const
{
    return QIcon::fromTheme( "nepomuk" );
}

bool
NepomukCollection::isWritable() const
{
    // right now NepomukCollectionLocation isn't implemented, which deals with moving
    // and copying tracks into collection. So false until that is implemented.

    return false;
}

bool
NepomukCollection::possiblyContainsTrack( const QUrl & ) const
{
    return true;
}

Meta::TrackPtr
NepomukCollection::trackForUrl( const QUrl &url )
{
    DEBUG_BLOCK
    debug() << url;
    return Meta::TrackPtr(); // TODO
}

}
