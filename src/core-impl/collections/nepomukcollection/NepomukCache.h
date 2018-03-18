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

#ifndef AMAROK_COLLECTION_NEPOMUKCACHE_H
#define AMAROK_COLLECTION_NEPOMUKCACHE_H

#include "core/meta/forward_declarations.h"

#include <QObject>

class QString;
class QUrl;

namespace Collections {

class NepomukCollection;

class NepomukCachePrivate;

/**
 * The storage/cache thingy for objects retrieved from Nepomuk.
 *
 * The Nepomuk tracks, artists, albums, composers and labels are identified by
 * their resource URIs. This class provides a one-to-one mapping from Nepomuk
 * URIs to respective Amarok Meta::Nepomuk* objects.
 *
 * The genres and years are identified by their literal values. This class
 * provides a one-to-one mapping from the literal values to Amarok
 * Meta::NepomukGenre and Meta::NepomukYear objects.
 *
 * You can convert a Nepomuk URI (or literal value for genres and years) to a
 * Meta object using the appropriate get*() method. If the object isn't known to
 * NepomukCache, it will create a new empty object and return a pointer to it.
 * All subsequent requests with the same URI (or literal value) will return
 * pointers to the same object.
 *
 * NepomukCache is non-copyable and should only be constructed by
 * NepomukCollection.
 *
 * All functions are thread-safe.
 */
class NepomukCache: public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(NepomukCache)

    NepomukCachePrivate *d;

    explicit NepomukCache( NepomukCollection *parent );
    ~NepomukCache();

    friend class NepomukCollection;

public:
    Meta::TrackPtr getTrack( const QUrl &resourceUri );
    Meta::ArtistPtr getArtist( const QUrl &resourceUri );
    Meta::AlbumPtr getAlbum( const QUrl &resourceUri );
    Meta::ComposerPtr getComposer( const QUrl &resourceUri );
    Meta::GenrePtr getGenre( const QString &genre );
    Meta::YearPtr getYear( const int year );
    Meta::LabelPtr getLabel( const QUrl &resourceUri );
};

}

#endif // AMAROK_COLLECTION_NEPOMUKCACHE_H
