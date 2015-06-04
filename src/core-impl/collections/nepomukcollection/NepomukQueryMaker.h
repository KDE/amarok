/****************************************************************************************
 * Copyright (c) 2013 Edward Toroshchin <amarok@hades.name>
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

#ifndef AMAROK_COLLECTION_NEPOMUKQUERYMAKER_H
#define AMAROK_COLLECTION_NEPOMUKQUERYMAKER_H

#include "core/collections/QueryMaker.h"

namespace Collections {

class NepomukCollection;

class NepomukQueryMakerPrivate;

/**
 * This class implements a QueryMaker for a given NepomukCollection.
 */
class NepomukQueryMaker : public QueryMaker
{
    Q_OBJECT

    NepomukQueryMakerPrivate *d;
    NepomukCollection *myCollection;

public:
    explicit NepomukQueryMaker( NepomukCollection *collection );
    virtual ~NepomukQueryMaker();

    virtual void abortQuery();
    virtual void run();

    virtual QueryMaker *setQueryType( QueryType type );

    virtual QueryMaker *addMatch( const Meta::TrackPtr &track );
    virtual QueryMaker *addMatch( const Meta::ArtistPtr &artist, ArtistMatchBehaviour behaviour = TrackArtists );
    virtual QueryMaker *addMatch( const Meta::AlbumPtr &album );
    virtual QueryMaker *addMatch( const Meta::ComposerPtr &composer );
    virtual QueryMaker *addMatch( const Meta::GenrePtr &genre );
    virtual QueryMaker *addMatch( const Meta::YearPtr &year );
    virtual QueryMaker *addMatch( const Meta::LabelPtr &label );

    virtual QueryMaker *addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd );
    virtual QueryMaker *excludeFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd );

    virtual QueryMaker *addNumberFilter( qint64 value, qint64 filter, NumberComparison compare );
    virtual QueryMaker *excludeNumberFilter( qint64 value, qint64 filter, NumberComparison compare );

    virtual QueryMaker *addReturnValue( qint64 value );
    virtual QueryMaker *addReturnFunction( ReturnFunction function, qint64 value );
    virtual QueryMaker *orderBy( qint64 value, bool descending = false );

    virtual QueryMaker *limitMaxResultSize( int size );

    virtual QueryMaker *setAlbumQueryMode( AlbumQueryMode mode );
    virtual QueryMaker *setLabelQueryMode( LabelQueryMode mode );

    virtual QueryMaker *beginAnd();
    virtual QueryMaker *beginOr();
    virtual QueryMaker *endAndOr();

private Q_SLOTS:
    void inquirerDone();
};

} //namespace Collections

#endif
