/*
  Copyright (c) 2006  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#ifndef MAGNATUNEDATABASEHANDLER_H
#define MAGNATUNEDATABASEHANDLER_H

#include "collectiondb.h"
#include "magnatunetypes.h"

#include <qstringlist.h>



/**
This class wraps the database operations needed by the MagnatuneBrowser

	@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class MagnatuneDatabaseHandler{
public:
    

    static MagnatuneDatabaseHandler * instance();

    ~MagnatuneDatabaseHandler();

    void createDatabase();
    void destroyDatabase();

    int insertTrack(MagnatuneTrack * track, int albumId, int artistId);
    int insertTracks(MagnatuneTrackList tracks, int artistId, int albumId);
    int insertAlbum(MagnatuneAlbum * album, int artistId);
   
    int insertArtist(MagnatuneArtist * artist);

    //get id, or -1 if artist does not exist
    int GetArtistIdByExactName(QString name);

    MagnatuneArtistList getArtistsByGenre(QString genre);
    MagnatuneArtist 	getArtistById(int id);
    MagnatuneAlbum 	getAlbumById(int id);
    MagnatuneAlbumList  getAlbumsByArtistId(int id, QString genre);
    MagnatuneTrackList  getTracksByAlbumId(int id);
    MagnatuneTrackList  getTracksByArtistId(int id);  //used for adding all tracks by a given artist to playlist

    QStringList getAlbumGenres();

    void begin();
    void commit();


protected:

    MagnatuneDatabaseHandler();
    static MagnatuneDatabaseHandler * m_pInstance;

};

#endif
