#include "core/playlists/PlaylistProvider.h"
#include "PlaylistProvider.moc"

using namespace Playlists;

PlaylistPtr
PlaylistProvider::addPlaylist( Playlists::PlaylistPtr playlist )
{
    Q_UNUSED( playlist );
    return PlaylistPtr();
}

Meta::TrackPtr
PlaylistProvider::addTrack( Meta::TrackPtr track )
{
    Q_UNUSED( track );
    return Meta::TrackPtr();
}
