#include "TrackProxy.h"

namespace Spotify
{

TrackProxy::TrackProxy( const KUrl& url, MetaProxy::TrackPtr trackPtr, Collections::SpotifyCollection* collection )
: QObject( 0 )
, m_collection( collection )
, m_controller( collection->controller() )
, m_proxyTrackPtr( trackPtr )
, m_query( 0 )
{
    connect( m_controller.data(), SIGNAL( spotifyError( Spotify::Controller::ErrorState ) ),
             this, SLOT( slotSpotifyError( Spotify::Controller::ErrorState ) ) );

    m_query = m_controller.data()->makeQuery( m_collection.data(),
                                      url.queryItem( "title" ),
                                      url.queryItem( "artist" ),
                                      url.queryItem( "album" ),
                                      url.queryItem( "genre" ) );
    m_controller.data()->resolve( m_query );
    Q_ASSERT( m_query != 0 );
}

TrackProxy::~TrackProxy()
{
}

void TrackProxy::slotSpotifyError( Spotify::Controller::ErrorState e )
{
    emit spotifyError( e );
    deleteLater();
}

void TrackProxy::slotTrackResolved( const Meta::SpotifyTrackList& trackList )
{
    if( trackList.isEmpty() )
        return;

    Meta::SpotifyTrackPtr trackPtr = trackList[0];
    Meta::TrackPtr realTrack;
    if( m_collection )
    {
        trackPtr->addToCollection( m_collection );
        realTrack = m_collection->trackForUrl( trackPtr->uidUrl() );
    }
    else
        realTrack = Meta::TrackPtr::staticCast( trackPtr );

    m_proxyTrackPtr->updateTrack( realTrack );
}

void TrackProxy::slotQueryDone( Spotify::Query* query, const Meta::SpotifyTrackList& trackList )
{
    Q_UNUSED( query );
    Q_UNUSED( trackList );
    deleteLater();
}

} // namespace Spotify

#include "TrackProxy.moc"
