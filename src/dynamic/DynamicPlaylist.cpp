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


#include "DynamicPlaylist.h"
#include "CollectionManager.h"

using namespace Meta;

DynamicPlaylist::DynamicPlaylist()
    : m_collection( CollectionManager::instance()->primaryCollection() ),
      m_upcoming(10), m_previous(5)
      
{
}

DynamicPlaylist::DynamicPlaylist( Collection* coll )
    : m_collection( coll ),
      m_upcoming(10), m_previous(5)
{
}

DynamicPlaylist::~DynamicPlaylist()
{
}


Meta::TrackList
DynamicPlaylist::getTracks( int count )
{
    Meta::TrackList tracks;
    while( count-- ) tracks.append( getTrack() );
    
    return tracks;
}

void
DynamicPlaylist::recalculate()
{
    // do nothing by default
}


// TODO: DynamicModel must know about this, or bad things will happen
QString DynamicPlaylist::title() const     { return m_title; }

int DynamicPlaylist::upcomingCount() const { return m_upcoming; }
int DynamicPlaylist::previousCount() const { return m_previous; }

void DynamicPlaylist::setTitle( QString title )        { m_title = title; }
void DynamicPlaylist::setUpcomingCount( int upcoming ) { m_upcoming = upcoming; }
void DynamicPlaylist::setPreviousCount( int previous ) { m_previous = previous; }


