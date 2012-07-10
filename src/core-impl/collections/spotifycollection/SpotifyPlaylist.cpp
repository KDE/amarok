#include "SpotifyPlaylist.h"

using Playlists;

SpotifyPlaylist::SpotifyPlaylist( Collections::SpotifyCollection* collection, SpotifyPlaylistProvider* provider)
    :QObject( 0 )
, m_name( i18n("New playlist") )
, m_sync( true )
, m_collection( collection )
, m_provider( provider )
{

}

SpotifyPlaylist::SpotifyPlaylist( const QString& name, const Meta::SpotifyTracklist& tracks,
                                  Collections::SpotifyCollection* collection,, SpotifyPlaylistProvider* provider, const sync = true )
    :QObject( 0 )
, m_name( name )
, m_sync( sync )
, m_collection( collection )
, m_provider( provider )
{
    //TODO: Add tracks to playlist
}

SpotifyPlaylist::~SpotifyPlaylist()
{
}

KUrl
SpotifyPlaylist::uidUrl() const
{
    //TODO: Return a generated unique id
}

QString
SpotifyPlaylist::name() const
{
    return m_name;
}

PlaylistProvider*
SpotifyPlaylist::provider() const
{
    return m_provider;
}

void
SpotifyPlaylist::setName( const QString& name )
{
    //TODO: Might need to notify the provider the name has been changed
    m_name = name;
}

int
SpotifyPlaylist::trackCount() const
{
    //TODO: Might return a reasonable number when loaded
    return -1;
}

Meta::TrackList
SpotifyPlaylist::tracks()
{
    //TODO: Return all tracks in the playlist
}

void
SpotifyPlaylist::triggerTrackLoad()
{
    // This should be called from external methods
    // then start a new thread to load all track information vid SpotifyCollection
    //TODO: Load track details
}

void
SpotifyPlaylist::addTrack( Meta::TrackPtr track, int position )
{
    //TODO: Add a new track to the playlist
}

void
SpotifyPlaylist::removeTrack( int position )
{
    //TODO: Remove a track at position `position`
}

