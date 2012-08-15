/****************************************************************************************
 * Copyright (c) 2012 Ryan Feng <odayfans@gmail.com>                                    *
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
#include "Query.h"
#include "core/support/Debug.h"

#include <QTimer>

namespace Spotify{

Query::Query( Collections::SpotifyCollection* collection, const QString& qid,
              const QString& title, const QString& artist, const QString& album,
              const QString& genre )
:QObject( 0 )
, m_qid( qid )
, m_title( title )
, m_artist( artist )
, m_album( album )
, m_genre( genre )
, m_timeout( 3000 ) // Default timeout is 3 seconds
, m_collection( collection )
{
    // Set timeout, the query will be deleted automatically after timedout
    QTimer::singleShot( m_timeout, this, SLOT( slotTimedout() ) );
}

Query::~Query()
{
    DEBUG_BLOCK
}

QString
Query::getFullQueryString() const
{
    DEBUG_BLOCK
    QString query;
    if( m_title == m_album && m_title == m_artist && m_title == m_genre )
    {
        query = m_title;
        return query;
    }
    if( !m_title.isEmpty() )
    {
        QString trimmedTitle = m_title;
        trimmedTitle.replace( "-", " " );
        query += QString( "title:'%1' " ).arg( trimmedTitle );
    }

    if( !m_artist.isEmpty() )
    {
        QString trimmedArtist = m_artist;
        trimmedArtist.replace( "feat.", " ", Qt::CaseInsensitive );
        query += QString( "artist:'%1' " ).arg( trimmedArtist );
    }

    if( !m_genre.isEmpty() )
    {
        query += QString( "genre:'%1' " ).arg( m_genre );
    }

    if( !m_album.isEmpty() )
    {
        query += QString( "album:'%1' ").arg(m_album);
    }

    return query;
}

void
Query::slotTracksAdded( const Meta::SpotifyTrackList& trackList )
{
    DEBUG_BLOCK
    m_results = trackList;
    if( trackList.isEmpty() )
    {
        warning() << "Empty track list";
    }

    foreach( Meta::SpotifyTrackPtr track, trackList )
    {
        track->addToCollection( m_collection );
    }

    emit newTrackList( trackList );
    emit queryDone( qid() );
}

void
Query::slotTimedout()
{
    emit queryError( QueryError ( ETimedOut, QString( "Query(%1) timed out!" ).arg( qid() ) ) );
    emit queryDone( qid() );

}

void
Query::slotAbortQuery()
{
    emit queryDone( qid() );
}

} // namespace Spotify

#include "Query.moc"
