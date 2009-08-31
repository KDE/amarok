/****************************************************************************************
 * Copyright (c) 2006,2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>               *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef MAGNATUNEDATABASEHANDLER_H
#define MAGNATUNEDATABASEHANDLER_H

#include "MagnatuneMeta.h"

#include <QStringList>
#include <QMap>

/**
* This class wraps the database operations needed by the MagnatuneStore
* Uses the singleton pattern
*
* @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class MagnatuneDatabaseHandler {
public:


    /**
     * Private constructor (singleton pattern)
     * @return Pointer to new object
     */
    MagnatuneDatabaseHandler();


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
     * @return the database id of the newly inserted track
     */
    int insertTrack( Meta::ServiceTrack *track );

    /**
     * inserts a new album into the Magnatune database
     * @param album pointer to the album to insert
     * @return the database id of the newly inserted album
     */
    int insertAlbum( Meta::ServiceAlbum *album );

    /**
     * inserts a new artist into the Magnatune database
     * @param artist pointer to the artist to insert
     * @return the database id of the newly inserted artist
     */
    int insertArtist( Meta::ServiceArtist *artist );

    /**
     * inserts a new genre into the Magnatune database
     * @param genre pointer to the genre to insert
     * @return the database id of the newly inserted genre
     */
    int insertGenre( Meta::ServiceGenre *genre );


    void insertMoods(int trackId, const QStringList &moods);

     /**
     * Retrieves the id of a named artist
     * @param name artist name to retrieve
     * @return id of artist. -1 if no artist is found
     */
    int getArtistIdByExactName(const QString &name);

    /**
     * Retrieves the id of an album based on its unique album code.
     * @param albumcode The album code.
     * @return The id of the album, -1 if not foud.
     */
    int getAlbumIdByAlbumCode( const QString &albumcode );



    /**
     * Begins a database transaction. Must be followed by a later call to commit()
     */
    void begin();

    /**
     * Completes (executes) a database transaction. Must be preceded by a call to begin()
     */
    void commit();

};

#endif
