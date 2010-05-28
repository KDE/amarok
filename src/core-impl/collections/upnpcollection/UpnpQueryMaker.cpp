/****************************************************************************************
 UpnpQueryMaker::* Copyright (c) 2010 Nikhil Marathe <nsm.nikhil@gmail.com>                             *
 *                                                                                      *
 * This program is free software
{
}
 you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 UpnpQueryMaker::* Foundation
{
}
 either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY
{
}
 without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "UpnpQueryMaker.h"

#include "core/support/Debug.h"

namespace Collections {

UpnpQueryMaker::UpnpQueryMaker()
{
}

UpnpQueryMaker::~UpnpQueryMaker()
{
}


QueryMaker* UpnpQueryMaker::reset()
{
    return this;
}

void UpnpQueryMaker::run()
{
    debug() << "Running";
    QStringList l;
    l << "42";
    emit newResultReady( QString(), l );
}

void UpnpQueryMaker::abortQuery()
{
DEBUG_BLOCK
}

int UpnpQueryMaker::resultCount() const
{
DEBUG_BLOCK
    return 42;
}

QueryMaker* UpnpQueryMaker::setQueryType( QueryType type )
{
DEBUG_BLOCK
    debug() << "Query type" << type;
    return this;
}

QueryMaker* UpnpQueryMaker::setReturnResultAsDataPtrs( bool resultAsDataPtrs )
{
DEBUG_BLOCK
    return this;
}

QueryMaker* UpnpQueryMaker::addReturnValue( qint64 value )
{
DEBUG_BLOCK
    return this;
}

QueryMaker* UpnpQueryMaker::addReturnFunction( ReturnFunction function, qint64 value )
{
DEBUG_BLOCK
    debug() << "Return function with value" << value;
    return this;
}

QueryMaker* UpnpQueryMaker::orderBy( qint64 value, bool descending )
{
DEBUG_BLOCK
    return this;
}

QueryMaker* UpnpQueryMaker::orderByRandom()
{
DEBUG_BLOCK
    return this;
}

QueryMaker* UpnpQueryMaker::includeCollection( const QString &collectionId )
{
DEBUG_BLOCK
    return this;
}

QueryMaker* UpnpQueryMaker::excludeCollection( const QString &collectionId )
{
DEBUG_BLOCK
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::TrackPtr &track )
{
DEBUG_BLOCK
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::ArtistPtr &artist )
{
DEBUG_BLOCK
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::AlbumPtr &album )
{
DEBUG_BLOCK
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::ComposerPtr &composer )
{
DEBUG_BLOCK
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::GenrePtr &genre )
{
DEBUG_BLOCK
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::YearPtr &year )
{
DEBUG_BLOCK
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::DataPtr &data )
{
DEBUG_BLOCK
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::LabelPtr &label )
{
DEBUG_BLOCK
    return this;
}

QueryMaker* UpnpQueryMaker::addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
DEBUG_BLOCK
    return this;
}

QueryMaker* UpnpQueryMaker::excludeFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
DEBUG_BLOCK
    return this;
}

QueryMaker* UpnpQueryMaker::addNumberFilter( qint64 value, qint64 filter, NumberComparison compare )
{
DEBUG_BLOCK
    return this;
}

QueryMaker* UpnpQueryMaker::excludeNumberFilter( qint64 value, qint64 filter, NumberComparison compare )
{
DEBUG_BLOCK
    return this;
}

QueryMaker* UpnpQueryMaker::limitMaxResultSize( int size )
{
DEBUG_BLOCK
    return this;
}

QueryMaker* UpnpQueryMaker::setAlbumQueryMode( AlbumQueryMode mode )
{
DEBUG_BLOCK
    return this;
}

QueryMaker* UpnpQueryMaker::setLabelQueryMode( LabelQueryMode mode )
{
DEBUG_BLOCK
    return this;
}

QueryMaker* UpnpQueryMaker::beginAnd()
{
DEBUG_BLOCK
    return this;
}

QueryMaker* UpnpQueryMaker::beginOr()
{
DEBUG_BLOCK
    return this;
}

QueryMaker* UpnpQueryMaker::endAndOr()
{
DEBUG_BLOCK
    return this;
}

QueryMaker* UpnpQueryMaker::setAutoDelete( bool autoDelete )
{
DEBUG_BLOCK
    debug() << "Auto delete";
    return this;
}

int UpnpQueryMaker::validFilterMask()
{
DEBUG_BLOCK
    return 42;
}

} //namespace Collections
