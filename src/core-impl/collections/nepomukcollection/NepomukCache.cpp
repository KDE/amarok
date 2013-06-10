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

#include "NepomukCache.h"

#include "NepomukCollection.h"
#include "meta/NepomukAlbum.h"
#include "meta/NepomukArtist.h"
#include "meta/NepomukComposer.h"
#include "meta/NepomukGenre.h"
#include "meta/NepomukLabel.h"
#include "meta/NepomukTrack.h"
#include "meta/NepomukYear.h"

#include <QHash>
#include <QMutex>
#include <QMutexLocker>

namespace Collections {

class NepomukCachePrivate
{
public:
    QHash< QUrl, Meta::TrackPtr > trackMap;
    QHash< QUrl, Meta::ArtistPtr > artistMap;
    QHash< QUrl, Meta::AlbumPtr > albumMap;
    QHash< QUrl, Meta::ComposerPtr > composerMap;
    QHash< QString, Meta::GenrePtr > genreMap;
    QHash< int, Meta::YearPtr > yearMap;
    QHash< QUrl, Meta::LabelPtr > labelMap;

    QMutex lock;

    template< class NepomukObject, class Key, class Ptr >
    Ptr getOrCreate( QHash< Key, Ptr > &hash, const Key &key )
    {
        QMutexLocker locker(&lock);
        Ptr &ptr( hash[key] );
        if( !ptr )
        {
            // Create a new empty object. The empty object must be filled
            // by NepomukParser before being used
            ptr = new NepomukObject( key );
        }

        return ptr;
    }
};

NepomukCache::NepomukCache( NepomukCollection *parent )
    : QObject( parent )
    , d( new NepomukCachePrivate )
{
}

NepomukCache::~NepomukCache()
{
    delete d;
    d = 0;
}

Meta::TrackPtr
NepomukCache::getTrack( const QUrl &resourceUri )
{
    return d->getOrCreate< Meta::NepomukTrack >( d->trackMap, resourceUri );
}

Meta::ArtistPtr
NepomukCache::getArtist( const QUrl &resourceUri )
{
    return d->getOrCreate< Meta::NepomukArtist >( d->artistMap, resourceUri );
}

Meta::AlbumPtr
NepomukCache::getAlbum( const QUrl &resourceUri )
{
    return d->getOrCreate< Meta::NepomukAlbum >( d->albumMap, resourceUri );
}

Meta::ComposerPtr
NepomukCache::getComposer( const QUrl &resourceUri )
{
    return d->getOrCreate< Meta::NepomukComposer >( d->composerMap, resourceUri );
}

Meta::GenrePtr
NepomukCache::getGenre( const QString &genre )
{
    return d->getOrCreate< Meta::NepomukGenre >( d->genreMap, genre );
}

Meta::YearPtr
NepomukCache::getYear( const int year )
{
    return d->getOrCreate< Meta::NepomukYear >( d->yearMap, year );
}

Meta::LabelPtr
NepomukCache::getLabel( const QUrl &label )
{
    return d->getOrCreate< Meta::NepomukLabel >( d->labelMap, label );
}

}
