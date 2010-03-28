/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#ifndef PLAYLISTCOLLECTION_H
#define PLAYLISTCOLLECTION_H

#include "core/collections/Collection.h"
#include "MemoryCollection.h"
#include "core/playlists/Playlist.h"

#include <QSharedPointer>

namespace Collections {

class MemoryCollection;

/**
  * Utility class that wraps a playlist as collection and makes it possible to
  * query the content of the playlist using QueryMaker.
  */
class PlaylistCollection : public Collections::Collection, public Playlists::PlaylistObserver
{
public:
    PlaylistCollection( const Playlists::PlaylistPtr &playlist );
    virtual ~PlaylistCollection();

    virtual QString collectionId() const;
    virtual QString prettyName() const;
    virtual QueryMaker* queryMaker();
    virtual CollectionLocation* location() const;

    KIcon icon() const; //why is this pure virtual?

    virtual void trackAdded( Playlists::PlaylistPtr playlist, Meta::TrackPtr track, int position ) = 0;
    virtual void trackRemoved( Playlists::PlaylistPtr playlist, int position ) = 0;

    Playlists::PlaylistPtr playlist() const;
private:
    void insertTrack( const Meta::TrackPtr &track );

    Playlists::PlaylistPtr m_playlist;

    QSharedPointer<Collections::MemoryCollection> m_mc;
};

} //namespace Collections

#endif // PLAYLISTCOLLECTION_H
