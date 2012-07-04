/****************************************************************************************
 * Copyright (c) 2008 Daniel Winter <dw@danielwinter.de>                                *
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

#ifndef NEPOMUKQUERYMAKER_H
#define NEPOMUKQUERYMAKER_H

#include "NepomukCollection.h"

#include "amarok_export.h"
#include "core/collections/QueryMaker.h"

namespace ThreadWeaver
{
class Job;
}

namespace Collections
{

// TODO: Check if AMAROK_EXPORT has to be included
class AMAROK_EXPORT NepomukQueryMaker : public QueryMaker
{
    Q_OBJECT

public:
    NepomukQueryMaker( NepomukCollection *collection );
    virtual ~NepomukQueryMaker();

    virtual void run();
    virtual void abortQuery();

    virtual QueryMaker* setQueryType( QueryType type );

    virtual QueryMaker* addReturnValue( qint64 value );
    virtual QueryMaker* addReturnFunction( ReturnFunction function,
                                           qint64 value );

    virtual QueryMaker* orderBy( qint64 value, bool descending = false );

    virtual QueryMaker* addMatch( const Meta::TrackPtr &track );
    virtual QueryMaker* addMatch( const Meta::ArtistPtr &artist );
    virtual QueryMaker* addMatch( const Meta::AlbumPtr &album );
    virtual QueryMaker* addMatch( const Meta::ComposerPtr &composer );
    virtual QueryMaker* addMatch( const Meta::GenrePtr &genre );
    virtual QueryMaker* addMatch( const Meta::YearPtr &year );
    virtual QueryMaker* addMatch( const Meta::LabelPtr &label );

    virtual QueryMaker* addFilter( qint64 value, const QString &filter,
                                   bool matchBegin = false,
                                   bool matchEnd = false );
    virtual QueryMaker* excludeFilter( qint64 value, const QString &filter,
                                       bool matchBegin = false,
                                       bool matchEnd = false );

    virtual Collections::QueryMaker* addNumberFilter( qint64 value, qint64 filter,
            NumberComparison compare );
    virtual QueryMaker* excludeNumberFilter( qint64 value, qint64 filter,
            NumberComparison compare );

    virtual QueryMaker* limitMaxResultSize( int size );

    virtual QueryMaker* beginAnd();
    virtual QueryMaker* beginOr();
    virtual QueryMaker* endAndOr();

    virtual QueryMaker* setAlbumQueryMode( AlbumQueryMode mode );
    virtual QueryMaker* setArtistQueryMode( ArtistQueryMode mode );
    virtual QueryMaker* setLabelQueryMode( LabelQueryMode mode );

private slots:
    void done( ThreadWeaver::Job * job );

protected:
    NepomukCollection *m_collection;

    struct Private;
    Private * const d;

};

} // namespace Collections

#endif // NEPOMUKQUERYMAKER_H
