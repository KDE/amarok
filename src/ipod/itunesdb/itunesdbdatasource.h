/***************************************************************************
 *   Copyright (C) 2004 by Michael Schulze                                 *
 *   mike.s@genion.de                                                      *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef ITUNESDBITUNESDBDATASOURCE_H
#define ITUNESDBITUNESDBDATASOURCE_H

#include <qvaluelist.h>

#include "track.h"
#include "ipod_playlist.h"
#include "ipod_playlistitem.h"

namespace itunesdb {

/**
 * ItunesDBDataSource is the interface that delivers data to an iTunesDBWriter
 * @author Michael Schulze
*/
class ItunesDBDataSource{
public:
    /**
     * called when the ItunesDBWriter starts to write
     */
    virtual void writeInit() = 0;

    /**
     * called when the ItunesDBWriter is done writing the database
     */
    virtual void writeFinished() = 0;

    /**
     * returns the number of playlists that need to be written
     */
    virtual Q_UINT32 getNumPlaylists() = 0;

    /**
     * returns the number of tracks that need to be written
     */
    virtual Q_UINT32 getNumTracks() = 0;

    /**
     * returns the main playlist. This playlist is special since every track needs to be in there
     */
    virtual IPodPlaylist * getMainplaylist() = 0;

    /**
     * returns the first playlist and sets the playlist position for nextPlaylist() to the first
     * playlist
     *
     */
    virtual IPodPlaylist * firstPlaylist() = 0;

    /**
     * returns the next playlist (see firstPlaylist())
     */
    virtual IPodPlaylist * nextPlaylist() = 0;

    /**
     * returns the first Track that needs to be written and sets the track position for nextTrack()
     * to the first track.
     */
    virtual Track * firstTrack() = 0;

    /**
     * returns the next Track (see firstTrack())
     */
    virtual Track * nextTrack() = 0;

    /**
     * gets called if an error occured during the write process.
     * The write process ends after a call to this method.
     */
    virtual void handleError( const QString& message) = 0;
};

}

#endif
