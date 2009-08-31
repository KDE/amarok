/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#ifndef OPMLDIRECTORYDATABASEHANDLER_H
#define OPMLDIRECTORYDATABASEHANDLER_H

#include "OpmlDirectoryMeta.h"


#include <QStringList>
#include <QMap>


/**
 * This class wraps the database operations needed by the OpmlDirectory
 *
 * @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
 */
class OpmlDirectoryDatabaseHandler {
public:
    /**
     * Private constructor (singleton pattern)
     * @return Pointer to new object
     */
    OpmlDirectoryDatabaseHandler();
    ~OpmlDirectoryDatabaseHandler();

    /**
     * Creates the tables needed to store OpmlDirectory info
     */
    void createDatabase();

    /**
     * Destroys OpmlDirectory tables
     */
    void destroyDatabase();

    /**
     * Inserts a new track into the OpmlDirectory database
     * @param track pointer to the track to insert
     * @return the database id of the newly inserted track
     */
    int insertTrack( Meta::ServiceTrackPtr track );

    /**
     * inserts a new album into the OpmlDirectory database
     * @param album pointer to the album to insert
     * @return the database id of the newly inserted album
     */
    int insertAlbum( Meta::ServiceAlbumPtr album );

    /**
     * Begins a database transaction. Must be followed by a later call to commit()
     */
    void begin();

    /**
     * Completes (executes) a database transaction. Must be preceded by a call to begin()
     */
    void commit();

    /**
     * Remove all genres that are applied to too few albums in an attempt to weed out the worst mistags and
     * speed up queries a bit!
     * @param minCount cutoff value...
     */
    void trimGenres( int minCount );
};

#endif
