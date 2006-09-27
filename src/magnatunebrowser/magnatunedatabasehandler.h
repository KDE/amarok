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
* This class wraps the database operations needed by the MagnatuneBrowser
* Uses the singleton pattern
*
* @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class MagnatuneDatabaseHandler{
public:
    

    /**
     * Function for retrieving the singleton
     * @return pointer to the singleton
     */
    static MagnatuneDatabaseHandler * instance();

    ~MagnatuneDatabaseHandler();

    /**
     * Creates the tables needed to store Magnatune info
     */
    void createDatabase();

    /**
     * Destroys Magnatune tables
     */
    void destroyDatabase();

    /**
     * Inserts a new track into the Magnatune database
     * @param track pointer to the track to insert 
     * @param albumId id of the album containing the track
     * @param artistId id of the artist performing the track
     * @return the database id of the newly inserted track
     */
    int insertTrack(MagnatuneTrack *track, int albumId, int artistId);

    /**
     * inserts a new album into the Magnatune database
     * @param album pointer to the album to insert
     * @param artistId id of the artist performing the album
     * @return the database id of the newly inserted album
     */
    int insertAlbum(MagnatuneAlbum *album, int artistId);
   
    /**
     * inserts a new artist into the Magnatune database
     * @param artist pointer to the artist to insert
     * @return the database id of the newly inserted artist
     */
    int insertArtist(MagnatuneArtist *artist);

    //get id, or -1 if artist does not exist
    /**
     * Retrieves the id of a named artist
     * @param name artist name to retrieve
     * @return id of artist. -1 if no artist is found
     */
    int getArtistIdByExactName(QString name);


    /**
     * Retrieves the id of an album based on its unique album code.
     * @param albumcode The album code.
     * @return The id of the album, -1 if not foud.
     */
    int getAlbumIdByAlbumCode( QString albumcode );

    /**
     * Returns all artist that has albums in a given genre. If an artist has both a Rock
     * and a Techno album, he will be included when searching for either
     * @param genre the genre
     * @return  A list of artist in the genre
     */
    MagnatuneArtistList getArtistsByGenre(QString genre);

    /**
     * Returns the artist with a given id
     * @param id The id of the artist to look for
     * @return The artist with the given id. Returns an empty artist if not found.
     */
    MagnatuneArtist getArtistById(int id);

    /**
     * Returns the album with a given id
     * @param id The id of the album to look for
     * @return The album with the given id. Returns an empty album if not found.
     */
    MagnatuneAlbum getAlbumById(int id);

    /**
     * Retrieves all albums by a single artist from the database
     * @param id The id of the artist
     * @param genre Limits the albums to a specific genre. Use "All" to get all albums
     * @return List of albums. empty if none are found
     */
    MagnatuneAlbumList getAlbumsByArtistId(int id, QString genre);

    /**
     * Retrieves all tracks on a given album
     * @param id The id of the album
     * @return A list of tracks. Empty if album is not found or has no tracks
     */
    MagnatuneTrackList getTracksByAlbumId(int id);

    /**
     * Retrieves all tracks by given artist
     * @param id The id of the artist
     * @return A list of tracks. Empty if artist is not found, artist has no albums or albums have no tracks
     */
    MagnatuneTrackList getTracksByArtistId(int id); 

    /**
     * Retrieves a list of all genres present in the databse
     * @return A list of genres
     */
    QStringList getAlbumGenres();

    /**
     * Begins a database transaction. Must be followed by a later call to commit()
     */
    void begin();

    /**
     * Completes (executes) a database transaction. Must be preceded by a call to begin()
     */
    void commit();


protected:

    /**
     * Private constructor (singleton pattern)
     * @return Pointer to new object
     */
    MagnatuneDatabaseHandler();
    static MagnatuneDatabaseHandler * m_pInstance;

};

#endif
