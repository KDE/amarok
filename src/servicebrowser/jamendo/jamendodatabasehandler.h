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

#ifndef JAMENDODATABASEHANDLER_H
#define JAMENDODATABASEHANDLER_H

#include "collectiondb.h"
#include "databasehandlerbase.h"
#include "jamendotypes.h"


#include <QStringList>
#include <QMap>



/**
* This class wraps the database operations needed by the JamendoBrowser
* Uses the singleton pattern
*
* @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class JamendoDatabaseHandler : public DatabaseHandlerBase {
public:
    
     //static DatabaseHandlerBase * instance();
 
    
    /**
     * Private constructor (singleton pattern)
     * @return Pointer to new object
     */
    JamendoDatabaseHandler();


    ~JamendoDatabaseHandler();

    /**
     * Creates the tables needed to store Jamendo info
     */
    void createDatabase();

    /**
     * Destroys Jamendo tables
     */
    void destroyDatabase();

    /**
     * Inserts a new track into the Jamendo database
     * @param track pointer to the track to insert 
     * @param albumId id of the album containing the track
     * @param artistId id of the artist performing the track
     * @return the database id of the newly inserted track
     */
    int insertTrack( ServiceTrack *track );

    /**
     * inserts a new album into the Jamendo database
     * @param album pointer to the album to insert
     * @param artistId id of the artist performing the album
     * @return the database id of the newly inserted album
     */
    int insertAlbum( ServiceAlbum *album );
   
    /**
     * inserts a new artist into the Jamendo database
     * @param artist pointer to the artist to insert
     * @return the database id of the newly inserted artist
     */
    int insertArtist( ServiceArtist *artist );

    //get id, or -1 if artist does not exist
    /**
     * Retrieves the id of a named artist
     * @param name artist name to retrieve
     * @return id of artist. -1 if no artist is found
     */
    int getArtistIdByExactName(const QString &name);

    /**
     * Returns all artist that has albums in a given genre. If an artist has both a Rock
     * and a Techno album, he will be included when searching for either
     * @param genre the genre
     * @return  A list of artist in the genre
     */
    ArtistList getArtistsByGenre( const QString &genre );

    /**
     * Returns the artist with a given id
     * @param id The id of the artist to look for
     * @return The artist with the given id. Returns an empty artist if not found.
     */
    ServiceArtist * getArtistById( int id );

    /**
     * Returns the album with a given id
     * @param id The id of the album to look for
     * @return The album with the given id. Returns an empty album if not found.
     */
    ServiceAlbum * getAlbumById( int id );


     /**
     * Returns the track with a given id
     * @param id The id of the track to look for
     * @return The track with the given id. Returns an empty album if not found.
     */
    ServiceTrack * getTrackById( int id );

     /**
     * Retrieves all albums by a single artist from the database
     * @param id The id of the artist
     * @param genre Limits the albums to a specific genre. Use "All" to get all albums
     * @return List of albums. empty if none are found
     */
    AlbumList getAlbumsByArtistId(int id, const QString &genre);

     /**
     * Retrieves all tracks on a given album
     * @param id The id of the album
     * @return A list of tracks. Empty if album is not found or has no tracks
     */
    TrackList getTracksByAlbumId(int id);

     /**
     * Retrieves a list of all genres present in the databse
     * @return A list of genres
     */
    QStringList getAlbumGenres();



protected:

};

#endif
