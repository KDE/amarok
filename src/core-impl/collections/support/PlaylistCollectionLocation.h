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

#ifndef PLAYLISTCOLLECTIONLOCATION_H
#define PLAYLISTCOLLECTIONLOCATION_H

#include "core/collections/CollectionLocation.h"

namespace Collections {

class PlaylistCollection;

/**
  * Utility class that allows modification of playlists using the standard
  * CollectionLocation API.
  * caveat: although it is writable, moving tracks to this "collection" does not
  * work, as it is just a wrapper around a playlist, which only holds references to tracks
  *
  * It is safe though as this collection does not signal that it has copied tracks successfully
  */
class PlaylistCollectionLocation : public CollectionLocation
{
public:
    PlaylistCollectionLocation( const PlaylistCollection *collection );

    QString prettyLocation() const;
    bool isWritable() const;
    bool remove( const Meta::TrackPtr &track );

protected:
    void copyUrlsToCollection( const QMap<Meta::TrackPtr, KUrl> &sources );
    //why is this called "removeUrls" if the argument are only tracks?
    void removeUrlsFromCollection( const Meta::TrackList &tracks );

private:
    const PlaylistCollection *m_collection;
};

} //namespace Collections

#endif // PLAYLISTCOLLECTIONLOCATION_H
