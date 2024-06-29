/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "core/meta/forward_declarations.h"
#include "core/collections/QueryMaker.h"
#include "ServiceCollection.h"
#include "amarok_export.h"

namespace ThreadWeaver
{
}

namespace Collections {

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
    ~DynamicServiceQueryMaker() override {}

    //this is the stuff that must be implemented
    void run() override = 0;
    void abortQuery() override = 0;

    //below here is the stuf that each dynamic querymaker will most likely only need
    //Some of, hence they are all stubbed out:

    QueryMaker* setQueryType( QueryType type ) override { Q_UNUSED( type); return this; }

    QueryMaker* addReturnValue ( qint64 value ) override;
    QueryMaker* addReturnFunction( ReturnFunction function, qint64 value ) override;
    QueryMaker* orderBy ( qint64 value, bool descending = false ) override;

    QueryMaker* addMatch ( const Meta::TrackPtr &track ) override;
    QueryMaker* addMatch ( const Meta::ArtistPtr &artist, ArtistMatchBehaviour behaviour = TrackArtists ) override;
    QueryMaker* addMatch ( const Meta::AlbumPtr &album ) override;
    QueryMaker* addMatch ( const Meta::GenrePtr &genre ) override;
    QueryMaker* addMatch ( const Meta::ComposerPtr &composer ) override;
    QueryMaker* addMatch ( const Meta::YearPtr &year ) override;
    QueryMaker* addMatch ( const Meta::LabelPtr &label ) override;

    QueryMaker* addFilter ( qint64 value, const QString &filter, bool matchBegin, bool matchEnd ) override;
    QueryMaker* excludeFilter ( qint64 value, const QString &filter, bool matchBegin, bool matchEnd ) override;

    QueryMaker* addNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare ) override;
    QueryMaker* excludeNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare ) override;

    QueryMaker* limitMaxResultSize ( int size ) override;

    QueryMaker* beginAnd() override { return this; }
    QueryMaker* beginOr() override { return this; }
    QueryMaker* endAndOr() override { return this; }

    static Meta::AlbumList matchAlbums( ServiceCollection *coll, const Meta::ArtistPtr &artist );
};

} //namespace Collections

#endif
