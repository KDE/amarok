#ifndef TRACK_PROXY_H
#define TRACK_PROXY_H

#include "Controller.h"
#include "../SpotifyMeta.h"
#include "../SpotifyCollection.h"
#include "core-impl/meta/proxy/MetaProxy.h"

namespace Spotify
{

class TrackProxy : public QObject
{
    Q_OBJECT
    public:
        TrackProxy( const KUrl& url, MetaProxy::TrackPtr trackPtr, Collections::SpotifyCollection* collection );
        ~TrackProxy();

    signals:
        void spotifyError( Spotify::Controller::ErrorState );

    private slots:
        void slotSpotifyError( Spotify::Controller::ErrorState );
        void slotTrackResolved( const Meta::SpotifyTrackList& trackList );
        void slotQueryDone( Spotify::Query* query, const Meta::SpotifyTrackList& trackList );

    private:
        QPointer< Collections::SpotifyCollection > m_collection;
        QWeakPointer< Spotify::Controller > m_controller;
        MetaProxy::TrackPtr m_proxyTrackPtr;
        Spotify::Query* m_query;

};

} // namespace Spotify

#endif
