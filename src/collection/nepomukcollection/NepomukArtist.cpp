/****************************************************************************************
 * Copyright (c) 2008 Daniel Winter <dw@danielwinter.de>                                *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "NepomukArtist.h"

#include "NepomukCollection.h"
#include "NepomukQueryMaker.h"

#include "Debug.h"
#include "Meta.h"

#include <QString>


using namespace Meta;

NepomukArtist::NepomukArtist( NepomukCollection *collection, const QString &name )
        : Artist()
        , m_collection( collection )
        , m_name( name )
        , m_albumsLoaded( false )
{
}

QString
NepomukArtist::name() const
{
    return m_name;
}

QString
NepomukArtist::prettyName() const
{
    return m_name;
}

QString
NepomukArtist::sortableName() const
{
    if ( m_sortName.isEmpty() && !m_name.isEmpty() )
    {
        if ( m_name.startsWith( "the ", Qt::CaseInsensitive ) )
        {
            QString begin = m_name.left( 3 );
            m_sortName = QString( "%1, %2" ).arg( m_name, begin );
            m_sortName = m_sortName.mid( 4 );
        }
        else
        {
            m_sortName = m_name;
        }
    }
    return m_sortName;
}

TrackList
NepomukArtist::tracks()
{
    return TrackList();
}

AlbumList
NepomukArtist::albums()
{
    if( m_albumsLoaded )
    {
        return m_albums;
    }
    else if( m_collection )
    {
        NepomukQueryMaker *qm = static_cast<NepomukQueryMaker*>( m_collection->queryMaker() );
        qm->setQueryType( QueryMaker::Album );
        addMatchTo( qm );
        qm->blocking( true );
        qm->run();
        m_albums = qm->albums( m_collection->collectionId() );
        delete qm;
        m_albumsLoaded = true;
        return m_albums;
    }
    else
    {
        return AlbumList();
    }
}

void
NepomukArtist::emptyCache()
{
    // FIXME: add lock
    m_albums.clear();
    m_albumsLoaded = false;
}
