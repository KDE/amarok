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
#ifndef ITUNESDBITUNESDBWRITER_H
#define ITUNESDBITUNESDBWRITER_H

#include <qfile.h>

#include "itunesdbdatasource.h"

namespace itunesdb {

/**
Writes an iTunesDB file
Documentation about the iTunesDB and iTunesSD formats from 
http://www.ipodlinux.org/ITunesDB

@author Michael Schulze
*/
class ItunesDBWriter{
public:
    ItunesDBWriter(ItunesDBDataSource* trackdatasource);

    ~ItunesDBWriter();

    /**
     * Writes the contents of the ItunesDBDataSource to the given file
     * @param file the file to write the information to
     */
    void write(QFile& file);
    /**
     * Writes the iTunesSD contents to the given file
     * @param file the file to write the information to
     */
    void writeSD( QFile& file );

private:
    ItunesDBDataSource * datasource;

    void fillTrackBuffer( QByteArray& buffer);
    //void fillPodcastBuffer( QByteArray& buffer );
    void fillPlaylistBuffer( QByteArray& buffer);
    void fillTrackBufferSD( QByteArray& buffer );

    void write3ByteLittle( QDataStream &stream, int v );
};

}

#endif
