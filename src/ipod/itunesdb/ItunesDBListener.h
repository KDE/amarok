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
#ifndef ITUNESDBITUNESDBLISTENER_H
#define ITUNESDBITUNESDBLISTENER_H

#include <qstring.h>
#include "track.h"
#include "ipod_playlist.h"
#include "ipod_playlistitem.h"

namespace itunesdb {

/**
 * The methods in this interface will be called by ITunesDBParser during the parsing process.
 * @author Michael Schulze
*/
class ItunesDBListener {

public:

    /**
     * gets called if an error occured during the parse process.
     * The parse process ends after a call to this method.
     */
    virtual void handleError( const QString& message) = 0;

    /**
     * sets the number of tracks (information from the itunesdb file)
     */
    virtual void setNumTracks( Q_UINT32 numtracks) = 0;

    /**
     * sets the number of playlists (information from the itunesdb file)
     */
    virtual void setNumPlaylists( Q_UINT32 numplaylists) = 0;

    /**
     * handles the given playlist.
     * The given playlist is only valid during the call.
     */
    virtual void handlePlaylist( const IPodPlaylist& playlist) = 0;

    /**
     * handles the given track.
     * The given playlist is only valid during the call.
     */
    virtual void handleTrack( const Track& track) = 0;

    /**
     * gets called at the beginning of the parse process.
     */
    virtual void parseStarted() = 0;

    /**
     * parsing process done. No calls to the methods above will happen from the parser.
     */
    virtual void parseFinished() = 0;
};

}

#endif
