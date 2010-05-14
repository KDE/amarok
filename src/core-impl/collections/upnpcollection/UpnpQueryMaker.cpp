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
}

int UpnpQueryMaker::resultCount() const
{
    return 42;
}

QueryMaker* UpnpQueryMaker::setQueryType( QueryType type )
{
    debug() << "Query type" << type;
    return this;
}

QueryMaker* UpnpQueryMaker::setReturnResultAsDataPtrs( bool resultAsDataPtrs )
{
    return this;
}

QueryMaker* UpnpQueryMaker::addReturnValue( qint64 value )
{
    return this;
}

QueryMaker* UpnpQueryMaker::addReturnFunction( ReturnFunction function, qint64 value )
{
    debug() << "Return function with value" << value;
    return this;
}

QueryMaker* UpnpQueryMaker::orderBy( qint64 value, bool descending )
{
    return this;
}

QueryMaker* UpnpQueryMaker::orderByRandom()
{
    return this;
}

QueryMaker* UpnpQueryMaker::includeCollection( const QString &collectionId )
{
    return this;
}

QueryMaker* UpnpQueryMaker::excludeCollection( const QString &collectionId )
{
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::TrackPtr &track )
{
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::ArtistPtr &artist )
{
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::AlbumPtr &album )
{
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::ComposerPtr &composer )
{
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::GenrePtr &genre )
{
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::YearPtr &year )
{
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::DataPtr &data )
{
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::LabelPtr &label )
{
    return this;
}

QueryMaker* UpnpQueryMaker::addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
    return this;
}

QueryMaker* UpnpQueryMaker::excludeFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
    return this;
}

QueryMaker* UpnpQueryMaker::addNumberFilter( qint64 value, qint64 filter, NumberComparison compare )
{
    return this;
}

QueryMaker* UpnpQueryMaker::excludeNumberFilter( qint64 value, qint64 filter, NumberComparison compare )
{
    return this;
}

QueryMaker* UpnpQueryMaker::limitMaxResultSize( int size )
{
    return this;
}

QueryMaker* UpnpQueryMaker::setAlbumQueryMode( AlbumQueryMode mode )
{
    return this;
}

QueryMaker* UpnpQueryMaker::setLabelQueryMode( LabelQueryMode mode )
{
    return this;
}

QueryMaker* UpnpQueryMaker::beginAnd()
{
    return this;
}

QueryMaker* UpnpQueryMaker::beginOr()
{
    return this;
}

QueryMaker* UpnpQueryMaker::endAndOr()
{
    return this;
}

QueryMaker* UpnpQueryMaker::setAutoDelete( bool autoDelete )
{
    debug() << "Auto delete";
    return this;
}

int UpnpQueryMaker::validFilterMask()
{
    return 42;
}

} //namespace Collections
