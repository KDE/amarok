#include "Query.h"
#include "core/support/Debug.h"

namespace Spotify{

Query::Query( Collections::SpotifyCollection* collection, const QString& qid, const QString& title, const QString& artist, const QString& album, const QString& genre )
:QObject( 0 )
, m_qid( qid )
, m_title( title )
, m_artist( artist )
, m_album( album )
, m_genre( genre )
, m_collection( collection )
{
    //TODO: Connect singals&slots
}

Query::~Query()
{
    //DONE:TODO: Notify controller to remove current query from queue
    emit queryDone( qid() );
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
Query::tracksAdded( const Meta::SpotifyTrackList& trackList )
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
    emit queryDone( this, trackList );
    emit queryDone( qid() );
}

void
Query::timedOut()
{
    //TODO: Abort current query, notify the controller to remove current query from queue
    emit queryError( QueryError ( ETimedOut, QString( "Query(%1) timed out!" ).arg( qid() ) ) );

    // Auto delete self
    this->deleteLater();

    emit queryDone( qid() );
}

void
Query::abortQuery()
{
    //TODO: Notify the controller to remove the query from queue
    this->deleteLater();

    emit queryDone( qid() );
}

} // namespace Spotify
