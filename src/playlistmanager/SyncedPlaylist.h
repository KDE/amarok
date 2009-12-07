#ifndef SYNCEDPLAYLIST_H
#define SYNCEDPLAYLIST_H

#include <src/core/playlists/Playlist.h>

class SyncedPlaylist : public Playlists::Playlist, public Playlists::PlaylistObserver
{
    public:
        SyncedPlaylist();
        SyncedPlaylist( Playlists::PlaylistPtr playlist );
        virtual ~SyncedPlaylist() {}

        //Playlists::Playlist methods
        virtual KUrl uidUrl() const;
        virtual QString name() const;
        virtual QString prettyName() const;
        virtual QString description() const;
        virtual void setName( const QString &name ) { Q_UNUSED( name ); }

        virtual Meta::TrackList tracks();

        virtual void addTrack( Meta::TrackPtr track, int position = -1 );
        virtual void removeTrack( int position );

        //PlaylistObserver methods
        virtual void trackAdded( Playlists::PlaylistPtr playlist, Meta::TrackPtr track,
                                 int position );
        virtual void trackRemoved( Playlists::PlaylistPtr playlist, int position );

        //SyncedPlaylist methods
        /** returns true when there is no active playlist associated with it anymore. */
        virtual bool isEmpty() const;
        virtual void addPlaylist( Playlists::PlaylistPtr playlist );

        virtual bool syncNeeded() const;
        virtual void doSync() const;

        virtual void removePlaylistsFrom( Playlists::PlaylistProvider *provider );

    protected:

    private:
        Playlists::PlaylistList m_playlists;
};

typedef KSharedPtr<SyncedPlaylist> SyncedPlaylistPtr;
typedef QList<SyncedPlaylistPtr> SyncedPlaylistList;

Q_DECLARE_METATYPE( SyncedPlaylistPtr )
Q_DECLARE_METATYPE( SyncedPlaylistList )

#endif // SYNCEDPLAYLIST_H
