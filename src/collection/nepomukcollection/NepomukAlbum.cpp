/* 
   Copyright (C) 2008 Daniel Winter <dw@danielwinter.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include "NepomukAlbum.h"

#include "NepomukArtist.h"
#include "NepomukCollection.h"

#include "BlockingQuery.h"
#include "Meta.h"

#include <QString>

using namespace Meta;

NepomukAlbum::NepomukAlbum( NepomukCollection *collection, const QString &name, const QString &artist )
        : Album()
        , m_collection( collection )
        , m_name( name )
        , m_artist( artist )
        , m_tracksLoaded( false )
{
}

QString
NepomukAlbum::name() const
{
    return m_name;
}

QString
NepomukAlbum::prettyName() const
{
    return m_name;
}

TrackList
NepomukAlbum::tracks()
{
    if( m_tracksLoaded )
    {
        return m_tracks;
    }
    else if( m_collection )
    {
        QueryMaker *qm = m_collection->queryMaker();
        qm->setQueryType( QueryMaker::Track );
        addMatchTo( qm );
        BlockingQuery bq( qm );
        bq.startQuery();
        m_tracks = bq.tracks( m_collection->collectionId() );
        m_tracksLoaded = true;
        return m_tracks;
    }
    else
        return TrackList(); 
}

bool
NepomukAlbum::isCompilation() const
{
    return false;
}

bool
NepomukAlbum::hasAlbumArtist() const
{
    return true;
}

ArtistPtr
NepomukAlbum::albumArtist() const
{
    return ArtistPtr( new NepomukArtist( m_collection, m_artist ) );
}
