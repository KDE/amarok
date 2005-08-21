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
#ifndef ITUNESDBITUNESDBPARSER_H
#define ITUNESDBITUNESDBPARSER_H

#define ITUNESDBPARSER_BUFFERSIZE 1024

#include "ItunesDBListener.h"
#include "track.h"
#include "ipod_playlistitem.h"
#include "ipod_playlist.h"

#include <qfile.h>

namespace itunesdb {

/**
 * parses an iTunesDB file and calls the appropriate methods of a given ITunesDBListener
 @author Michael Schulze
*/
class ItunesDBParser{
private:
    ItunesDBListener * listener;
    IPodPlaylist current_playlist;

public:
    /**
     * creates a new parser that calls the appropriate methods on the given listener during the parse process
     */
    ItunesDBParser(ItunesDBListener& listener);

    virtual ~ItunesDBParser();

    /**
     * parses the given iTunesDB file
     * @param file the file that should be parsed.
     */
    void parse(QFile& file);

private:
    // static void setTrackInfo( Track& track, QString data, Q_UINT32 field);
    void handleItem( ListItem& item);
    void seekRelative(QDataStream& stream, uint numbytes);
};

}

#endif
