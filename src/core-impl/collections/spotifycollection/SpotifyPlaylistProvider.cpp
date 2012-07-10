#include "SpotifyPlaylistProvider.h"

using Playlists;

SpotifyPlaylistProvider::SpotifyPlaylistProvider( Collections::SpotifyCollection* collection, QObject* parent = 0 )
: UserPlaylistProvider( parent )
, m_collection( collection )
, m_controller( collection->controller() )
{

}

SpotifyPlaylistProvider::~SpotifyPlaylistProvider()
{
}

QString
SpotifyPlaylistProvider::prettyName() const
{
    return i18n("Spotify playlists");
}

KIcon
SpotifyPlaylistProvider::icon() const
{
    return KIcon("SpotifyPlaylistProvider");
}

int
SpotifyPlaylistProvider::playlistCount() const
{
    return -1;
}

PlaylistList
SpotifyPlaylistProvider::playlists()
{
    //TODO: Get and return all playlists of Spotify via SpotifyCollection
}

PlaylistPtr
SpotifyPlaylistProvider::addPlaylist( PlaylistPtr playlist )
{
    //TODO: Add new playlist to Spotify via SpotifyCollection
}

PlaylistPtr
SpotifyPlaylistProvider::save( const Meta::TrackList& tracks,
                               const QString& name = QString() )
{
    //TODO: Save all playlists to Spotify via SpotifyCollection
}

QActionList
SpotifyPlaylistProvider::providerActions()
{
}

QActionList
SpotifyPlaylistProvider::playlistActions()
{
}

QActionList
SpotifyPlaylistProvider::trackActions()
{
}

bool
SpotifyPlaylistProvider::isWriteable()
{
    return true;
}

void
SpotifyPlaylistProvider::rename( PlaylistPtr playlist, const QString& newName )
{
    //TODO: Rename a playlist via SpotifyCollection
}

bool
SpotifyPlaylistProvider::deletePlaylists( PlaylistList playlistList )
{
    //TODO: Delete a list of playlists via SpotifyCollection
}

void
SpotifyPlaylistProvider::trackAdded( PlaylistPtr playlist, Meta::TrackPtr track, int position )
{
    //TODO: Add a new track to a playlist to position `position` via SpotifyCollection
}

void
SpotifyPlaylistProvider::trackRemoved( PlaylistPtr playlist, int position )
{
    //TODO: Remove a track from a playlist at position `position` via SpotifyCollection
}

void
SpotifyPlaylistProvider::setSync( SpotifyPlaylistPtr, bool sync )
{
    //TODO: Set a playlist to auto sync mode if sync is true
}

bool
SpotifyPlaylistProvider::sync( SpotifyPlaylistPtr playlistPtr )
{
    //TODO: Return the sync status of playlist playlistPtr
}
