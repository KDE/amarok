/***************************************************************************
 * copyright            : (C) 2008 Daniel Jones <danielcjones@gmail.com> 
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy 
 * defined in Section 14 of version 3 of the license.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

#include "RandomPlaylist.h"

#include "BlockingQuery.h"
#include "Collection.h"
#include "Debug.h"

#include <KRandom>

using namespace Meta;


RandomPlaylist::RandomPlaylist() : DynamicPlaylist()
{
    m_title = "Random";
}

RandomPlaylist::RandomPlaylist( Collection* c ) : DynamicPlaylist( c )
{
    m_title = "Random";
}


RandomPlaylist::~RandomPlaylist() 
{
}


Meta::TrackPtr
RandomPlaylist::getTrack()
{
    if( m_cache.empty() ) FillCache();

    if( m_cache.empty() )
    {
        warning() << "RandomPlaylist is not delivering.";
        return TrackPtr();
    }

    int i = KRandom::random() % m_cache.size();
    TrackPtr tr = m_cache[i];
    m_cache.removeAt( i );

    return tr;
}


void
RandomPlaylist::FillCache()
{
    QueryMaker* qm = m_collection->queryMaker();
    qm->orderByRandom();
    qm->startTrackQuery();
    qm->limitMaxResultSize( CACHE_SIZE - m_cache.size() );

    BlockingQuery bq( qm );

    bq.startQuery();

    QList<Meta::TrackList> trackLists = bq.tracks().values();
    foreach( Meta::TrackList ts, trackLists )
    {
        foreach( Meta::TrackPtr t, ts )
        {
            m_cache.append( t );
        }
    }
}



