/****************************************************************************************
 * Copyright (c) 2012 Phalgun Guduthur <me@phalgun.in>                                  *
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

#include "NepomukQueryMakerInternal.h"

#include "NepomukCollection.h"
#include "core-impl/collections/support/MemoryCustomValue.h"
#include "core-impl/collections/support/MemoryFilter.h"
#include "core-impl/collections/support/MemoryQueryMakerHelper.h"

#include "core/meta/Meta.h"

#include <KSortableList>

namespace Collections
{

NepomukQueryMakerInternal::NepomukQueryMakerInternal( NepomukCollection *collection )
    : QObject()
    , m_collection( collection )
//    , m_matchers( 0 )
    , m_filters( 0 )
    , m_maxSize( 0 )
    , m_type( QueryMaker::None )
    , m_albumQueryMode( QueryMaker::AllAlbums )
    , m_artistQueryMode( QueryMaker::TrackArtists )
    , m_orderDescending( false )
    , m_orderByNumberField( false )
    , m_orderByField( 0 )
{

}

NepomukQueryMakerInternal::~NepomukQueryMakerInternal()
{
    delete m_filters;
    //    delete m_matchers;
    qDeleteAll( m_returnFunctions );
    qDeleteAll( m_returnValues );
}

void
NepomukQueryMakerInternal::runQuery()
{

}

template <class PointerType>
void NepomukQueryMakerInternal::emitProperResult( const QList<PointerType>& list )
{
    QList<PointerType> resultList = list;

    if( m_maxSize >= 0 && resultList.count() > m_maxSize )
        resultList = resultList.mid( 0, m_maxSize );

    emit newResultReady( list );
}

template<typename T>
static inline QList<T> reverse( const QList<T> &l )
{
    QList<T> ret;
    for( int i = l.size() - 1; i >= 0; --i )
        ret.append( l.at( i ) );
    return ret;
}

void
NepomukQueryMakerInternal::handleResult()
{

}

void
NepomukQueryMakerInternal::handleResult( const Meta::TrackList &tmpTracks )
{

}

//void
//NepomukQueryMakerInternal::setMatchers( MemoryMatcher *matchers )
//{
////    m_matchers = matchers;
//}

void
NepomukQueryMakerInternal::setFilters( MemoryFilter *filters )
{
    m_filters = filters;
}

void
NepomukQueryMakerInternal::setMaxSize( int maxSize )
{
    m_maxSize = maxSize;
}

void
NepomukQueryMakerInternal::setType( QueryMaker::QueryType type )
{
    m_type = type;
}

void
NepomukQueryMakerInternal::setCustomReturnFunctions( const QList<CustomReturnFunction *> &functions )
{
    m_returnFunctions = functions;
}

void
NepomukQueryMakerInternal::setCustomReturnValues( const QList<CustomReturnValue *> &values )
{
    m_returnValues = values;
}

}//namespace Collections
