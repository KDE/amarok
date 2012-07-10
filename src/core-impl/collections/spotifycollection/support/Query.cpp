#include "Query.h"

namespace Spotify{

Query::Query( const QString& qid, const QString& title, const QString& artist, const QString& album, const QString& genre )
:QObject( 0 )
, m_qid( qid )
, m_title( title )
, m_artist( artist )
, m_album( album )
, m_genre( genre )
{
    //TODO: Connect singals&slots
}

Query::~Query()
{
    //DONE:TODO: Notify controller to remove current query from queue
    emit queryFinished( qid() );
}

QString
Query::getFullQueryString()
{
    QString query;
    if( !m_title.isEmpty() )
    {
        QString trimmedTitle = m_title.replace( "-", " " );
        query += QString( "title:'%1' " ).arg( trimmedTitle );
    }

    if( !m_artist.isEmpty() )
    {
        QString trimmedArtist = m_artist.replace( "feat.", " ", Qt::CaseInsensitive );
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
Query::tracksAdded( const Meta::SpotifyTrackListPtr trackListPtr )
{
    m_results = trackListPtr;
    emit newTrackList( trackListPtr );
}

void
Query::timedOut()
{
    //TODO: Abort current query, notify the controller to remove current query from queue
    emit queryError( ETimedOut );

    // Auto delete self
    this->deleteLater();

    emit queryFinished( qid() );
}

void
Query::queryAborted()
{
    //TODO: Notify the controller to remove the query from queue
    this->deleteLater();

    emit queryFinished( qid() );
}

} // namespace Spotify
