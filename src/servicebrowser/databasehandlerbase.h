/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#ifndef DATABASEHANDLERBASE_H
#define DATABASEHANDLERBASE_H

#include "simpleservicetypes.h"

#include <QStringList>



/**
* This class defines the database operations that must be implemented 
* for a database handler to be used with the generic database driven model
*
* @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class DatabaseHandlerBase{
public:
    

    /**
     * Function for retrieving the singleton
     * @return pointer to the singleton
     */
   // static DatabaseHandlerBase * instance();


        /**
     * Private constructor (singleton pattern)
     * @return Pointer to new object
     */
    DatabaseHandlerBase() {}

    virtual ~DatabaseHandlerBase() {}

    /**
     * Creates the tables needed to store Jamendo info
     */
    virtual void createDatabase() = 0;

    /**
     * Destroys Jamendo tables
     */
    virtual void destroyDatabase() = 0;

    /**
     * Inserts a new track into the database
     * @param track pointer to the track to insert 
     * @param albumId id of the album containing the track
     * @param artistId id of the artist performing the track
     * @return the database id of the newly inserted track
     */
    //virtual int insertTrack( SimpleServiceTrack *track ) = 0;

    /**
     * inserts a new album into the database
     * @param album pointer to the album to insert
     * @param artistId id of the artist performing the album
     * @return the database id of the newly inserted album
     */
    //virtual int insertAlbum( SimpleServiceAlbum *album ) = 0;
   
    /**
     * inserts a new artist into the database
     * @param artist pointer to the artist to insert
     * @return the database id of the newly inserted artist
     */
    //virtual int insertArtist( SimpleServiceArtist *artist ) = 0;

    //get id, or -1 if artist does not exist
    /**
     * Retrieves the id of a named artist
     * @param name artist name to retrieve
     * @return id of artist. -1 if no artist is found
     */
    virtual int getArtistIdByExactName(const QString &name) = 0;

    /**
     * Returns all artist that has albums in a given genre. If an artist has both a Rock
     * and a Techno album, he will be included when searching for either
     * @param genre the genre
     * @return  A list of artist in the genre
     */
    virtual SimpleServiceArtistList getArtistsByGenre( const QString &genre ) = 0;

    /**
     * Returns the artist with a given id
     * @param id The id of the artist to look for
     * @return The artist with the given id. Returns an empty artist if not found.
     */
    virtual SimpleServiceArtist * getArtistById( int id ) = 0;

    /**
     * Returns the album with a given id
     * @param id The id of the album to look for
     * @return The album with the given id. Returns an empty album if not found.
     */
    virtual SimpleServiceAlbum * getAlbumById( int id ) = 0;


     /**
     * Returns the track with a given id
     * @param id The id of the track to look for
     * @return The track with the given id. Returns an empty album if not found.
     */
    virtual SimpleServiceTrack * getTrackById( int id ) = 0;

     /**
     * Retrieves all albums by a single artist from the database
     * @param id The id of the artist
     * @param genre Limits the albums to a specific genre. Use "All" to get all albums
     * @return List of albums. empty if none are found
     */
    virtual SimpleServiceAlbumList getAlbumsByArtistId(int id, const QString &genre) = 0;

     /**
     * Retrieves all tracks on a given album
     * @param id The id of the album
     * @return A list of tracks. Empty if album is not found or has no tracks
     */
    virtual SimpleServiceTrackList getTracksByAlbumId(int id) = 0;

     /**
     * Retrieves a list of all genres present in the databse
     * @return A list of genres
     */
    virtual QStringList getAlbumGenres() = 0;


    /**
     * Begins a database transaction. Must be followed by a later call to commit()
     */
    void begin();

    /**
     * Completes (executes) a database transaction. Must be preceded by a call to begin()
     */
    void commit();


protected:


    //static DatabaseHandlerBase * m_pInstance;

};

#endif
