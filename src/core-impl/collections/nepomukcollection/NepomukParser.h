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

#ifndef AMAROK_COLLECTION_NEPOMUKPARSER_H
#define AMAROK_COLLECTION_NEPOMUKPARSER_H

#include "core/meta/Meta.h"

#include <Soprano/QueryResultIterator>

#include <QObject>
#include <QSet>
#include <QStringList>

namespace Collections {

class NepomukCollection;

/**
 * This is an interface for classes that parse the results of the
 * NepomukQueryMaker's Soprano queries.
 *
 * The parse() method is called by NepomukInquirer with the query result
 * iterator.
 *
 * The implementations of this interface should return the results by emitting
 * the appropriate newResultReady() signals.
 *
 * The instances of this class will be constructed in a non-main thread by the
 * NepomukInquirer, so don't do anything nasty in constructors and methods (no
 * GUI, no non-thread-safe methods of non-local objects, etc.)
 */
class NepomukParser: public QObject
{
    Q_OBJECT

public:
    /**
     * Construct a NepomukParser.
     *
     * @param coll must be a valid NepomukCollection
     */
    NepomukParser( NepomukCollection *coll );

    /**
     * Parse all the query results in the given QueryResultIterator and emit
     * newResultReady with the constructed objects.
     */
    virtual void parse( Soprano::QueryResultIterator& ) = 0;

Q_SIGNALS:
    void newResultReady( Meta::TrackList );
    void newResultReady( Meta::ArtistList );
    void newResultReady( Meta::AlbumList );
    void newResultReady( Meta::GenreList );
    void newResultReady( Meta::ComposerList );
    void newResultReady( Meta::YearList );
    void newResultReady( QStringList );
    void newResultReady( Meta::LabelList );

protected:
    bool parseOne( Soprano::QueryResultIterator &queryResult,
                   Meta::TrackList &objectList );
    bool parseOne( Soprano::QueryResultIterator &queryResult,
                   Meta::ArtistList &objectList );
    bool parseOne( Soprano::QueryResultIterator &queryResult,
                   Meta::AlbumList &objectList );
    bool parseOne( Soprano::QueryResultIterator &queryResult,
                   Meta::GenreList &objectList );
    bool parseOne( Soprano::QueryResultIterator &queryResult,
                   Meta::ComposerList &objectList );
    bool parseOne( Soprano::QueryResultIterator &queryResult,
                   Meta::YearList &objectList );
    bool parseOne( Soprano::QueryResultIterator &queryResult,
                   QStringList &objectList );
    bool parseOne( Soprano::QueryResultIterator &queryResult,
                   Meta::LabelList &objectList );

private:
    NepomukCollection *m_collection;
    QSet<QUrl> m_seen_uri;
};

template< class MetaObjectList >
class NepomukObjectParser: public NepomukParser
{
public:
    NepomukObjectParser( NepomukCollection *coll )
        : NepomukParser( coll )
    {}

    virtual void parse( Soprano::QueryResultIterator &queryResult )
    {
        MetaObjectList result;

        while( queryResult.next() )
            parseOne( queryResult, result );

        emit newResultReady( result );
    }
};

typedef NepomukObjectParser< Meta::TrackList > NepomukTrackParser;
typedef NepomukObjectParser< Meta::ArtistList > NepomukArtistParser;
typedef NepomukObjectParser< Meta::AlbumList > NepomukAlbumParser;
typedef NepomukObjectParser< Meta::GenreList > NepomukGenreParser;
typedef NepomukObjectParser< Meta::ComposerList > NepomukComposerParser;
typedef NepomukObjectParser< Meta::YearList > NepomukYearParser;
typedef NepomukObjectParser< QStringList > NepomukCustomParser;
typedef NepomukObjectParser< Meta::LabelList > NepomukLabelParser;

}

#endif // AMAROK_COLLECTION_NEPOMUKPARSER_H
