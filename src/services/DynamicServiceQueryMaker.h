/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#ifndef DYNAMICSERVICEQUERYMAKER_H
#define DYNAMICSERVICEQUERYMAKER_H

#include "Meta.h"
#include "QueryMaker.h"
#include "ServiceCollection.h"
#include "amarok_export.h"

#include <kio/jobclasses.h>

namespace ThreadWeaver
{
    class Job;
}


/**
A base class for implementing custom querymakers that fetch data from an external source.
Basically just stubs out the stuff that not every dynamic querymaker will need

    @author
*/
class AMAROK_EXPORT DynamicServiceQueryMaker : public QueryMaker
{
Q_OBJECT
public:
    DynamicServiceQueryMaker( );
    virtual ~DynamicServiceQueryMaker() {};

    //this is the stuff that must be implmeneted
    virtual QueryMaker* reset() = 0;
    virtual void run() = 0;
    virtual void abortQuery() = 0;

    virtual QueryMaker* setReturnResultAsDataPtrs ( bool resultAsDataPtrs ) = 0;


    //below here is the stuf that each dynamic querymaker will most likely only need
    //Some of, hense they are all stubbed out:

    virtual QueryMaker* setQueryType( QueryType type ) { Q_UNUSED( type); return this; }

    virtual QueryMaker* addReturnValue ( qint64 value );
    virtual QueryMaker* addReturnFunction( ReturnFunction function, qint64 value );
    virtual QueryMaker* orderBy ( qint64 value, bool descending = false );
    virtual QueryMaker* orderByRandom();

    virtual QueryMaker* includeCollection ( const QString &collectionId );
    virtual QueryMaker* excludeCollection ( const QString &collectionId );

    virtual QueryMaker* addMatch ( const Meta::TrackPtr &track );
    virtual QueryMaker* addMatch ( const Meta::ArtistPtr &artist );
    virtual QueryMaker* addMatch ( const Meta::AlbumPtr &album );
    virtual QueryMaker* addMatch ( const Meta::GenrePtr &genre );
    virtual QueryMaker* addMatch ( const Meta::ComposerPtr &composer );
    virtual QueryMaker* addMatch ( const Meta::YearPtr &year );

    //this function must be reimplemented if the QueryMaker accepts any kind of qualifiers
    virtual QueryMaker* addMatch ( const Meta::DataPtr &data );

    virtual QueryMaker* addFilter ( qint64 value, const QString &filter, bool matchBegin, bool matchEnd );
    virtual QueryMaker* excludeFilter ( qint64 value, const QString &filter, bool matchBegin, bool matchEnd );

    virtual QueryMaker* addNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare );
    virtual QueryMaker* excludeNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare );

    virtual QueryMaker* limitMaxResultSize ( int size );

    virtual QueryMaker* beginAnd() { return this; }
    virtual QueryMaker* beginOr() { return this; }
    virtual QueryMaker* endAndOr() { return this; }
};

#endif
